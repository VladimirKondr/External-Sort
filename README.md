# External Sort

C++20 библиотека для сортировки данных, превышающих объём оперативной памяти, с использованием алгоритма **k-way external merge sort**.

## Задача

При работе с большими данными (логи, базы данных, научные вычисления) часто возникает необходимость сортировки файлов размером в десятки и сотни гигабайт — значительно больше доступной RAM. Стандартные in-memory алгоритмы (`std::sort`) здесь неприменимы.

**External Sort** решает эту задачу:
- Читает данные порциями, помещающимися в память
- Сортирует каждую порцию и записывает во временный файл (run)
- Сливает runs через k-way merge до получения финального отсортированного файла

## Возможности

- Сортировка файлов любого размера при ограниченной памяти
- Поддержка любых сериализуемых типов (POD, `std::string`, `std::vector`, custom)
- Настраиваемые параметры: объём памяти, степень слияния (k), размер буферов
- Сортировка по возрастанию и убыванию
- Move-семантика для эффективной работы с дорогостоящими типами
- Абстракция от хранилища (файлы или in-memory для тестов)
- Интеграция с любой системой логирования

## Быстрый старт

```cpp
#include "k_way_merge_sorter.hpp"
#include "file_stream.hpp"

int main() {
    // Фабрика для работы с файлами (временные файлы в "temp_dir")
    io::FileStreamFactory<int> factory("temp_dir");

    // Сортировщик
    external_sort::KWayMergeSorter<int> sorter(
        factory,
        "input.bin",           // входной файл
        "sorted.bin",          // выходной файл
        64 * 1024 * 1024,      // 64 MB памяти для runs
        8,                     // 8-way merge
        4096,                  // буфер I/O
        true                   // по возрастанию
    );

    sorter.Sort();
    return 0;
}
```

## Архитектура

Проект состоит из четырёх модулей, каждый со своей зоной ответственности:

```
┌─────────────────────────────────────────────────────────────────────┐
│                          external_sort                               │
│                   (алгоритм k-way merge sort)                        │
│                                                                      │
│  Использует io для чтения/записи, serialization для работы с        │
│  произвольными типами, logging для отладки и мониторинга            │
└─────────────────────────────────────────────────────────────────────┘
        │                      │                      │
        ▼                      ▼                      ▼
┌───────────────┐    ┌─────────────────┐    ┌─────────────────┐
│      io       │    │  serialization  │    │     logging     │
│               │    │                 │    │                 │
│ Потоки ввода/ │    │ Сериализация    │    │ Абстрактный     │
│ вывода,       │    │ типов в бинар-  │    │ интерфейс       │
│ временные     │    │ ный формат      │    │ логирования     │
│ файлы         │    │                 │    │                 │
└───────────────┘    └─────────────────┘    └─────────────────┘
```

### Взаимодействие модулей

1. **external_sort** — главный модуль, реализует алгоритм сортировки
   - Получает данные через абстрактный `IStreamFactory<T>`
   - Использует `Serializer<T>` для расчёта размера элементов в памяти
   - Логирует прогресс через глобальный `logging::Registry`

2. **io** — абстракция ввода-вывода
   - `FileStreamFactory` — работа с реальными файлами
   - `InMemoryStreamFactory` — работа в памяти (для тестов)
   - `TempFileManager` — управление временными файлами
   - Использует `serialization` для записи/чтения элементов

3. **serialization** — сериализация данных
   - Автоматическая поддержка POD типов
   - Встроенная поддержка `std::string`, `std::vector<T>`
   - Расширяемость через методы или специализации
   - C++20 concepts для проверки типов

4. **logging** — гибкое логирование
   - Абстрактный интерфейс `ILogger`
   - Готовая интеграция с spdlog
   - `NullLogger` для отключения логов
   - `LoggerAdapter` для любого пользовательского логгера

## Документация модулей

| Модуль | Описание | Документация |
|--------|----------|--------------|
| **external_sort** | Алгоритм k-way merge sort | [README](external_sort/README.md) |
| **io** | Потоки ввода-вывода, буферизация | [README](io/README.md) |
| **serialization** | Сериализация типов | [README](serialization/README.md) |
| **logging** | Система логирования | [README](logging/README.md) |

## Сборка

### Требования

- **Bazel** (рекомендуется 7.0+)
- **C++20** компилятор (GCC 11+, Clang 14+, MSVC 2022+)
- macOS, Linux или Windows

### Зависимости (подтягиваются автоматически)

| Зависимость | Версия | Назначение |
|-------------|--------|------------|
| GoogleTest | 1.14.0 | Тестирование |
| spdlog | 1.16.0 | Логирование (опционально) |
| Google Benchmark | 1.9.4 | Бенчмарки |

### Команды

```bash
# Сборка всего проекта
bazel build //...

# Запуск всех тестов
bazel test //...

# Пример использования
bazel run //external_sort:example

# Пример сериализации
bazel run //serialization:example

# Бенчмарки производительности
bazel run -c opt //external_sort:performance_runner

# Генерация compile_commands.json (для IDE)
bazel run @hedron_compile_commands//:refresh_all
```

