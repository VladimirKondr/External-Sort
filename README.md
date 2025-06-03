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
- **Цветная система отладки** - встроенное логирование с цветовой индикацией
- **Полное тестовое покрытие** - comprehensive unit tests с Google Test

##  Требования

- **C++20** compatible compiler (GCC 10+, Clang 12+, MSVC 19.29+)
- **Bazel** 6.0+ для сборки
- **GoogleTest** (автоматически подтягивается Bazel)

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
```

### Покрытие тестами

Проект включает comprehensive тесты для:
-  ElementBuffer - буферизация элементов
-  FileInputStream/FileOutputStream - файловый ввод-вывод
-  InMemoryStream - потоки в памяти
-  TempFileManager - управление временными файлами
-  KWayMergeSorter - основной алгоритм сортировки
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

# Через Bazel (использует ваши конфиги .clang-tidy)
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

##  Лицензия

Этот проект лицензирован под MIT License - см. файл [LICENSE](LICENSE) для деталей.