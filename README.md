# External Sort Library

[![C++20](https://img.shields.io/badge/C++-20-blue.svg?style=flat&logo=c%2B%2B)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Bazel](https://img.shields.io/badge/Build-Bazel-76D275.svg)](https://bazel.build/)

Высокопроизводительная библиотека для внешней сортировки больших объемов данных в C++, реализующая k-путевой алгоритм слияния с буферизацией и управлением памятью.

##  Особенности

- **K-путевая внешняя сортировка слиянием** - эффективная сортировка данных, не помещающихся в память
- **Шаблонная архитектура** - поддержка любых сравнимых типов данных
- **Абстракция хранилища** - работа с файлами и in-memory хранилищем через единый интерфейс
- **Управление буферами** - настраиваемая буферизация для оптимизации производительности
- **Автоматическое управление временными файлами** - безопасная очистка промежуточных данных
- **Интеллектуальная сериализация** - поддержка различных типов с автоматическим выбором метода сериализации
- **Цветная система отладки** - встроенное логирование с цветовой индикацией
- **Полное тестовое покрытие** - comprehensive unit tests с Google Test

##  Требования

- **C++20** compatible compiler (GCC 10+, Clang 12+, MSVC 19.29+)
- **Bazel** 6.0+ для сборки
- **GoogleTest** (автоматически подтягивается Bazel)

## Система сериализации

Библиотека поддерживает комплексную систему сериализации с автоматическим выбором оптимального механизма на основе свойств типа. Система включает в себя концепты C++20 для проверки типов во время компиляции, фабричный паттерн для создания сериализаторов и поддержку различных стратегий сериализации.

### Архитектура системы сериализации

#### Концепты типов

Система использует C++20 концепты для классификации типов:

```cpp
// Концепт для POD-типов (Plain Old Data)
template<typename T>
concept PodSerializable = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

// Концепт для типов с методами сериализации
template<typename T>
concept MethodSerializable = requires(T obj, const T const_obj, FILE* file) {
    { const_obj.Serialize(file) } -> std::convertible_to<bool>;
    { obj.Deserialize(file) } -> std::convertible_to<bool>;
};

// Концепт для типов с внешними функциями (ADL)
template<typename T>
concept CustomSerializable = requires(T obj, const T const_obj, FILE* file) {
    { Serialize(const_obj, file) } -> std::convertible_to<bool>;
    { Deserialize(obj, file) } -> std::convertible_to<bool>;
};
```

#### Фабрика сериализаторов

Фабричная функция `create_serializer<T>()` автоматически выбирает подходящий сериализатор:

```cpp
template<typename T>
std::unique_ptr<Serializer<T>> create_serializer() {
    if constexpr (PodSerializable<T>) {
        return std::make_unique<PodSerializer<T>>();
    } else if constexpr (CustomSerializable<T>) {
        return std::make_unique<CustomFunctionSerializer<T>>();
    } else if constexpr (MethodSerializable<T>) {
        return std::make_unique<MethodSerializer<T>>();
    } else {
        static_assert(false, "Type не поддерживает сериализацию");
    }
}
```

### 1. POD-типы (Plain Old Data)

Для тривиально-копируемых типов с стандартной раскладкой памяти сериализация выполняется напрямую с использованием `fwrite`/`fread`. Это самый быстрый метод сериализации:

```cpp
// POD-тип (использует PodSerializer)
struct Vector3D {
    float x, y, z;

    bool operator<(const Vector3D& other) const {
        if (x != other.x) return x < other.x;
        if (y != other.y) return y < other.y;
        return z < other.z;
    }
    
    bool operator==(const Vector3D& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

// Статическая проверка типа
static_assert(PodSerializable<Vector3D>);

// Использование
auto serializer = create_serializer<Vector3D>();
Vector3D point{1.0f, 2.0f, 3.0f};

FILE* file = fopen("data.bin", "wb");
serializer->Serialize(point, file);  // Быстрая POD-сериализация
fclose(file);
```

### 2. Классы с методами сериализации

Для типов с собственными методами сериализации, которые могут содержать сложную логику или переменные данные:

```cpp
class ComplexData {
private:
    std::string name;
    std::vector<int> values;
    double coefficient;

public:
    ComplexData() = default;
    ComplexData(const std::string& n, const std::vector<int>& v, double c)
        : name(n), values(v), coefficient(c) {}
    
    // Методы сериализации должны иметь следующие сигнатуры:
    bool Serialize(FILE* file) const {
        // Сериализация строки
        uint64_t name_size = name.size();
        if (fwrite(&name_size, sizeof(uint64_t), 1, file) != 1) return false;
        if (fwrite(name.data(), sizeof(char), name_size, file) != name_size) return false;
        
        // Сериализация вектора
        uint64_t values_size = values.size();
        if (fwrite(&values_size, sizeof(uint64_t), 1, file) != 1) return false;
        if (values_size > 0) {
            if (fwrite(values.data(), sizeof(int), values_size, file) != values_size) return false;
        }
        
        // Сериализация coefficient
        return fwrite(&coefficient, sizeof(double), 1, file) == 1;
    }
    
    bool Deserialize(FILE* file) {
        // Десериализация строки
        uint64_t name_size;
        if (fread(&name_size, sizeof(uint64_t), 1, file) != 1) return false;
        name.resize(name_size);
        if (name_size > 0) {
            if (fread(&name[0], sizeof(char), name_size, file) != name_size) return false;
        }
        
        // Десериализация вектора
        uint64_t values_size;
        if (fread(&values_size, sizeof(uint64_t), 1, file) != 1) return false;
        values.resize(values_size);
        if (values_size > 0) {
            if (fread(values.data(), sizeof(int), values_size, file) != values_size) return false;
        }
        
        // Десериализация coefficient
        return fread(&coefficient, sizeof(double), 1, file) == 1;
    }
    
    bool operator<(const ComplexData& other) const {
        return name < other.name;
    }
    
    bool operator==(const ComplexData& other) const {
        return name == other.name && values == other.values && coefficient == other.coefficient;
    }
};

// Статическая проверка типа
static_assert(MethodSerializable<ComplexData>);
```

### 3. Типы с внешними функциями сериализации (ADL)

Для типов, которые сериализуются с помощью внешних функций, использующих Argument Dependent Lookup (ADL):

```cpp
namespace my_types {

class Person {
public:
    std::string name;
    int age;
    double height;

    bool operator<(const Person& other) const {
        if (name != other.name) return name < other.name;
        if (age != other.age) return age < other.age;
        return height < other.height;
    }
    
    bool operator==(const Person& other) const {
        return name == other.name && age == other.age && height == other.height;
    }
};

// Функции должны быть определены в том же пространстве имен, 
// что и класс, для работы ADL
bool Serialize(const Person& person, FILE* file) {
    // Сериализация имени
    uint64_t name_size = person.name.size();
    if (fwrite(&name_size, sizeof(uint64_t), 1, file) != 1) return false;
    if (name_size > 0) {
        if (fwrite(person.name.data(), sizeof(char), name_size, file) != name_size) return false;
    }
    
    // Сериализация возраста и роста
    if (fwrite(&person.age, sizeof(int), 1, file) != 1) return false;
    return fwrite(&person.height, sizeof(double), 1, file) == 1;
}

bool Deserialize(Person& person, FILE* file) {
    // Десериализация имени
    uint64_t name_size;
    if (fread(&name_size, sizeof(uint64_t), 1, file) != 1) return false;
    person.name.resize(name_size);
    if (name_size > 0) {
        if (fread(&person.name[0], sizeof(char), name_size, file) != name_size) return false;
    }
    
    // Десериализация возраста и роста
    if (fread(&person.age, sizeof(int), 1, file) != 1) return false;
    return fread(&person.height, sizeof(double), 1, file) == 1;
}

// Пример сложной вложенной структуры
struct Company {
    std::string name;
    std::vector<Person> employees;
    double revenue;

    bool operator<(const Company& other) const {
        return name < other.name;
    }
};

bool Serialize(const Company& company, FILE* file) {
    // Сериализация названия
    uint64_t name_size = company.name.size();
    if (fwrite(&name_size, sizeof(uint64_t), 1, file) != 1) return false;
    if (name_size > 0) {
        if (fwrite(company.name.data(), sizeof(char), name_size, file) != name_size) return false;
    }
    
    // Сериализация сотрудников (используя ADL для Person)
    uint64_t employees_count = company.employees.size();
    if (fwrite(&employees_count, sizeof(uint64_t), 1, file) != 1) return false;
    for (const auto& employee : company.employees) {
        if (!Serialize(employee, file)) return false;  // ADL вызов
    }
    
    return fwrite(&company.revenue, sizeof(double), 1, file) == 1;
}

bool Deserialize(Company& company, FILE* file) {
    // Десериализация названия
    uint64_t name_size;
    if (fread(&name_size, sizeof(uint64_t), 1, file) != 1) return false;
    company.name.resize(name_size);
    if (name_size > 0) {
        if (fread(&company.name[0], sizeof(char), name_size, file) != name_size) return false;
    }
    
    // Десериализация сотрудников
    uint64_t employees_count;
    if (fread(&employees_count, sizeof(uint64_t), 1, file) != 1) return false;
    company.employees.resize(employees_count);
    for (auto& employee : company.employees) {
        if (!Deserialize(employee, file)) return false;  // ADL вызов
    }
    
    return fread(&company.revenue, sizeof(double), 1, file) == 1;
}

} // namespace my_types

// Статическая проверка типа
static_assert(CustomSerializable<my_types::Person>);
static_assert(CustomSerializable<my_types::Company>);
```

### 4. Специализации для стандартных типов

Библиотека содержит встроенные специализации для стандартных типов C++:

```cpp
// Специализация для std::string
template<>
class Serializer<std::string> {
public:
    bool Serialize(const std::string& str, FILE* file) const {
        uint64_t size = str.size();
        if (fwrite(&size, sizeof(uint64_t), 1, file) != 1) return false;
        if (size > 0) {
            return fwrite(str.data(), sizeof(char), size, file) == size;
        }
        return true;
    }
    
    bool Deserialize(std::string& str, FILE* file) {
        uint64_t size;
        if (fread(&size, sizeof(uint64_t), 1, file) != 1) return false;
        str.resize(size);
        if (size > 0) {
            return fread(&str[0], sizeof(char), size, file) == size;
        }
        return true;
    }
};

// Специализация для std::vector<T>
template<typename T>
class Serializer<std::vector<T>> {
private:
    Serializer<T> element_serializer;

public:
    bool Serialize(const std::vector<T>& vec, FILE* file) const {
        uint64_t size = vec.size();
        if (fwrite(&size, sizeof(uint64_t), 1, file) != 1) return false;
        
        for (const auto& element : vec) {
            if (!element_serializer.Serialize(element, file)) return false;
        }
        return true;
    }
    
    bool Deserialize(std::vector<T>& vec, FILE* file) {
        uint64_t size;
        if (fread(&size, sizeof(uint64_t), 1, file) != 1) return false;
        
        vec.resize(size);
        for (auto& element : vec) {
            if (!element_serializer.Deserialize(element, file)) return false;
        }
        return true;
    }
};
```

### Приоритет механизмов сериализации

При наличии нескольких механизмов сериализации для одного типа, выбор происходит в следующем порядке:

1. **POD-сериализация** (для POD-типов) - самая быстрая
2. **Внешние функции** (для типов с функциями `Serialize`/`Deserialize`)
3. **Методы класса** (для типов с методами `Serialize`/`Deserialize`)
4. **Специализации** (для `std::string`, `std::vector<T>`, etc.)

### Обработка ошибок

Все методы сериализации возвращают `bool` для индикации успеха/неудачи:

```cpp
auto serializer = create_serializer<MyType>();
MyType data = /* ... */;

FILE* file = fopen("data.bin", "wb");
if (!file) {
    // Обработка ошибки открытия файла
    return false;
}

if (!serializer->Serialize(data, file)) {
    // Обработка ошибки сериализации
    fclose(file);
    return false;
}

fclose(file);
```

### Тестирование

Библиотека включает в себя комплексное тестирование всех механизмов сериализации:

```bash
# Запуск всех тестов сериализации
bazel test //:serialization_tests

# Запуск отдельных тестов
bazel test //:pod_serialization_test
bazel test //:method_serialization_test  
bazel test //:external_adl_serialization_test
bazel test //:serializers_concepts_validation_test
```

##  Установка и сборка

### Сборка с помощью Bazel

```bash
# Клонирование репозитория
git clone https://github.com/yourusername/external-sort-library.git
cd external-sort-library

# Сборка всех целей
bazel build //...

# Запуск тестов
bazel test //...

# Сборка конкретных приложений
bazel build //:main_app      # CLI приложение
bazel build //:demo          # Демонстрационная программа
bazel build //:external_sort_task  # Решение задачи
```

### Сборка утилит для создания тестовых данных

```bash
bazel build //:create_test_input
bazel build //:create_random_test
```

##  Быстрый старт

### Простой пример использования

```cpp
#include "file_stream.hpp"
#include "k_way_merge_sorter.hpp"

int main() {
    using namespace external_sort;
    
    // Создание фабрики для работы с файлами
    FileStreamFactory<uint64_t> factory("temp_sort_dir");
    
    // Настройка параметров сортировки
    uint64_t memory_limit = 64 * 1024 * 1024;  // 64 MB
    uint64_t k_degree = 16;                     // 16-путевое слияние
    uint64_t buffer_size = 1024;                // размер буфера
    
    // Создание и запуск сортировщика
    KWayMergeSorter<uint64_t> sorter(
        factory, "input.bin", "output.bin", 
        memory_limit, k_degree, buffer_size, true
    );
    
    sorter.Sort();
    return 0;
}
```

### Работа с in-memory хранилищем

```cpp
#include "memory_stream.hpp"
#include "k_way_merge_sorter.hpp"

int main() {
    using namespace external_sort;
    
    // In-memory фабрика для тестирования
    InMemoryStreamFactory<int> factory;
    
    // Создание тестовых данных
    CreateTestDataInStorage(factory, "input", 10000, true);
    
    // Сортировка
    KWayMergeSorter<int> sorter(
        factory, "input", "output", 
        32768,  // memory limit
        4,      // k-degree
        256,    // buffer size
        true    // ascending order
    );
    
    sorter.Sort();
    return 0;
}
```

##  CLI Приложения

### Основное приложение

```bash
# Базовое использование (с параметрами по умолчанию)
./bazel-bin/main_app

# С кастомными параметрами
./bazel-bin/main_app input.bin output.bin 128 16 2048 temp_dir

# Показать справку
./bazel-bin/main_app --help
```

**Параметры:**
- `input_file` - входной файл (по умолчанию: `input.bin`)
- `output_file` - выходной файл (по умолчанию: `output.bin`)
- `memory_limit_mb` - лимит памяти в МБ (по умолчанию: `64`)
- `k_degree` - степень k-путевого слияния (по умолчанию: `16`)
- `io_buffer_elements` - размер буфера в элементах (по умолчанию: `1024`)
- `temp_dir` - директория для временных файлов

### Создание тестовых данных

```bash
# Создание простого тестового файла
./bazel-bin/create_test_input

# Создание случайных данных
./bazel-bin/create_random_test output.bin 1000000 1 100000
```

### Демонстрационная программа

```bash
# Запуск полного набора демонстраций и тестов
./bazel-bin/demo
```

##  Работа с Сериализацией

Библиотека поддерживает различные механизмы сериализации для разных типов данных:

### 1. POD-типы (Plain Old Data)

POD-типы автоматически сериализуются с использованием прямой записи/чтения через fwrite/fread:

```cpp
struct Vector3D {
    float x, y, z;
    
    // Определите операторы сравнения для сортировки
    bool operator<(const Vector3D& other) const {
        if (x != other.x) return x < other.x;
        if (y != other.y) return y < other.y;
        return z < other.z;
    }
};

void sortVector3Ds() {
    FileStreamFactory<Vector3D> factory;
    KWayMergeSorter<Vector3D> sorter(factory, 1024);
    sorter.Sort("input.bin", "output.bin");
}
```

### 2. Типы с методами сериализации

Для сложных типов можно определить методы сериализации:

```cpp
class Point {
public:
    int x, y;
    
    bool operator<(const Point& other) const {
        return (x < other.x || (x == other.x && y < other.y));
    }
    
    // Методы сериализации
    bool Serialize(FILE* file) const {
        fwrite(&x, sizeof(int), 1, file);
        return fwrite(&y, sizeof(int), 1, file) == 1;
    }
    
    bool Deserialize(FILE* file) {
        fread(&x, sizeof(int), 1, file);
        return fread(&y, sizeof(int), 1, file) == 1;
    }
};
```

### 3. Типы с внешними функциями сериализации

Альтернативно, можно определить свободные функции:

```cpp
class Person {
public:
    std::string name;
    int age;
    
    bool operator<(const Person& other) const {
        return (age < other.age || (age == other.age && name < other.name));
    }
};

// Внешние функции сериализации (в том же пространстве имен, что и тип или с ADL)
bool Serialize(const Person& person, FILE* file) {
    uint64_t length = person.name.length();
    fwrite(&length, sizeof(uint64_t), 1, file);
    fwrite(person.name.data(), sizeof(char), length, file);
    return fwrite(&person.age, sizeof(int), 1, file) == 1;
}

bool Deserialize(Person& person, FILE* file) {
    uint64_t length;
    fread(&length, sizeof(uint64_t), 1, file);
    person.name.resize(length);
    fread(&person.name[0], sizeof(char), length, file);
    return fread(&person.age, sizeof(int), 1, file) == 1;
}
```

### 4. Специализация шаблонов для стандартных контейнеров

Библиотека включает встроенную поддержку для `std::string` и `std::vector<T>`:

```cpp
// Для std::string и std::vector<T> уже реализованы специализации
void sortNames() {
    std::vector<std::string> names = {"Charlie", "Alice", "Bob", "David"};
    
    FileStreamFactory<std::string> factory;
    KWayMergeSorter<std::string> sorter(factory, 1024);
    
    // Запись в файл
    {
        FileOutputStream<std::string> output("names.bin", 1024);
        for (const auto& name : names) {
            output.Write(name);
        }
    }
    
    // Сортировка
    sorter.Sort("names.bin", "sorted_names.bin");
}
```

### Выбор метода сериализации

Библиотека автоматически выбирает наиболее подходящий метод сериализации в следующем порядке приоритета:

1. POD-типы → `PodSerializer`
2. Типы с внешними функциями → `CustomFunctionSerializer`
3. Типы с методами → `MethodSerializer`

Для определения подходящего типа используются C++20 концепты из `type_concepts.hpp`.

##  Архитектура

### Основные компоненты

```
external_sort/
├── interfaces.hpp          # Абстрактные интерфейсы
├── k_way_merge_sorter.hpp  # Основной алгоритм сортировки
├── file_stream.hpp         # Файловые потоки ввода-вывода
├── memory_stream.hpp       # In-memory потоки
├── element_buffer.hpp      # Буферизация элементов
├── temp_file_manager.hpp   # Управление временными файлами
├── debug_logger.hpp        # Система логирования
├── serializers.hpp         # Система сериализации
├── type_concepts.hpp       # Концепты для проверки типов (C++20)
└── utilities.hpp           # Утилитные функции
```

### Иерархия классов

```cpp
IStreamFactory<T>
├── FileStreamFactory<T>     // Файловое хранилище
└── InMemoryStreamFactory<T> // In-memory хранилище

IInputStream<T> / IOutputStream<T>
├── FileInputStream<T> / FileOutputStream<T>
└── InMemoryInputStream<T> / InMemoryOutputStream<T>

KWayMergeSorter<T>           // Главный алгоритм сортировки
ElementBuffer<T>             // Система буферизации
TempFileManager              // Управление временными файлами

// Система сериализации
Serializer<T>
├── PodSerializer<T>            // Для POD-типов
├── CustomFunctionSerializer<T> // Для типов с внешними функциями
└── MethodSerializer<T>         // Для типов с методами сериализации

// Концепты типов (C++20)
Sortable<T>                  // Проверка операторов сравнения
FileSerializable<T>          // Проверка возможности сериализации
SupportedFileType<T>         // Проверка полной поддержки типа
```

##  Тестирование

### Запуск всех тестов

```bash
bazel test //... --test_output=all
```

### Конкретные тестовые наборы

```bash
# Тесты буферов
bazel test //:all_tests --test_filter="ElementBuffer*"

# Тесты файловых потоков
bazel test //:all_tests --test_filter="FileStream*"

# Тесты основного алгоритма
bazel test //:all_tests --test_filter="KWayMergeSorter*"

# Тесты сериализации пользовательских типов
bazel test //:test_custom_serialization
```

### Покрытие тестами

Проект включает comprehensive тесты для:
-  ElementBuffer - буферизация элементов
-  FileInputStream/FileOutputStream - файловый ввод-вывод
-  InMemoryStream - потоки в памяти
-  TempFileManager - управление временными файлами
-  KWayMergeSorter - основной алгоритм сортировки
-  Сериализация и поддержка различных типов данных
-  Граничные случаи и обработка ошибок

##  Скрипты разработки

Проект включает набор удобных скриптов для автоматизации задач разработки:

### Форматирование кода (`scripts/format-code.sh`)

```bash
# Проверка форматирования всех файлов
./scripts/format-code.sh

# Автоматическое исправление форматирования
./scripts/format-code.sh --fix

# Работа с конкретным файлом
./scripts/format-code.sh --file include/file_stream.hpp
./scripts/format-code.sh --fix --file src/main.cpp

# Подробный вывод
./scripts/format-code.sh --verbose
```

### Статический анализ (`scripts/run-clang-tidy.sh`)

```bash
# Анализ кода (только ошибки в файлах проекта)
./scripts/run-clang-tidy.sh

# Безопасные автоматические исправления
./scripts/run-clang-tidy.sh --auto-fix

# Исправление всех найденных проблем
./scripts/run-clang-tidy.sh --fix

# Анализ конкретного файла
./scripts/run-clang-tidy.sh --file src/utilities.cpp
```

**Различия между режимами исправления:**
- `--auto-fix`: Исправляет только безопасные проблемы (nullptr, override, auto, range-for, etc.)
- `--fix`: Пытается исправить все найденные проблемы (может требовать ручной проверки)

**Преимущества скриптов:**
- Фильтрация вывода: показывают только ошибки в файлах проекта
- Цветной вывод для лучшей читаемости
- Поддержка работы с отдельными файлами
- Автоматическая генерация `compile_commands.json` при необходимости

##  Настройка и конфигурация

### Отладочный режим

Для включения детального логирования скомпилируйте с флагом `DEBUG`:

```bash
bazel build //... --copt=-DDEBUG
```

### Форматирование кода

```bash
# Форматирование всех файлов
./scripts/format-code.sh --fix

# Проверка форматирования (dry-run)
./scripts/format-code.sh

# Форматирование конкретного файла
./scripts/format-code.sh --fix --file src/main.cpp

# Через Bazel (использует ваши конфиги .clang-format)
bazel run //:format_all
bazel run //:check_format
```

### Статический анализ

```bash
# Анализ кода (показывает только ошибки в файлах проекта)
./scripts/run-clang-tidy.sh

# Автоматическое исправление безопасных проблем
./scripts/run-clang-tidy.sh --auto-fix

# Исправление всех проблем (требует внимания!)
./scripts/run-clang-tidy.sh --fix

# Анализ конкретного файла
./scripts/run-clang-tidy.sh --file src/main.cpp

# Через Bazel (использует ваши конфиги .clang.tidy)
bazel run //:clang_tidy         # Только анализ
bazel run //:clang_tidy_auto_fix # Безопасные исправления
bazel run //:clang_tidy_fix      # Все исправления
```

**Типы проблем, исправляемых автоматически (`--auto-fix`):**
- `modernize-use-nullptr` - замена NULL на nullptr
- `modernize-use-override` - добавление override
- `modernize-use-auto` - использование auto где возможно
- `modernize-loop-convert` - замена на range-based for
- `performance-*` - оптимизации производительности
- `readability-*` - улучшения читаемости кода

##  Производительность

### Алгоритмическая сложность

- **Временная сложность:** O(n log n) где n - количество элементов
- **Пространственная сложность:** O(k * B + M) где:
  - k - степень слияния
  - B - размер буфера
  - M - размер памяти для начальных прогонов

### Рекомендации по настройке

| Размер данных | Память | K-degree | Буфер |
|---------------|--------|----------|-------|
| < 1GB         | 64MB   | 8-16     | 1024  |
| 1-10GB        | 256MB  | 16-32    | 2048  |
| > 10GB        | 1GB+   | 32-64    | 4096  |

## Поддерживаемые типы данных

Библиотека поддерживает сортировку любых типов данных, удовлетворяющих следующим условиям:
- Тип должен быть сравнимым (`operator<`, `operator==`)
- Для файлового режима: тип должен быть тривиально сериализуемым (POD) или иметь специализированную сериализацию

**Из коробки протестированы и поддерживаются:**
- Все стандартные целочисленные типы: `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, `int32_t`, `uint32_t`, `int64_t`, `uint64_t`
- Числа с плавающей точкой: `float`, `double`
- `std::string` (с автоматической сериализацией/десериализацией для файлов)
- Пользовательские структуры (если реализованы операторы сравнения и, при необходимости, сериализация)

Пример пользовательского типа:
```cpp
struct MyStruct {
    int id;
    double value;
    bool operator<(const MyStruct& other) const { return id < other.id; }
    bool operator==(const MyStruct& other) const { return id == other.id && value == other.value; }
};
```

> **Примечание:**
> Для нестандартных/сложных типов, используемых с файловым хранилищем, потребуется реализовать свою сериализацию (см. пример для `std::string` в исходниках).

---

##  Лицензия

Этот проект лицензирован под MIT License - см. файл [LICENSE](LICENSE) для деталей.