## Поддерживаемые типы данных

### Автоматически (POD)

```cpp
int, double, float, char, uint64_t, ...
struct Point { double x, y, z; };  // trivially copyable
```

### Встроенная поддержка

```cpp
std::string
std::vector<T>  // где T — любой поддерживаемый тип
```

### Custom типы

```cpp
struct MyRecord {
    uint64_t id;
    std::string name;

    // Методы сериализации
    bool Serialize(FILE* file) const;
    bool Deserialize(FILE* file);

    // Для оптимальной производительности
    uint64_t GetSerializedSize() const;

    // Для сортировки
    bool operator<(const MyRecord& other) const;
};
```

Подробнее: [serialization/README.md](serialization/README.md)

## Параметры сортировки

| Параметр | Описание | Рекомендация |
|----------|----------|--------------|
| `mem_bytes` | Память для runs | Больше → меньше runs → быстрее |
| `k_degree` | Степень k-way | 4-16 оптимально |
| `io_buf_elems` | Буфер I/O | 1024-8192 элементов |

```cpp
// Для файла ~10 GB
KWayMergeSorter<MyType> sorter(
    factory,
    "huge_input.bin",
    "sorted_output.bin",
    256 * 1024 * 1024,  // 256 MB
    16,                  // 16-way merge
    8192,
    true
);
```

## Производительность

Алгоритм оптимизирован для минимизации I/O операций:

- **Время:** O(N log N)
- **I/O:** O((N/M) × log_k(N/M)) проходов по данным
- **Диск:** ~2x размер входных данных (временные файлы)

Где N — количество элементов, M — память для runs, k — степень слияния.

### Бенчмарки

```bash
bazel run -c opt //external_sort:performance_runner
```

Тестируемые сценарии:
- Разные размеры памяти (16/64/256 MB)
- Разные степени слияния (2/8/32/128)
- Разные размеры файлов (5M-100M элементов)
- Разные распределения (random/sorted/reverse)
- Разные типы (uint64_t/string/struct)

## Тестирование

```bash
# Все тесты
bazel test //...

# Тесты конкретного модуля
bazel test //external_sort/...
bazel test //io/...
bazel test //serialization/...

# С подробным выводом
bazel test //... --test_output=streamed
```

## Структура проекта

```
External-Sort/
├── external_sort/           # Основной алгоритм сортировки
│   ├── include/
│   │   └── k_way_merge_sorter.hpp
│   ├── examples/
│   ├── benchmarks/
│   └── tests/
│
├── io/                      # Ввод-вывод и буферизация
│   ├── include/
│   │   ├── interfaces.hpp   # IInputStream, IOutputStream, IStreamFactory
│   │   ├── file_stream.hpp
│   │   └── memory_stream.hpp
│   └── tests/
│
├── serialization/           # Сериализация типов
│   ├── include/
│   │   ├── serializers.hpp
│   │   └── type_concepts.hpp
│   ├── examples/
│   └── tests/
│
├── logging/                 # Система логирования
│   ├── include/
│   │   ├── ILogger.hpp
│   │   ├── Registry.hpp
│   │   └── SpdlogWrapper.hpp
│   └── src/
│
├── MODULE.bazel             # Зависимости Bazel
├── WORKSPACE
└── README.md
```

## Ограничения

- **Endianness:** Бинарный формат зависит от архитектуры (little-endian/big-endian). Файлы, созданные на одной архитектуре, могут быть некорректно прочитаны на другой
- **Версионирование формата:** Отсутствует — при изменении структуры типа старые файлы станут несовместимы
- **Многопоточность:** Сортировка выполняется в одном потоке. Для параллельной обработки нескольких файлов создавайте отдельные экземпляры `KWayMergeSorter`
- **Дисковое пространство:** Требуется ~2x размера входных данных для временных файлов
- **Минимальная память:** `mem_bytes` должен вмещать хотя бы один элемент

## Вклад в проект

Проект открыт для вклада! Если вы хотите помочь:

1. Форкните репозиторий
2. Создайте ветку для вашей функции (`git checkout -b feature/amazing-feature`)
3. Сделайте коммит изменений (`git commit -m 'Add amazing feature'`)
4. Запушьте ветку (`git push origin feature/amazing-feature`)
5. Откройте Pull Request

### Перед отправкой PR

```bash
# Убедитесь, что все тесты проходят
bazel test //...

# Проверьте форматирование (если настроен clang-format)
# clang-format -i <измененные файлы>
```

## Лицензия

MIT License

Copyright (c) 2024 Vladimir Kondratyonok

## Автор

**Vladimir Kondratyonok**

---

**Ссылки на документацию модулей:**
- [external_sort](external_sort/README.md) — алгоритм сортировки
- [io](io/README.md) — потоки ввода-вывода
- [serialization](serialization/README.md) — сериализация
- [logging](logging/README.md) — логирование
