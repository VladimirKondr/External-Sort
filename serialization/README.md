# Модуль Serialization

Система сериализации для C++20 с поддержкой POD типов, стандартных контейнеров и пользовательских типов через concepts.

## Содержание

- [Обзор](#обзор)
- [Архитектура](#архитектура)
- [Concepts (требования к типам)](#concepts)
- [Компоненты](#компоненты)
- [Сборка](#сборка)
- [Примеры использования](#примеры-использования)
- [Оптимизация производительности](#оптимизация-производительности)
- [API Reference](#api-reference)

---

## Обзор

Модуль предоставляет:

- Автоматическую сериализацию POD типов через `fwrite`/`fread`
- Встроенную поддержку `std::string` и `std::vector<T>`
- Три способа добавления сериализации для custom типов
- C++20 concepts для проверки типов на этапе компиляции
- Расчёт размера сериализованных данных (важно для external sort)
- Интеграцию с системой логирования

---

## Архитектура

```
┌─────────────────────────────────────────────────────────────────┐
│                     CreateSerializer<T>()                        │
│              (фабрика — выбирает нужный сериализатор)            │
└─────────────────────────────────────────────────────────────────┘
                               │
           ┌───────────────────┼───────────────────┐
           │                   │                   │
           ▼                   ▼                   ▼
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│  PodSerializer  │  │CustomFunction-  │  │ MethodSerializer│
│                 │  │   Serializer    │  │                 │
│ (trivially      │  │                 │  │ (методы класса) │
│  copyable)      │  │ (free functions)│  │                 │
└─────────────────┘  └─────────────────┘  └─────────────────┘
        │                    │                    │
        ▼                    ▼                    ▼
┌─────────────────────────────────────────────────────────────────┐
│                        Serializer<T>                             │
│          (базовый интерфейс: Serialize, Deserialize,            │
│                        GetSerializedSize)                        │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Специализации:                                │
│           Serializer<std::string>, Serializer<std::vector<T>>   │
└─────────────────────────────────────────────────────────────────┘
```

---

## Concepts

Модуль использует C++20 concepts для определения способа сериализации типа.

### PodSerializable

Для типов, которые можно записать/прочитать напрямую через `fwrite`/`fread`:

```cpp
template <typename T>
concept PodSerializable = std::is_trivially_copyable_v<T>
                       && std::is_standard_layout_v<T>;
```

**Примеры:** `int`, `double`, `struct { int x, y; }`

### MethodSerializable

Для типов с методами сериализации:

```cpp
template <typename T>
concept MethodSerializable = requires(const T& obj, T& obj_mut, FILE* file) {
    { obj.Serialize(file) } -> std::same_as<bool>;
    { obj_mut.Deserialize(file) } -> std::same_as<bool>;
};
```

### CustomSerializable

Для типов с free-функциями сериализации (через ADL):

```cpp
template <typename T>
concept CustomSerializable = requires(const T& obj, T& obj_mut, FILE* file) {
    { Serialize(obj, file) } -> std::same_as<bool>;
    { Deserialize(obj_mut, file) } -> std::same_as<bool>;
};
```

### SpecializedSerializable

Для типов со специализацией `Serializer<T>`:

```cpp
template <typename T>
concept SpecializedSerializable = Serializer<T>::Specialized::value;
```

### FileSerializable

Объединяющий concept — тип можно сериализовать, если он удовлетворяет любому из вышеперечисленных:

```cpp
template <typename T>
concept FileSerializable = PodSerializable<T>
                        || CustomSerializable<T>
                        || MethodSerializable<T>
                        || SpecializedSerializable<T>;
```

---

## Компоненты

### Serializer<T> (`include/serializers.hpp`)

Базовый интерфейс сериализатора:

```cpp
template <typename T>
class Serializer {
public:
    virtual bool Serialize(const T& obj, FILE* file) = 0;
    virtual bool Deserialize(T& obj, FILE* file) = 0;
    virtual uint64_t GetSerializedSize(const T& obj) = 0;
};
```

### CreateSerializer<T>() (`include/serializers.hpp`)

Фабричная функция — создаёт подходящий сериализатор:

```cpp
template <typename T>
std::unique_ptr<Serializer<T>> CreateSerializer();
```

### Встроенные специализации

| Тип | Формат |
|-----|--------|
| `std::string` | `[uint64_t length][char[] data]` |
| `std::vector<T>` | `[uint64_t size][T[] elements]` |

---

## Сборка

### Bazel target

```python
"//serialization:serialization"
```

### Подключение

```python
cc_binary(
    name = "my_app",
    srcs = ["main.cpp"],
    deps = [
        "//serialization",
    ],
)
```

### Тесты

```bash
bazel test //serialization/...
bazel test //serialization:type_concepts_test
bazel test //serialization:serializing_test
```

### Пример

```bash
bazel run //serialization:example
```

---

## Примеры использования

### 1. POD типы (автоматически)

```cpp
#include "serializers.hpp"

struct Point {
    double x, y, z;
};

int main() {
    auto serializer = serialization::CreateSerializer<Point>();

    Point original{1.0, 2.0, 3.0};

    // Запись
    FILE* file = fopen("point.bin", "wb");
    serializer->Serialize(original, file);
    fclose(file);

    // Чтение
    Point loaded;
    file = fopen("point.bin", "rb");
    serializer->Deserialize(loaded, file);
    fclose(file);

    // loaded == original
    return 0;
}
```

### 2. std::string

```cpp
#include "serializers.hpp"

int main() {
    auto serializer = serialization::CreateSerializer<std::string>();

    std::string original = "Hello, World! Unicode: αβγδ 🚀";

    FILE* file = fopen("string.bin", "wb");
    serializer->Serialize(original, file);
    fclose(file);

    std::string loaded;
    file = fopen("string.bin", "rb");
    serializer->Deserialize(loaded, file);
    fclose(file);

    assert(original == loaded);
    return 0;
}
```

### 3. std::vector<T>

```cpp
#include "serializers.hpp"

int main() {
    auto serializer = serialization::CreateSerializer<std::vector<std::string>>();

    std::vector<std::string> original = {
        "First",
        "Second",
        "Third with emoji 🎉"
    };

    FILE* file = fopen("strings.bin", "wb");
    serializer->Serialize(original, file);
    fclose(file);

    std::vector<std::string> loaded;
    file = fopen("strings.bin", "rb");
    serializer->Deserialize(loaded, file);
    fclose(file);

    assert(original == loaded);
    return 0;
}
```

### 4. Вложенные контейнеры

```cpp
#include "serializers.hpp"

int main() {
    // Вектор векторов целых чисел
    auto serializer = serialization::CreateSerializer<std::vector<std::vector<int>>>();

    std::vector<std::vector<int>> matrix = {
        {1, 2, 3},
        {4, 5, 6, 7},
        {8, 9}
    };

    FILE* file = fopen("matrix.bin", "wb");
    serializer->Serialize(matrix, file);
    fclose(file);

    std::vector<std::vector<int>> loaded;
    file = fopen("matrix.bin", "rb");
    serializer->Deserialize(loaded, file);
    fclose(file);

    assert(matrix == loaded);
    return 0;
}
```

### 5. Custom тип с методами (MethodSerializable)

```cpp
#include "serializers.hpp"

class Person {
public:
    std::string name;
    int32_t age;
    double height;

    // Методы сериализации
    bool Serialize(FILE* file) const {
        // Имя: длина + данные
        uint64_t name_len = name.length();
        if (fwrite(&name_len, sizeof(uint64_t), 1, file) != 1) return false;
        if (fwrite(name.data(), 1, name_len, file) != name_len) return false;

        // Возраст и рост
        if (fwrite(&age, sizeof(int32_t), 1, file) != 1) return false;
        if (fwrite(&height, sizeof(double), 1, file) != 1) return false;

        return true;
    }

    bool Deserialize(FILE* file) {
        uint64_t name_len;
        if (fread(&name_len, sizeof(uint64_t), 1, file) != 1) return false;
        name.resize(name_len);
        if (fread(&name[0], 1, name_len, file) != name_len) return false;

        if (fread(&age, sizeof(int32_t), 1, file) != 1) return false;
        if (fread(&height, sizeof(double), 1, file) != 1) return false;

        return true;
    }

    // ВАЖНО: для производительности добавьте этот метод!
    uint64_t GetSerializedSize() const {
        return sizeof(uint64_t) + name.length()  // имя
             + sizeof(int32_t)                   // возраст
             + sizeof(double);                   // рост
    }

    bool operator==(const Person& other) const {
        return name == other.name && age == other.age && height == other.height;
    }
};

int main() {
    auto serializer = serialization::CreateSerializer<Person>();

    Person alice{"Alice", 30, 165.5};

    FILE* file = fopen("person.bin", "wb");
    serializer->Serialize(alice, file);
    fclose(file);

    Person loaded;
    file = fopen("person.bin", "rb");
    serializer->Deserialize(loaded, file);
    fclose(file);

    assert(alice == loaded);
    return 0;
}
```

### 6. Custom тип с free-функциями (CustomSerializable / ADL)

```cpp
#include "serializers.hpp"

namespace myapp {

struct Record {
    uint64_t id;
    double value;

    bool operator==(const Record& other) const {
        return id == other.id && value == other.value;
    }
};

// Free-функции в том же namespace — найдутся через ADL
bool Serialize(const Record& obj, FILE* file) {
    if (fwrite(&obj.id, sizeof(uint64_t), 1, file) != 1) return false;
    if (fwrite(&obj.value, sizeof(double), 1, file) != 1) return false;
    return true;
}

bool Deserialize(Record& obj, FILE* file) {
    if (fread(&obj.id, sizeof(uint64_t), 1, file) != 1) return false;
    if (fread(&obj.value, sizeof(double), 1, file) != 1) return false;
    return true;
}

}  // namespace myapp

int main() {
    auto serializer = serialization::CreateSerializer<myapp::Record>();

    myapp::Record original{42, 3.14159};

    FILE* file = fopen("record.bin", "wb");
    serializer->Serialize(original, file);
    fclose(file);

    myapp::Record loaded;
    file = fopen("record.bin", "rb");
    serializer->Deserialize(loaded, file);
    fclose(file);

    assert(original == loaded);
    return 0;
}
```

### 7. Специализация Serializer<T>

Для типов из внешних библиотек, где нельзя добавить методы или ADL-функции:

```cpp
#include "serializers.hpp"

// Внешний тип (например, из сторонней библиотеки)
struct ExternalPoint {
    float x, y;
};

// Специализация сериализатора
namespace serialization {

template <>
class Serializer<ExternalPoint> {
public:
    using Specialized = std::true_type;  // Обязательно!

    bool Serialize(const ExternalPoint& obj, FILE* file) {
        return fwrite(&obj, sizeof(ExternalPoint), 1, file) == 1;
    }

    bool Deserialize(ExternalPoint& obj, FILE* file) {
        return fread(&obj, sizeof(ExternalPoint), 1, file) == 1;
    }

    uint64_t GetSerializedSize(const ExternalPoint& obj) {
        return sizeof(ExternalPoint);
    }
};

}  // namespace serialization

int main() {
    auto serializer = serialization::CreateSerializer<ExternalPoint>();

    ExternalPoint p{1.5f, 2.5f};

    FILE* file = fopen("ext_point.bin", "wb");
    serializer->Serialize(p, file);
    fclose(file);

    return 0;
}
```

### 8. Расчёт размера данных

```cpp
#include "serializers.hpp"

int main() {
    auto string_serializer = serialization::CreateSerializer<std::string>();
    auto vector_serializer = serialization::CreateSerializer<std::vector<int>>();

    std::string str = "Hello, World!";
    std::vector<int> vec = {1, 2, 3, 4, 5};

    // Узнать размер ДО записи
    uint64_t str_size = string_serializer->GetSerializedSize(str);
    uint64_t vec_size = vector_serializer->GetSerializedSize(vec);

    std::cout << "String size: " << str_size << " bytes\n";
    // = sizeof(uint64_t) + 13 = 21 bytes

    std::cout << "Vector size: " << vec_size << " bytes\n";
    // = sizeof(uint64_t) + 5 * sizeof(int) = 28 bytes

    return 0;
}
```

---

## Оптимизация производительности

### Проблема: GetSerializedSize() без оптимизации

Для типов **без** метода `GetSerializedSize()` библиотека использует fallback:
- Сериализует объект в `/dev/null` (или `NUL` на Windows)
- Считает записанные байты

Это работает, но **медленно** из-за I/O операций.

### Решение: добавьте GetSerializedSize()

```cpp
struct MyData {
    uint64_t id;
    std::string name;
    std::vector<double> values;

    bool Serialize(FILE* file) const { /* ... */ }
    bool Deserialize(FILE* file) { /* ... */ }

    // ДОБАВЬТЕ ЭТО для производительности!
    uint64_t GetSerializedSize() const {
        uint64_t size = 0;

        size += sizeof(id);                              // id
        size += sizeof(uint64_t) + name.length();        // name (length + data)
        size += sizeof(uint64_t);                        // values.size()
        size += values.size() * sizeof(double);          // values data

        return size;
    }
};
```

### Сравнение производительности

| Подход | Скорость | Использование |
|--------|----------|---------------|
| С `GetSerializedSize()` | **O(1)** или **O(n)** арифметика | Рекомендуется |
| Без (fallback) | **O(n)** + I/O overhead | Только для отладки |

---

## API Reference

### CreateSerializer<T>()

```cpp
template <typename T>
std::unique_ptr<Serializer<T>> CreateSerializer();
```

Создаёт сериализатор для типа `T`. Выбирает реализацию автоматически:

| Concept | Сериализатор |
|---------|--------------|
| `PodSerializable` | `PodSerializer<T>` |
| `CustomSerializable` | `CustomFunctionSerializer<T>` |
| `MethodSerializable` | `MethodSerializer<T>` |
| `SpecializedSerializable` | `Serializer<T>` (специализация) |

### Serializer<T>

| Метод | Описание |
|-------|----------|
| `bool Serialize(const T&, FILE*)` | Сериализовать объект в файл |
| `bool Deserialize(T&, FILE*)` | Десериализовать объект из файла |
| `uint64_t GetSerializedSize(const T&)` | Размер сериализованных данных в байтах |

### Concepts

| Concept | Требования |
|---------|------------|
| `PodSerializable<T>` | `trivially_copyable` + `standard_layout` |
| `MethodSerializable<T>` | Методы `Serialize(FILE*)` и `Deserialize(FILE*)` |
| `CustomSerializable<T>` | Free-функции `Serialize(T, FILE*)` и `Deserialize(T&, FILE*)` |
| `SpecializedSerializable<T>` | Специализация `Serializer<T>` с `Specialized = true_type` |
| `FileSerializable<T>` | Любой из вышеперечисленных |

---

## Бинарный формат

### POD типы

```
┌─────────────────────────┐
│  raw bytes (sizeof(T))  │
└─────────────────────────┘
```

### std::string

```
┌──────────────────────────┐
│ uint64_t: length (N)     │  ← 8 байт
├──────────────────────────┤
│ char[N]: string data     │  ← N байт
└──────────────────────────┘
```

### std::vector<T>

```
┌──────────────────────────┐
│ uint64_t: size (N)       │  ← 8 байт
├──────────────────────────┤
│ T[0] serialized          │
├──────────────────────────┤
│ T[1] serialized          │
├──────────────────────────┤
│ ...                      │
├──────────────────────────┤
│ T[N-1] serialized        │
└──────────────────────────┘
```

---

## Обработка ошибок

- Все методы `Serialize`/`Deserialize` возвращают `bool`
- `false` означает ошибку I/O
- Ошибки логируются через модуль `logging`
- При ошибке состояние объекта и файла не определено

```cpp
auto serializer = CreateSerializer<MyType>();

if (!serializer->Serialize(obj, file)) {
    // Ошибка записи
    std::cerr << "Serialization failed!" << std::endl;
}

if (!serializer->Deserialize(obj, file)) {
    // Ошибка чтения (EOF, corrupted data, etc.)
    std::cerr << "Deserialization failed!" << std::endl;
}
```

---

## Портабельность и ограничения

### Портабельность

| Аспект | Поддержка | Примечание |
|--------|-----------|------------|
| **Endianness** | Не портабельно | Файлы, созданные на little-endian, не читаются на big-endian и наоборот |
| **Размер типов** | Платформо-зависим | `int` может быть 32 или 64 бита. Используйте `int32_t`, `uint64_t` для переносимости |
| **Выравнивание** | Автоматическое | POD типы сериализуются с учётом padding (sizeof) |
| **Компилятор** | GCC, Clang, MSVC | Требуется поддержка C++20 |

### Рекомендации для кроссплатформенности

```cpp
// ПЛОХО: размер зависит от платформы
struct BadRecord {
    int id;           // 32 или 64 бита?
    long value;       // 32, 64 или даже 128 бит?
};

// ХОРОШО: фиксированный размер
struct GoodRecord {
    int64_t id;       // Всегда 64 бита
    uint32_t value;   // Всегда 32 бита
};
```

### Версионирование формата

Библиотека **не поддерживает** версионирование формата автоматически. При изменении структуры типа:

1. Старые файлы станут несовместимы
2. Десериализация может привести к undefined behavior

**Решение:** добавьте версию в свой формат вручную:

```cpp
struct VersionedData {
    static constexpr uint32_t FORMAT_VERSION = 1;

    uint32_t version = FORMAT_VERSION;
    // ... остальные поля ...

    bool Serialize(FILE* file) const {
        if (fwrite(&version, sizeof(version), 1, file) != 1) return false;
        // ... сериализация остальных полей ...
        return true;
    }

    bool Deserialize(FILE* file) {
        if (fread(&version, sizeof(version), 1, file) != 1) return false;
        if (version != FORMAT_VERSION) {
            // Обработка старой версии или ошибка
            return false;
        }
        // ... десериализация остальных полей ...
        return true;
    }
};
```

### Известные ограничения

- **Указатели:** Нельзя сериализовать указатели напрямую
- **Полиморфизм:** Виртуальные классы требуют ручной реализации с type tags
- **Циклические ссылки:** Не поддерживаются
- **Большие объекты:** Весь объект должен помещаться в память

---

## Лицензия

MIT License. См. корневой файл LICENSE проекта.
