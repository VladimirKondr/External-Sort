#!/bin/bash

# Скрипт для автоматического форматирования C++ файлов с помощью clang-format
# Использование: ./scripts/format-code.sh [--fix] [--file <файл>] [--verbose] [--help]

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

# Настройки по умолчанию
FIX_MODE=false
TARGET_FILE=""
VERBOSE=false

show_help() {
    echo "Скрипт автоматического форматирования C++ кода"
    echo ""
    echo "Использование:"
    echo "  $0                           - проверить форматирование всех файлов"
    echo "  $0 --fix                     - исправить форматирование всех файлов"
    echo "  $0 --file <путь>             - проверить конкретный файл"
    echo "  $0 --fix --file <путь>       - исправить конкретный файл"
    echo "  $0 --verbose                 - показать подробные ошибки форматирования"
    echo "  $0 --help                    - показать эту справку"
    echo ""
    echo "Поддерживаемые файлы: .cpp, .hpp, .c, .h, .tpp"
    echo ""
    echo "Примеры:"
    echo "  $0                                              # показать файлы с ошибками"
    echo "  $0 --fix                                        # исправить все файлы"
    echo "  $0 --file src/main.cpp                          # проверить файл"
    echo "  $0 --fix --file src/main.cpp                    # исправить файл"
    echo "  $0 --verbose                                     # подробный вывод ошибок"
    echo "  $0 --fix --file include/debug_logger.hpp --verbose"
}

check_clang_format() {
    if ! command -v clang-format &> /dev/null; then
        echo -e "${RED}Ошибка: clang-format не найден${NC}"
        echo "Установите: brew install llvm"
        exit 1
    fi
}

format_file() {
    local file="$1"
    local fix_mode="$2"
    
    if [[ ! -f "$file" ]]; then
        echo -e "${RED}Ошибка: файл '$file' не найден${NC}"
        return 1
    fi
    
    case "$file" in
        *.cpp|*.hpp|*.c|*.h|*.tpp)
            ;;
        *)
            if [[ "$VERBOSE" == true ]]; then
                echo -e "${YELLOW}Пропуск: '$file' (неподдерживаемое расширение)${NC}"
            fi
            return 0
            ;;
    esac
    
    if [[ "$VERBOSE" == true ]]; then
        echo -e "${BLUE}Обработка: $file${NC}"
    fi
    
    if [[ "$fix_mode" == true ]]; then
        if clang-format -i --style=file "$file" 2>/dev/null; then
            if [[ "$VERBOSE" == true ]]; then
                echo -e "${GREEN}  ✓ Отформатирован${NC}"
            fi
            return 0
        else
            echo -e "${RED}  ✗ Ошибка форматирования: $file${NC}"
            return 1
        fi
    else
        local format_output
        format_output=$(clang-format --dry-run --Werror --style=file "$file" 2>&1)
        local format_result=$?
        
        if [[ $format_result -eq 0 ]]; then
            if [[ "$VERBOSE" == true ]]; then
                echo -e "${GREEN}  ✓ Форматирование корректное${NC}"
            fi
            return 0
        else
            echo -e "${RED}  ✗ Требуется форматирование: $file${NC}"
            if [[ "$VERBOSE" == true && -n "$format_output" ]]; then
                echo -e "${YELLOW}    Детали:${NC}"
                echo "$format_output" | sed 's/^/      /'
            fi
            return 1
        fi
    fi
}

format_all_files() {
    local fix_mode="$1"
    local total_files=0
    local processed_files=0
    local error_files=0
    
    if [[ "$fix_mode" == true ]]; then
        echo -e "${YELLOW}Исправление форматирования всех файлов...${NC}"
    else
        echo -e "${YELLOW}Проверка форматирования всех файлов...${NC}"
    fi
    
    local files=(
        $(find . -name "*.cpp" -not -path "./bazel-*" -not -path "./external/*" 2>/dev/null)
        $(find . -name "*.hpp" -not -path "./bazel-*" -not -path "./external/*" 2>/dev/null)
        $(find . -name "*.c" -not -path "./bazel-*" -not -path "./external/*" 2>/dev/null)
        $(find . -name "*.h" -not -path "./bazel-*" -not -path "./external/*" 2>/dev/null)
        $(find . -name "*.tpp" -not -path "./bazel-*" -not -path "./external/*" 2>/dev/null)
    )
    
    total_files=${#files[@]}
    
    if [[ $total_files -eq 0 ]]; then
        echo -e "${YELLOW}C++ файлы не найдены${NC}"
        return 0
    fi
    
    echo -e "${BLUE}Найдено файлов: $total_files${NC}"
    
    for file in "${files[@]}"; do
        if format_file "$file" "$fix_mode"; then
            ((processed_files++))
        else
            ((error_files++))
        fi
    done
    
    echo ""
    echo -e "${BLUE}Результат:${NC}"
    echo -e "  Всего файлов: $total_files"
    echo -e "  Обработано: $processed_files"
    
    if [[ $error_files -gt 0 ]]; then
        echo -e "  ${RED}С ошибками: $error_files${NC}"
        return 1
    else
        echo -e "  ${GREEN}Все файлы успешно обработаны${NC}"
        return 0
    fi
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --file)
            TARGET_FILE="$2"
            shift 2
            ;;
        --fix)
            FIX_MODE=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --help|-h)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}Неизвестный аргумент: $1${NC}"
            echo "Используйте --help для справки"
            exit 1
            ;;
    esac
done

check_clang_format

if [[ ! -f ".clang-format" ]]; then
    echo -e "${YELLOW}Предупреждение: файл .clang-format не найден${NC}"
    echo "Будет использован стиль по умолчанию"
fi

if [[ -n "$TARGET_FILE" ]]; then
    if [[ "$FIX_MODE" == true ]]; then
        echo -e "${YELLOW}Исправление форматирования файла: $TARGET_FILE${NC}"
    else
        echo -e "${YELLOW}Проверка форматирования файла: $TARGET_FILE${NC}"
    fi
    
    if format_file "$TARGET_FILE" "$FIX_MODE"; then
        if [[ "$FIX_MODE" == true ]]; then
            echo -e "${GREEN}Файл успешно отформатирован${NC}"
        else
            echo -e "${GREEN}Форматирование корректное${NC}"
        fi
        exit 0
    else
        exit 1
    fi
else
    if format_all_files "$FIX_MODE"; then
        exit 0
    else
        exit 1
    fi
fi
