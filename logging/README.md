# Модуль Logging

Гибкая система логирования для C++20 с поддержкой пользовательских логгеров и интеграцией с spdlog.

## Содержание

- [Обзор](#обзор)
- [Архитектура](#архитектура)
- [Компоненты](#компоненты)
- [Сборка](#сборка)
- [Примеры использования](#примеры-использования)
- [API Reference](#api-reference)

---

## Обзор

Модуль предоставляет абстрактный интерфейс логирования, позволяющий:

- Использовать любой логгер через единый интерфейс
- Подключать spdlog "из коробки"
- Создавать собственные логгеры
- Отключать логирование без изменения кода (NullLogger)
- Безопасно работать в многопоточной среде

---

## Архитектура

```
┌─────────────────────────────────────────────────────────┐
│                      Registry                            │
│  (глобальный реестр с thread-safe SetLogger/GetLogger)  │
└─────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│                      ILogger                             │
│  (абстрактный интерфейс: LogInfo, LogWarning, LogError) │
└─────────────────────────────────────────────────────────┘
              ▲                    ▲                    ▲
              │                    │                    │
     ┌────────┴───┐      ┌────────┴───┐      ┌────────┴────────┐
     │ NullLogger │      │SpdlogWrapper│      │ LoggerAdapter<T>│
     │  (no-op)   │      │  (spdlog)   │      │ (custom logger) │
     └────────────┘      └─────────────┘      └─────────────────┘
```

---

## Компоненты

### ILogger (`include/ILogger.hpp`)

Базовый интерфейс для всех логгеров.

```cpp
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void LogInfo(const std::string& message) = 0;
    virtual void LogWarning(const std::string& message) = 0;
    virtual void LogError(const std::string& message) = 0;
};
```

### NullLogger (`include/NullLogger.hpp`)

No-op реализация, которая отбрасывает все сообщения. Используется по умолчанию.

```cpp
class NullLogger : public ILogger {
    void LogInfo(const std::string& message) override { }
    void LogWarning(const std::string& message) override { }
    void LogError(const std::string& message) override { }
};
```

### SpdlogWrapper (`include/SpdlogWrapper.hpp`)

Обёртка над библиотекой [spdlog](https://github.com/gabime/spdlog) с поддержкой PIMPL.

**Типы sink'ов:**

| Тип | Описание |
|-----|----------|
| `SpdlogSinkType::Console` | Вывод в консоль с цветами |
| `SpdlogSinkType::File` | Запись только в файл |
| `SpdlogSinkType::Both` | Консоль + файл одновременно |

### LoggerAdapter (`include/LoggerAdapter.hpp`)

Шаблонный адаптер для подключения любого пользовательского логгера.

**Требования к пользовательскому логгеру:**

```cpp
struct MyLogger {
    void info(const std::string& msg);   // обязательно
    void warn(const std::string& msg);   // обязательно
    void error(const std::string& msg);  // обязательно
};
```

### Registry (`include/Registry.hpp`)

Глобальный реестр для управления текущим логгером. Thread-safe.

**Функции:**

| Функция | Описание |
|---------|----------|
| `SetLogger(logger)` | Установить логгер |
| `SetDefaultLogger()` | Сбросить на NullLogger |
| `detail::GetLoggerInstance()` | Получить текущий логгер |

---

## Сборка

### Bazel targets

```python
# Только интерфейс (ILogger + NullLogger)
"//logging:logging_interface"

# Адаптер для custom логгеров
"//logging:logging_adapter"

# Глобальный реестр
"//logging:logging_registry"

# Всё вместе (рекомендуется)
"//logging:logging"

# spdlog обёртка (требует зависимость @spdlog)
"//logging:spdlog_wrapper"
```

### Подключение в BUILD.bazel

```python
cc_binary(
    name = "my_app",
    srcs = ["main.cpp"],
    deps = [
        "//logging",
        "//logging:spdlog_wrapper",  # если нужен spdlog
    ],
)
```

---

## Примеры использования

### 1. Использование spdlog (рекомендуется)

```cpp
#include "SpdlogWrapper.hpp"
#include "Registry.hpp"

int main() {
    // Логирование в консоль
    auto console_logger = std::make_shared<logging::SpdlogWrapper>(
        "my_app",
        logging::SpdlogSinkType::Console
    );
    logging::SetLogger(console_logger);

    // Использование
    logging::detail::GetLoggerInstance()->LogInfo("Application started");
    logging::detail::GetLoggerInstance()->LogWarning("Low memory");
    logging::detail::GetLoggerInstance()->LogError("Connection failed");

    return 0;
}
```

### 2. Логирование в файл

```cpp
#include "SpdlogWrapper.hpp"
#include "Registry.hpp"

int main() {
    // Только в файл
    auto file_logger = std::make_shared<logging::SpdlogWrapper>(
        "file_logger",
        logging::SpdlogSinkType::File,
        "application.log"
    );
    logging::SetLogger(file_logger);

    logging::detail::GetLoggerInstance()->LogInfo("This goes to file only");

    return 0;
}
```

### 3. Логирование в консоль и файл одновременно

```cpp
#include "SpdlogWrapper.hpp"
#include "Registry.hpp"

int main() {
    auto both_logger = std::make_shared<logging::SpdlogWrapper>(
        "dual_logger",
        logging::SpdlogSinkType::Both,
        "debug.log"
    );
    logging::SetLogger(both_logger);

    // Сообщение появится и в консоли, и в файле
    logging::detail::GetLoggerInstance()->LogInfo("Visible everywhere");

    return 0;
}
```

### 4. Собственный логгер (copyable)

```cpp
#include "Registry.hpp"
#include <iostream>

struct SimpleConsoleLogger {
    void info(const std::string& msg) {
        std::cout << "[INFO] " << msg << std::endl;
    }

    void warn(const std::string& msg) {
        std::cout << "[WARN] " << msg << std::endl;
    }

    void error(const std::string& msg) {
        std::cerr << "[ERROR] " << msg << std::endl;
    }
};

int main() {
    // Передаём по значению — логгер будет скопирован
    logging::SetLogger(SimpleConsoleLogger{});

    logging::detail::GetLoggerInstance()->LogInfo("Hello from custom logger!");

    return 0;
}
```

### 5. Non-copyable логгер через shared_ptr

```cpp
#include "Registry.hpp"
#include <fstream>
#include <mutex>

class ThreadSafeFileLogger {
public:
    explicit ThreadSafeFileLogger(const std::string& filename)
        : file_(filename, std::ios::app) {}

    // Запрещаем копирование
    ThreadSafeFileLogger(const ThreadSafeFileLogger&) = delete;
    ThreadSafeFileLogger& operator=(const ThreadSafeFileLogger&) = delete;

    void info(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        file_ << "[INFO] " << msg << std::endl;
    }

    void warn(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        file_ << "[WARN] " << msg << std::endl;
    }

    void error(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        file_ << "[ERROR] " << msg << std::endl;
    }

private:
    std::ofstream file_;
    std::mutex mutex_;
};

int main() {
    // Non-copyable логгер передаём через shared_ptr
    auto logger = std::make_shared<ThreadSafeFileLogger>("app.log");
    logging::SetLogger(logger);

    logging::detail::GetLoggerInstance()->LogInfo("Thread-safe logging!");

    return 0;
}
```

### 6. Отключение логирования

```cpp
#include "Registry.hpp"

int main() {
    // Вариант 1: Установить NullLogger явно
    logging::SetDefaultLogger();

    // Вариант 2: Передать nullptr
    logging::SetLogger(nullptr);

    // Все вызовы логирования теперь ничего не делают
    logging::detail::GetLoggerInstance()->LogInfo("This is ignored");

    return 0;
}
```

### 7. Создание helper-функций для удобства

```cpp
#include "Registry.hpp"

namespace myapp {

inline void LogInfo(const std::string& msg) {
    logging::detail::GetLoggerInstance()->LogInfo(msg);
}

inline void LogWarning(const std::string& msg) {
    logging::detail::GetLoggerInstance()->LogWarning(msg);
}

inline void LogError(const std::string& msg) {
    logging::detail::GetLoggerInstance()->LogError(msg);
}

}  // namespace myapp

// Использование
int main() {
    myapp::LogInfo("Clean and simple");
    return 0;
}
```

### 8. Наследование от ILogger напрямую

```cpp
#include "ILogger.hpp"
#include "Registry.hpp"
#include <iostream>

class ColoredConsoleLogger : public logging::ILogger {
public:
    void LogInfo(const std::string& message) override {
        std::cout << "\033[32m[INFO]\033[0m " << message << std::endl;
    }

    void LogWarning(const std::string& message) override {
        std::cout << "\033[33m[WARN]\033[0m " << message << std::endl;
    }

    void LogError(const std::string& message) override {
        std::cerr << "\033[31m[ERROR]\033[0m " << message << std::endl;
    }
};

int main() {
    auto logger = std::make_shared<ColoredConsoleLogger>();
    logging::SetLogger(logger);  // Работает напрямую без адаптера

    logging::detail::GetLoggerInstance()->LogInfo("Green text");
    logging::detail::GetLoggerInstance()->LogWarning("Yellow text");
    logging::detail::GetLoggerInstance()->LogError("Red text");

    return 0;
}
```

---

## API Reference

### namespace logging

#### ILogger

| Метод | Описание |
|-------|----------|
| `virtual void LogInfo(const std::string& message)` | Логирование информационного сообщения |
| `virtual void LogWarning(const std::string& message)` | Логирование предупреждения |
| `virtual void LogError(const std::string& message)` | Логирование ошибки |

#### SpdlogWrapper

| Конструктор | Описание |
|-------------|----------|
| `SpdlogWrapper(name, sink_type, filename)` | Создание логгера с указанным sink |

| Параметр | Тип | По умолчанию | Описание |
|----------|-----|--------------|----------|
| `name` | `const std::string&` | `"spdlog"` | Имя логгера |
| `sink_type` | `SpdlogSinkType` | `Console` | Тип вывода |
| `filename` | `const std::string&` | `"logs.log"` | Путь к файлу (для File/Both) |

#### Registry Functions

| Функция | Описание |
|---------|----------|
| `SetLogger(UserLogger&&)` | Установить copyable логгер |
| `SetLogger(std::shared_ptr<UserLogger>)` | Установить non-copyable логгер |
| `SetLogger(std::shared_ptr<ILogger>)` | Установить ILogger-совместимый логгер |
| `SetDefaultLogger()` | Сбросить на NullLogger |
| `detail::GetLoggerInstance()` | Получить текущий `std::shared_ptr<ILogger>` |

---

## Thread Safety

- `SetLogger()` — thread-safe (защищён mutex)
- `GetLoggerInstance()` — возвращает shared_ptr, безопасно для чтения
- `SpdlogWrapper` — thread-safe (spdlog гарантирует)
- Пользовательские логгеры должны сами обеспечивать thread-safety

---

## Best Practices

### Когда использовать какой уровень

| Уровень | Когда использовать | Примеры |
|---------|-------------------|---------|
| **Info** | Важные этапы выполнения, статистика | "Sorting started", "Created 15 runs", "Merge pass 2/3 completed" |
| **Warning** | Нештатные ситуации, которые не блокируют работу | "Large element detected", "Fallback to slow path" |
| **Error** | Ошибки, требующие внимания | "Failed to write temp file", "Serialization error" |

### Рекомендации

1. **Инициализируйте логгер в начале программы:**
   ```cpp
   int main() {
       // Настройка логгера ДО использования библиотеки
       auto logger = std::make_shared<logging::SpdlogWrapper>(...);
       logging::SetLogger(logger);

       // Теперь используйте external_sort, io и т.д.
   }
   ```

2. **Используйте NullLogger для production без логов:**
   ```cpp
   #ifdef NDEBUG
       logging::SetDefaultLogger();  // NullLogger — минимальный overhead
   #else
       logging::SetLogger(debug_logger);
   #endif
   ```

3. **Для высоконагруженных сценариев:**
   - Используйте `SpdlogSinkType::File` вместо `Console` (консольный вывод медленнее)
   - Или отключите логирование полностью через `NullLogger`

4. **Создавайте helper-функции для удобства:**
   ```cpp
   namespace myapp {
       inline void Log(const std::string& msg) {
           logging::detail::GetLoggerInstance()->LogInfo(msg);
       }
   }
   ```

### Производительность

| Логгер | Относительная скорость |
|--------|----------------------|
| NullLogger | Fastest (no-op) |
| SpdlogWrapper (File) | Fast |
| SpdlogWrapper (Console) | Medium |
| SpdlogWrapper (Both) | Slowest |

---

## Лицензия

MIT License. См. корневой файл LICENSE проекта.
