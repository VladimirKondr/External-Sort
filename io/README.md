# Модуль IO

Абстрактный слой ввода-вывода для работы с потоками данных. Поддерживает файловое и in-memory хранение с единым интерфейсом.

## Содержание

- [Обзор](#обзор)
- [Архитектура](#архитектура)
- [Компоненты](#компоненты)
- [Сборка](#сборка)
- [Примеры использования](#примеры-использования)
- [API Reference](#api-reference)
- [Формат файлов](#формат-файлов)

---

## Обзор

Модуль предоставляет:

- Абстрактные интерфейсы `IInputStream<T>`, `IOutputStream<T>`, `IStreamFactory<T>`
- Файловую реализацию с буферизацией и сериализацией
- In-memory реализацию для тестирования
- Управление временными файлами с автоматической очисткой
- Move-семантику для эффективной работы с дорогостоящими типами
- Интеграцию с модулем сериализации

---

## Архитектура

```
┌─────────────────────────────────────────────────────────────┐
│                    IStreamFactory<T>                         │
│  (CreateInputStream, CreateOutputStream, CreateTempOutput)   │
└─────────────────────────────────────────────────────────────┘
              ▲                              ▲
              │                              │
┌─────────────┴─────────────┐  ┌─────────────┴─────────────┐
│   FileStreamFactory<T>    │  │  InMemoryStreamFactory<T> │
│  (работа с файлами)       │  │  (работа с памятью)       │
└───────────────────────────┘  └───────────────────────────┘
              │                              │
              ▼                              ▼
┌─────────────────────────────────────────────────────────────┐
│              IInputStream<T> / IOutputStream<T>              │
│     (Advance, Value, TakeValue, Write, Finalize, etc.)      │
└─────────────────────────────────────────────────────────────┘
              ▲                              ▲
              │                              │
┌─────────────┴─────────────┐  ┌─────────────┴─────────────┐
│  FileInputStream<T>       │  │  InMemoryInputStream<T>   │
│  FileOutputStream<T>      │  │  InMemoryOutputStream<T>  │
└───────────────────────────┘  └───────────────────────────┘
              │
              ▼
┌───────────────────────────┐
│     ElementBuffer<T>      │
│  (буферизация чтения/     │
│   записи)                 │
└───────────────────────────┘
```

---

## Компоненты

### StorageId (`include/storage_types.hpp`)

Алиас для идентификации хранилища:

```cpp
using StorageId = std::string;  // путь к файлу или имя in-memory хранилища
```

### IInputStream<T> (`include/interfaces.hpp`)

Интерфейс входного потока:

```cpp
template <typename T>
class IInputStream {
public:
    virtual void Advance() = 0;              // Перейти к следующему элементу
    virtual const T& Value() const = 0;      // Получить текущий элемент (const ref)
    virtual T TakeValue() = 0;               // Забрать элемент (move)
    virtual bool IsExhausted() const = 0;    // Поток исчерпан?
    virtual bool IsEmptyOriginalStorage() const = 0;  // Хранилище было пустым?
};
```

### IOutputStream<T> (`include/interfaces.hpp`)

Интерфейс выходного потока:

```cpp
template <typename T>
class IOutputStream {
public:
    virtual void Write(const T& value) = 0;   // Записать элемент (копия)
    virtual void Write(T&& value) = 0;        // Записать элемент (move)
    virtual void Finalize() = 0;              // Завершить запись
    virtual uint64_t GetTotalElementsWritten() const = 0;  // Кол-во элементов
    virtual uint64_t GetTotalBytesWritten() const = 0;     // Кол-во байт
    virtual StorageId GetId() const = 0;      // Идентификатор хранилища
};
```

### IStreamFactory<T> (`include/interfaces.hpp`)

Фабрика для создания потоков:

```cpp
template <typename T>
class IStreamFactory {
public:
    virtual std::unique_ptr<IInputStream<T>> CreateInputStream(
        const StorageId& id, uint64_t buffer_capacity_elements) = 0;

    virtual std::unique_ptr<IOutputStream<T>> CreateOutputStream(
        const StorageId& id, uint64_t buffer_capacity_elements) = 0;

    virtual std::unique_ptr<IOutputStream<T>> CreateTempOutputStream(
        StorageId& out_temp_id, uint64_t buffer_capacity_elements) = 0;

    virtual void DeleteStorage(const StorageId& id) = 0;
    virtual void MakeStoragePermanent(const StorageId& temp_id, const StorageId& final_id) = 0;
    virtual bool StorageExists(const StorageId& id) const = 0;
    virtual StorageId GetTempStorageContextId() const = 0;
};
```

### FileStreamFactory<T> (`include/file_stream.hpp`)

Файловая реализация фабрики:

- Создаёт `FileInputStream<T>` и `FileOutputStream<T>`
- Управляет временными файлами через `TempFileManager`
- Поддерживает любые сериализуемые типы

### InMemoryStreamFactory<T> (`include/memory_stream.hpp`)

In-memory реализация фабрики:

- Хранит данные в `std::map<StorageId, std::shared_ptr<std::vector<T>>>`
- Идеально для unit-тестов
- Не требует файловой системы

### ElementBuffer<T> (`include/element_buffer.hpp`)

Внутренний буфер для оптимизации I/O:

- Буферизует чтение/запись для уменьшения системных вызовов
- Поддерживает move-семантику через `PushBack(T&&)` и `ReadNext()`

### TempFileManager (`include/temp_file_manager.hpp`)

Менеджер временных файлов:

- Автоматически создаёт директорию для временных файлов
- Генерирует уникальные имена файлов
- Автоматически удаляет директорию в деструкторе (если создал сам)

---

## Сборка

### Bazel targets

```python
# Только буфер элементов
"//io:element_buffer"

# Полный модуль IO (рекомендуется)
"//io:io"
```

### Зависимости

```python
deps = [
    "//io:io",
    "//serialization",  # автоматически подключается
    "//logging",        # автоматически подключается
]
```

### Тесты

```bash
# Все тесты модуля
bazel test //io/...

# Конкретные тесты
bazel test //io:element_buffer_test
bazel test //io:file_stream_test
bazel test //io:memory_stream_test
bazel test //io:temp_file_manager_test
```

---

## Примеры использования

### 1. Базовая запись и чтение файла (POD тип)

```cpp
#include "file_stream.hpp"

int main() {
    io::FileStreamFactory<int> factory("temp_dir");

    // Запись данных
    {
        auto output = factory.CreateOutputStream("numbers.bin", 100);
        for (int i = 0; i < 1000; ++i) {
            output->Write(i);
        }
        output->Finalize();

        std::cout << "Written: " << output->GetTotalElementsWritten()
                  << " elements, " << output->GetTotalBytesWritten()
                  << " bytes" << std::endl;
    }

    // Чтение данных
    {
        auto input = factory.CreateInputStream("numbers.bin", 100);
        while (!input->IsExhausted()) {
            int value = input->Value();
            std::cout << value << " ";
            input->Advance();
        }
    }

    return 0;
}
```

### 2. Работа со строками

```cpp
#include "file_stream.hpp"

int main() {
    io::FileStreamFactory<std::string> factory("data");

    // Запись строк
    {
        auto output = factory.CreateOutputStream("words.bin", 50);
        output->Write("Hello");
        output->Write("World");
        output->Write("External Sort!");
        output->Finalize();
    }

    // Чтение строк с move-семантикой
    {
        auto input = factory.CreateInputStream("words.bin", 50);
        std::vector<std::string> words;

        while (!input->IsExhausted()) {
            // TakeValue() использует move — эффективно для строк
            words.push_back(input->TakeValue());
            input->Advance();
        }

        for (const auto& word : words) {
            std::cout << word << std::endl;
        }
    }

    return 0;
}
```

### 3. Использование временных файлов

```cpp
#include "file_stream.hpp"

int main() {
    io::FileStreamFactory<double> factory("temp");

    io::StorageId temp_id;

    // Создание временного файла
    {
        auto temp_output = factory.CreateTempOutputStream(temp_id, 100);
        for (int i = 0; i < 500; ++i) {
            temp_output->Write(i * 0.5);
        }
        temp_output->Finalize();
    }

    std::cout << "Temp file: " << temp_id << std::endl;

    // Превращение временного файла в постоянный
    factory.MakeStoragePermanent(temp_id, "final_data.bin");

    // Проверка существования
    std::cout << "Exists: " << factory.StorageExists("final_data.bin") << std::endl;

    // Удаление
    factory.DeleteStorage("final_data.bin");

    return 0;
}
```

### 4. In-memory потоки для тестов

```cpp
#include "memory_stream.hpp"

void TestSorting() {
    io::InMemoryStreamFactory<int> factory;

    // Создание тестовых данных
    {
        auto output = factory.CreateOutputStream("test_input", 100);
        output->Write(5);
        output->Write(2);
        output->Write(8);
        output->Write(1);
        output->Finalize();
    }

    // Проверка данных
    auto data = factory.GetStorageData("test_input");
    assert(data != nullptr);
    assert(data->size() == 4);

    // Чтение
    auto input = factory.CreateInputStream("test_input", 100);
    std::vector<int> result;
    while (!input->IsExhausted()) {
        result.push_back(input->TakeValue());
        input->Advance();
    }

    // result: {5, 2, 8, 1}
}
```

### 5. Custom структура

```cpp
#include "file_stream.hpp"

struct Point {
    double x, y, z;

    // Методы сериализации
    bool Serialize(FILE* file) const {
        return fwrite(this, sizeof(Point), 1, file) == 1;
    }

    bool Deserialize(FILE* file) {
        return fread(this, sizeof(Point), 1, file) == 1;
    }

    uint64_t GetSerializedSize() const {
        return sizeof(Point);
    }
};

int main() {
    io::FileStreamFactory<Point> factory("points_data");

    // Запись точек
    {
        auto output = factory.CreateOutputStream("points.bin", 100);
        output->Write(Point{1.0, 2.0, 3.0});
        output->Write(Point{4.0, 5.0, 6.0});
        output->Finalize();
    }

    // Чтение точек
    {
        auto input = factory.CreateInputStream("points.bin", 100);
        while (!input->IsExhausted()) {
            Point p = input->TakeValue();
            std::cout << "(" << p.x << ", " << p.y << ", " << p.z << ")\n";
            input->Advance();
        }
    }

    return 0;
}
```

### 6. Эффективная работа с move-семантикой

```cpp
#include "file_stream.hpp"
#include <vector>

int main() {
    io::FileStreamFactory<std::vector<int>> factory("vectors");

    // Запись векторов с move
    {
        auto output = factory.CreateOutputStream("data.bin", 10);

        std::vector<int> v1 = {1, 2, 3, 4, 5};
        std::vector<int> v2 = {10, 20, 30};

        // Move — избегаем копирования больших векторов
        output->Write(std::move(v1));
        output->Write(std::move(v2));

        // v1 и v2 теперь пусты
        output->Finalize();
    }

    // Чтение с TakeValue()
    {
        auto input = factory.CreateInputStream("data.bin", 10);

        while (!input->IsExhausted()) {
            // TakeValue() возвращает вектор через move
            std::vector<int> vec = input->TakeValue();

            std::cout << "Vector size: " << vec.size() << std::endl;
            input->Advance();
        }
    }

    return 0;
}
```

### 7. Копирование данных между хранилищами

```cpp
#include "file_stream.hpp"

template <typename T>
void CopyStorage(io::IStreamFactory<T>& factory,
                 const io::StorageId& src,
                 const io::StorageId& dst,
                 uint64_t buffer_size = 1024) {
    auto input = factory.CreateInputStream(src, buffer_size);
    auto output = factory.CreateOutputStream(dst, buffer_size);

    while (!input->IsExhausted()) {
        output->Write(input->TakeValue());
        input->Advance();
    }

    output->Finalize();
}

int main() {
    io::FileStreamFactory<int> factory("data");

    // ... создать source.bin ...

    CopyStorage(factory, "source.bin", "copy.bin");

    return 0;
}
```

### 8. Отслеживание размера записанных данных

```cpp
#include "file_stream.hpp"

int main() {
    io::FileStreamFactory<std::string> factory("bench");

    auto output = factory.CreateOutputStream("strings.bin", 100);

    std::vector<std::string> data = {
        "short",
        "a medium length string",
        "this is a much longer string that takes more space"
    };

    for (const auto& s : data) {
        output->Write(s);

        // Отслеживание прогресса
        std::cout << "Elements: " << output->GetTotalElementsWritten()
                  << ", Bytes: " << output->GetTotalBytesWritten()
                  << std::endl;
    }

    output->Finalize();

    // Финальный размер
    std::cout << "Total: " << output->GetTotalBytesWritten() << " bytes\n";

    return 0;
}
```

---

## API Reference

### IInputStream<T>

| Метод | Описание |
|-------|----------|
| `void Advance()` | Переход к следующему элементу |
| `const T& Value() const` | Текущий элемент (const reference) |
| `T TakeValue()` | Забрать текущий элемент (move) |
| `bool IsExhausted() const` | Проверка конца потока |
| `bool IsEmptyOriginalStorage() const` | Было ли хранилище изначально пустым |

### IOutputStream<T>

| Метод | Описание |
|-------|----------|
| `void Write(const T&)` | Записать элемент (копия) |
| `void Write(T&&)` | Записать элемент (move) |
| `void Finalize()` | Завершить запись |
| `uint64_t GetTotalElementsWritten() const` | Количество записанных элементов |
| `uint64_t GetTotalBytesWritten() const` | Количество записанных байт |
| `StorageId GetId() const` | ID хранилища |

### IStreamFactory<T>

| Метод | Описание |
|-------|----------|
| `CreateInputStream(id, buffer)` | Создать входной поток |
| `CreateOutputStream(id, buffer)` | Создать выходной поток |
| `CreateTempOutputStream(out_id, buffer)` | Создать временный выходной поток |
| `DeleteStorage(id)` | Удалить хранилище |
| `MakeStoragePermanent(temp, final)` | Переименовать временное в постоянное |
| `StorageExists(id)` | Проверить существование |
| `GetTempStorageContextId()` | Путь к временной директории |

### ElementBuffer<T>

| Метод | Описание |
|-------|----------|
| `bool PushBack(const T&)` | Добавить элемент (возвращает true если буфер полон) |
| `bool PushBack(T&&)` | Добавить элемент (move) |
| `T ReadNext()` | Прочитать следующий элемент |
| `bool HasMoreToRead() const` | Есть ещё элементы для чтения |
| `uint64_t Size() const` | Количество элементов |
| `uint64_t Capacity() const` | Ёмкость буфера |
| `void Clear()` | Очистить буфер |

### TempFileManager

| Метод | Описание |
|-------|----------|
| `TempFileManager(base_dir)` | Конструктор с базовой директорией |
| `std::string GenerateTempFilename(prefix, ext)` | Сгенерировать уникальное имя |
| `void CleanupFile(path)` | Удалить файл |
| `const fs::path& GetBaseDirPath() const` | Путь к базовой директории |

---

## Формат файлов

Файлы, созданные `FileOutputStream`, имеют следующий формат:

```
┌────────────────────────────────────────┐
│  uint64_t: количество элементов (N)    │  ← 8 байт заголовок
├────────────────────────────────────────┤
│  Element 0 (serialized)                │
├────────────────────────────────────────┤
│  Element 1 (serialized)                │
├────────────────────────────────────────┤
│  ...                                   │
├────────────────────────────────────────┤
│  Element N-1 (serialized)              │
└────────────────────────────────────────┘
```

**Для POD типов:** элементы записываются как есть (`sizeof(T)` байт каждый)

**Для std::string:**
```
┌──────────────────────────┐
│ uint64_t: длина строки   │
├──────────────────────────┤
│ char[]: данные строки    │
└──────────────────────────┘
```

---

## Thread Safety

- `FileInputStream` / `FileOutputStream` — **не thread-safe** (один поток на файл)
- `InMemoryStreamFactory` — **не thread-safe** (разделяемое состояние)
- Для многопоточной работы используйте отдельные фабрики или внешнюю синхронизацию

---

## Ограничения

### Известные ограничения

- **Размер файла:** Максимальный размер ограничен `uint64_t` элементов (~18 экзабайт при sizeof(T)=1)
- **Буфер:** Размер буфера задаётся в элементах, не в байтах. Для больших элементов учитывайте потребление памяти
- **Временные файлы:** При аварийном завершении программы временные файлы могут остаться на диске
- **Права доступа:** Требуются права на чтение/запись в указанные директории

### Рекомендации по размеру буфера

| Размер элемента | Рекомендуемый buffer_capacity |
|-----------------|------------------------------|
| < 100 байт | 4096-8192 |
| 100-1000 байт | 1024-4096 |
| > 1000 байт | 256-1024 |

### Обработка ошибок

| Ошибка | Поведение |
|--------|-----------|
| Файл не существует (чтение) | Исключение при создании потока |
| Нет прав на запись | Исключение при создании потока |
| Диск заполнен | Исключение при `Write()` или `Finalize()` |
| Corrupted file | Неопределённое поведение при десериализации |

---

## Лицензия

MIT License. См. корневой файл LICENSE проекта.
