# Модуль External Sort

Реализация алгоритма **k-way external merge sort** для сортировки данных, превышающих объём оперативной памяти.

## Содержание

- [Обзор](#обзор)
- [Алгоритм](#алгоритм)
- [Архитектура](#архитектура)
- [Компоненты](#компоненты)
- [Сборка](#сборка)
- [Примеры использования](#примеры-использования)
- [Параметры и оптимизация](#параметры-и-оптимизация)
- [Бенчмарки](#бенчмарки)
- [API Reference](#api-reference)

---

## Обзор

Модуль предоставляет:

- K-way merge sort для файлов любого размера
- Абстракцию от хранилища через интерфейсы `IStreamFactory`
- Поддержку любых сериализуемых типов (POD, string, vector, custom)
- Настраиваемый объём памяти и степень слияния (k)
- Move-семантику для эффективной работы с дорогостоящими типами
- Сортировку по возрастанию и убыванию
- Интеграцию с системой логирования

---

## Алгоритм

### Фаза 1: Создание начальных runs

```
┌─────────────────────────────────────────────────────────────────┐
│                        Входной файл                              │
│  [5, 2, 8, 1, 9, 3, 7, 4, 6, 0, ...]                            │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  Читаем элементы пока не заполним memory_for_runs_bytes         │
│  Сортируем в памяти (std::sort)                                 │
│  Записываем во временный файл (run)                             │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐
│  Run 1  │  │  Run 2  │  │  Run 3  │  │  Run 4  │  ...
│ [1,2,5] │  │ [3,8,9] │  │ [4,6,7] │  │ [0,...] │
└─────────┘  └─────────┘  └─────────┘  └─────────┘
```

### Фаза 2: K-way слияние

```
Pass 1 (k=2):
┌─────────┐  ┌─────────┐       ┌─────────┐  ┌─────────┐
│  Run 1  │  │  Run 2  │       │  Run 3  │  │  Run 4  │
└────┬────┘  └────┬────┘       └────┬────┘  └────┬────┘
     │            │                 │            │
     └─────┬──────┘                 └─────┬──────┘
           ▼                               ▼
    ┌─────────────┐                 ┌─────────────┐
    │  Merged 1   │                 │  Merged 2   │
    └─────────────┘                 └─────────────┘

Pass 2 (k=2):
    ┌─────────────┐                 ┌─────────────┐
    │  Merged 1   │                 │  Merged 2   │
    └──────┬──────┘                 └──────┬──────┘
           │                               │
           └───────────────┬───────────────┘
                           ▼
                 ┌─────────────────┐
                 │   Final Output   │
                 │   (sorted!)      │
                 └─────────────────┘
```

### Сложность

| Операция | Сложность |
|----------|-----------|
| Время | O(N log N) |
| I/O операций | O((N/M) * log_k(N/M)) |
| Дисковое пространство | ~2x размер входных данных |

Где:
- N — количество элементов
- M — память для runs (элементов в памяти)
- k — степень слияния

---

## Архитектура

```
┌─────────────────────────────────────────────────────────────────┐
│                      KWayMergeSorter<T>                          │
│                                                                  │
│  ┌──────────────────┐    ┌──────────────────────────────────┐  │
│  │ CreateInitialRuns│───▶│ Читает input → сортирует →        │  │
│  │                  │    │ записывает временные runs         │  │
│  └──────────────────┘    └──────────────────────────────────┘  │
│                                                                  │
│  ┌──────────────────┐    ┌──────────────────────────────────┐  │
│  │ MergeGroupOfRuns │───▶│ Сливает k runs через priority_queue│  │
│  │                  │    │ в один выходной run               │  │
│  └──────────────────┘    └──────────────────────────────────┘  │
│                                                                  │
│  ┌──────────────────┐    ┌──────────────────────────────────┐  │
│  │      Sort()      │───▶│ Повторяет слияние пока не         │  │
│  │                  │    │ останется один run → output       │  │
│  └──────────────────┘    └──────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                      IStreamFactory<T>                           │
│   (FileStreamFactory или InMemoryStreamFactory)                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Компоненты

### KWayMergeSorter<T> (`include/k_way_merge_sorter.hpp`)

Основной класс сортировщика:

```cpp
template <typename T>
class KWayMergeSorter {
public:
    KWayMergeSorter(
        io::IStreamFactory<T>& factory,  // Фабрика потоков
        io::StorageId input_id,          // ID входного хранилища
        io::StorageId output_id,         // ID выходного хранилища
        uint64_t mem_bytes,              // Память для runs (байты)
        uint64_t k_degree,               // Степень k-way (≥2)
        uint64_t io_buf_elems,           // Размер I/O буфера
        bool sort_ascending = true       // Направление сортировки
    );

    void Sort();  // Выполнить сортировку
};
```

### MergeSource<T>

Вспомогательная структура для k-way слияния:

```cpp
template <typename T>
struct MergeSource {
    io::IInputStream<T>* stream;  // Указатель на поток
};
```

---

## Сборка

### Bazel target

```python
"//external_sort:external_sort"
```

### Подключение

```python
cc_binary(
    name = "my_sorter",
    srcs = ["main.cpp"],
    deps = [
        "//external_sort",
        "//io",
    ],
)
```

### Тесты

```bash
# Все тесты модуля
bazel test //external_sort/...

# Файловые тесты
bazel test //external_sort:basic_sorting_file_test
bazel test //external_sort:pod_types_file_test

# In-memory тесты
bazel test //external_sort:basic_sorting_memory_test
bazel test //external_sort:pod_types_memory_test

# Сложные типы
bazel test //external_sort:string_sorting_test
bazel test //external_sort:complex_types_test
```

### Пример и бенчмарки

```bash
# Пример использования
bazel run //external_sort:example

# Бенчмарки производительности
bazel run //external_sort:performance_runner
```

---

## Примеры использования

### 1. Сортировка целых чисел (файлы)

```cpp
#include "k_way_merge_sorter.hpp"
#include "file_stream.hpp"
#include <random>

using external_sort::KWayMergeSorter;
using io::FileStreamFactory;

int main() {
    // Создаём фабрику (временные файлы в "temp_dir")
    FileStreamFactory<int> factory("temp_dir");

    std::string input_file = "input.bin";
    std::string output_file = "sorted.bin";

    // Генерируем тестовые данные
    {
        auto output = factory.CreateOutputStream(input_file, 1024);
        std::mt19937 gen(42);
        std::uniform_int_distribution<int> dist(0, 1'000'000);

        for (int i = 0; i < 10'000'000; ++i) {
            output->Write(dist(gen));
        }
        output->Finalize();
    }

    // Сортируем
    KWayMergeSorter<int> sorter(
        factory,
        input_file,
        output_file,
        64 * 1024 * 1024,  // 64 MB для runs
        8,                  // 8-way merge
        8192,               // буфер 8192 элемента
        true                // по возрастанию
    );

    sorter.Sort();

    // Читаем результат
    {
        auto input = factory.CreateInputStream(output_file, 1024);
        int prev = INT_MIN;

        while (!input->IsExhausted()) {
            int val = input->TakeValue();
            assert(val >= prev);  // Проверяем порядок
            prev = val;
            input->Advance();
        }
    }

    return 0;
}
```

### 2. Сортировка строк

```cpp
#include "k_way_merge_sorter.hpp"
#include "file_stream.hpp"

int main() {
    io::FileStreamFactory<std::string> factory("temp");

    // Создаём входной файл со строками
    {
        auto output = factory.CreateOutputStream("words.bin", 100);
        output->Write("zebra");
        output->Write("apple");
        output->Write("mango");
        output->Write("banana");
        output->Write("cherry");
        output->Finalize();
    }

    // Сортируем (учитываем размер строк в памяти)
    external_sort::KWayMergeSorter<std::string> sorter(
        factory,
        "words.bin",
        "sorted_words.bin",
        1024,  // 1 KB — достаточно для примера
        2,     // 2-way merge
        10,
        true
    );

    sorter.Sort();

    // Результат: apple, banana, cherry, mango, zebra
    return 0;
}
```

### 3. Сортировка по убыванию

```cpp
#include "k_way_merge_sorter.hpp"
#include "file_stream.hpp"

int main() {
    io::FileStreamFactory<double> factory("temp");

    // ... создаём входной файл ...

    external_sort::KWayMergeSorter<double> sorter(
        factory,
        "input.bin",
        "descending.bin",
        16 * 1024 * 1024,
        4,
        1024,
        false  // ← по УБЫВАНИЮ
    );

    sorter.Sort();

    return 0;
}
```

### 4. In-memory сортировка (для тестов)

```cpp
#include "k_way_merge_sorter.hpp"
#include "memory_stream.hpp"

void TestSorting() {
    io::InMemoryStreamFactory<int> factory;

    // Создаём тестовые данные
    {
        auto output = factory.CreateOutputStream("test_input", 100);
        output->Write(5);
        output->Write(2);
        output->Write(8);
        output->Write(1);
        output->Write(9);
        output->Write(3);
        output->Finalize();
    }

    // Сортируем в памяти
    external_sort::KWayMergeSorter<int> sorter(
        factory,
        "test_input",
        "test_output",
        sizeof(int) * 3,  // 3 элемента на run
        2,
        10,
        true
    );

    sorter.Sort();

    // Проверяем результат
    auto result = factory.GetStorageData("test_output");
    // result: {1, 2, 3, 5, 8, 9}
}
```

### 5. Custom структура с методами сериализации

```cpp
#include "k_way_merge_sorter.hpp"
#include "file_stream.hpp"

struct Record {
    uint64_t id;
    double score;
    std::string name;

    // Сериализация
    bool Serialize(FILE* file) const {
        if (fwrite(&id, sizeof(id), 1, file) != 1) return false;
        if (fwrite(&score, sizeof(score), 1, file) != 1) return false;

        uint64_t len = name.length();
        if (fwrite(&len, sizeof(len), 1, file) != 1) return false;
        if (fwrite(name.data(), 1, len, file) != len) return false;

        return true;
    }

    bool Deserialize(FILE* file) {
        if (fread(&id, sizeof(id), 1, file) != 1) return false;
        if (fread(&score, sizeof(score), 1, file) != 1) return false;

        uint64_t len;
        if (fread(&len, sizeof(len), 1, file) != 1) return false;
        name.resize(len);
        if (fread(&name[0], 1, len, file) != len) return false;

        return true;
    }

    // ВАЖНО для производительности!
    uint64_t GetSerializedSize() const {
        return sizeof(id) + sizeof(score) + sizeof(uint64_t) + name.length();
    }

    // Оператор сравнения для сортировки
    bool operator<(const Record& other) const {
        return score < other.score;  // Сортировка по score
    }

    bool operator>(const Record& other) const {
        return score > other.score;
    }
};

int main() {
    io::FileStreamFactory<Record> factory("records_temp");

    // ... создаём входной файл с записями ...

    external_sort::KWayMergeSorter<Record> sorter(
        factory,
        "records.bin",
        "sorted_records.bin",
        32 * 1024 * 1024,  // 32 MB
        4,
        512,
        true
    );

    sorter.Sort();

    return 0;
}
```

### 6. Обработка пустого входа

```cpp
#include "k_way_merge_sorter.hpp"
#include "file_stream.hpp"

int main() {
    io::FileStreamFactory<int> factory("temp");

    // Создаём пустой файл
    {
        auto output = factory.CreateOutputStream("empty.bin", 100);
        output->Finalize();  // Ничего не записываем
    }

    // Сортировка пустого файла — OK
    external_sort::KWayMergeSorter<int> sorter(
        factory,
        "empty.bin",
        "output.bin",
        1024,
        2,
        100,
        true
    );

    sorter.Sort();  // Создаст пустой output.bin

    return 0;
}
```

### 7. С логированием

```cpp
#include "k_way_merge_sorter.hpp"
#include "file_stream.hpp"
#include "SpdlogWrapper.hpp"
#include "Registry.hpp"

int main() {
    // Включаем логирование
    auto logger = std::make_shared<logging::SpdlogWrapper>(
        "sorter",
        logging::SpdlogSinkType::Both,
        "sort.log"
    );
    logging::SetLogger(logger);

    io::FileStreamFactory<int> factory("temp");

    // ... подготовка данных ...

    external_sort::KWayMergeSorter<int> sorter(
        factory,
        "input.bin",
        "output.bin",
        64 * 1024 * 1024,
        8,
        4096,
        true
    );

    // В логах будет информация о каждом run и слиянии
    sorter.Sort();

    return 0;
}
```

---

## Параметры и оптимизация

### Параметры конструктора

| Параметр | Тип | Описание | Рекомендация |
|----------|-----|----------|--------------|
| `mem_bytes` | `uint64_t` | Память для runs | Чем больше — тем меньше runs → быстрее |
| `k_degree` | `uint64_t` | Степень слияния | 4-16 оптимально |
| `io_buf_elems` | `uint64_t` | Буфер I/O | 1024-8192 элементов |
| `sort_ascending` | `bool` | Направление | `true` = по возрастанию |

### Влияние параметров

```
mem_bytes ↑  →  меньше runs  →  меньше проходов слияния  →  быстрее
k_degree ↑  →  меньше проходов  →  но больше памяти на слияние
io_buf ↑    →  меньше I/O вызовов  →  быстрее (до предела)
```

### Формула для runs

```
Количество initial runs ≈ (размер входных данных) / mem_bytes
Количество проходов слияния ≈ ceil(log_k(количество runs))
```

### Рекомендуемые значения

| Размер данных | mem_bytes | k_degree | io_buf |
|---------------|-----------|----------|--------|
| < 100 MB | 16-32 MB | 4 | 1024 |
| 100 MB - 1 GB | 64-128 MB | 8 | 4096 |
| 1-10 GB | 128-256 MB | 16 | 8192 |
| > 10 GB | 256-512 MB | 16-32 | 8192 |

---

## Бенчмарки

### Запуск бенчмарков

```bash
bazel run -c opt //external_sort:performance_runner
```

### Тестируемые сценарии

1. **RAM Limit** — влияние размера памяти (16/64/256 MB)
2. **K-Degree** — влияние степени слияния (2/8/32/128)
3. **File Size** — разные размеры файлов (5M/10M/50M/100M элементов)
4. **Data Distribution** — random/sorted/reverse
5. **Data Types** — uint64_t/std::string/custom struct

### Влияние GetSerializedSize()

Бенчмарки включают сравнение:
- `WithMethodsAndOptimizedSize` — с методом `GetSerializedSize()`
- `WithMethodsNoSizeOptimization` — без метода (fallback на `/dev/null`)

**Разница может быть значительной** — всегда добавляйте `GetSerializedSize()` для custom типов!

---

## API Reference

### KWayMergeSorter<T>

#### Конструктор

```cpp
KWayMergeSorter(
    io::IStreamFactory<T>& factory,
    io::StorageId input_id,
    io::StorageId output_id,
    uint64_t mem_bytes,
    uint64_t k_degree,
    uint64_t io_buf_elems,
    bool sort_ascending = true
);
```

| Параметр | Описание |
|----------|----------|
| `factory` | Фабрика потоков (файловая или in-memory) |
| `input_id` | Идентификатор входного хранилища |
| `output_id` | Идентификатор выходного хранилища |
| `mem_bytes` | Память для создания runs (в байтах) |
| `k_degree` | Степень k-way слияния (минимум 2) |
| `io_buf_elems` | Размер буфера I/O в элементах |
| `sort_ascending` | `true` — по возрастанию, `false` — по убыванию |

**Исключения:**
- `std::invalid_argument` — если `k_degree < 2`
- `std::runtime_error` — если `output_id` внутри временной директории

#### Метод Sort()

```cpp
void Sort();
```

Выполняет полную сортировку:
1. Создаёт initial runs
2. Выполняет k-way слияние
3. Записывает результат в `output_id`

**Исключения:**
- `std::runtime_error` — если `mem_bytes` слишком мал для одного элемента

---

## Требования к типу T

Тип `T` должен удовлетворять:

1. **Сериализуемость** — `serialization::FileSerializable<T>`
2. **Сравнимость** — операторы `<` и `>` (или только нужный)
3. **Default constructible** — для десериализации
4. **Move constructible** (рекомендуется) — для эффективности

```cpp
struct MyType {
    // Обязательно для сортировки
    bool operator<(const MyType& other) const;
    bool operator>(const MyType& other) const;

    // Обязательно для сериализации (один из способов)
    bool Serialize(FILE* file) const;
    bool Deserialize(FILE* file);

    // Рекомендуется для производительности
    uint64_t GetSerializedSize() const;
};
```

---

## Обработка ошибок

| Ситуация | Поведение |
|----------|-----------|
| Пустой вход | Создаётся пустой выходной файл |
| `k_degree < 2` | `std::invalid_argument` |
| Недостаточно памяти для 1 элемента | `std::runtime_error` |
| Ошибка I/O | Пробрасывается исключение из io модуля |
| Output в temp директории | `std::runtime_error` |

---

## Ограничения и edge cases

### Ограничения

- **Однопоточность:** Алгоритм выполняется в одном потоке. Для параллельной сортировки нескольких файлов используйте отдельные экземпляры `KWayMergeSorter`
- **Память:** Параметр `mem_bytes` должен быть достаточен для хранения хотя бы одного элемента типа `T`
- **Дисковое пространство:** Требуется примерно 2x размера входных данных для временных runs
- **k_degree:** Минимальное значение — 2. Слишком большое значение увеличивает потребление памяти при слиянии

### Edge cases

| Сценарий | Поведение |
|----------|-----------|
| Пустой входной файл | Создаётся пустой выходной файл |
| Один элемент | Копируется в выходной файл без слияния |
| Все элементы одинаковые | Корректная сортировка (stable не гарантируется) |
| Уже отсортированные данные | Работает корректно, но без оптимизации |
| `mem_bytes` < размер одного элемента | `std::runtime_error` |

### Важные замечания

- **Стабильность сортировки:** Не гарантируется. Элементы с одинаковыми ключами могут поменять относительный порядок
- **Прерывание:** При исключении или аварийном завершении временные файлы могут остаться на диске
- **Concurrent access:** Не поддерживается одновременный доступ к одному и тому же входному/выходному файлу

---

## Лицензия

MIT License. См. корневой файл LICENSE проекта.
