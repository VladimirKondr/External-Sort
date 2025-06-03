#!/bin/bash

# Скрипт для запуска clang-tidy для анализа и исправления C++ кода
# Использование: ./scripts/run-clang-tidy.sh [--fix] [--auto-fix] [--file <файл>] [--help]

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

show_help() {
    echo -e "${BLUE}Использование:${NC}"
    echo -e "  ${YELLOW}$0${NC}                        - только анализ (показывает только ошибки в файлах проекта)"
    echo -e "  ${YELLOW}$0 --fix${NC}                 - анализ и исправление всех проблем"
    echo -e "  ${YELLOW}$0 --auto-fix${NC}            - исправление только безопасных проблем (автоматически исправляемых)"
    echo -e "  ${YELLOW}$0 --file <файл>${NC}         - анализ конкретного файла"
    echo -e "  ${YELLOW}$0 --fix --file <файл>${NC}   - исправление конкретного файла"
    echo -e "  ${YELLOW}$0 --help${NC}                - показать эту справку"
    echo ""
    echo -e "${CYAN}Примеры:${NC}"
    echo -e "  ${YELLOW}./scripts/run-clang-tidy.sh${NC}                           # Анализ всех файлов"
    echo -e "  ${YELLOW}./scripts/run-clang-tidy.sh --auto-fix${NC}                # Безопасные исправления"
    echo -e "  ${YELLOW}./scripts/run-clang-tidy.sh --file src/main.cpp${NC}       # Анализ одного файла"
    echo -e "  ${YELLOW}./scripts/run-clang-tidy.sh --fix --file include/*.hpp${NC} # Исправление заголовков"
    exit 0
}

for arg in "$@"; do
    if [ "$arg" = "--help" ] || [ "$arg" = "-h" ]; then
        show_help
    fi
done

echo "Запуск clang-tidy анализа..."

if [ ! -f "compile_commands.json" ]; then
    echo -e "${YELLOW}  compile_commands.json не найден, генерируем...${NC}"
    bazel run @hedron_compile_commands//:refresh_all --config=cpp20_dbg
fi

# Список проверок, которые безопасно исправлять автоматически
AUTO_FIX_CHECKS="modernize-use-nullptr,modernize-use-override,modernize-use-auto,modernize-use-emplace,modernize-loop-convert,modernize-use-bool-literals,modernize-redundant-void-arg,modernize-use-equals-default,modernize-use-equals-delete,readability-redundant-member-init,readability-braces-around-statements,readability-container-size-empty,readability-delete-null-pointer,readability-redundant-smartptr-get,performance-faster-string-find,performance-for-range-copy,performance-unnecessary-copy-initialization,performance-unnecessary-value-param,readability-redundant-control-flow,performance-move-const-arg,modernize-use-transparent-functors"

get_clang_tidy_args() {
    local mode="$1"
    local base_args="--config-file=.clang-tidy"
    
    case "$mode" in
        "auto-fix")
            echo "$base_args --checks=-*,$AUTO_FIX_CHECKS --fix"
            ;;
        "fix")
            echo "$base_args --fix"
            ;;
        *)
            echo "$base_args"
            ;;
    esac
}

filter_project_files() {
    grep -E "(^[^:]*\.(cpp|hpp|tpp):|^[[:space:]]*[0-9]+[[:space:]]*warning|^[[:space:]]*[0-9]+[[:space:]]*error|^[[:space:]]*[0-9]+[[:space:]]*note)" | \
    grep -v -E "(^/usr/|^/System/|^/Applications/|external/|bazel-)" || true
}

check_file() {
    local file="$1"
    local context_args="$2"
    local mode="${3:-check}"
    
    echo -e "${BLUE} Проверка: ${file}${NC}"
    
    local clang_tidy_args=$(get_clang_tidy_args "$mode")
    local exit_code=0
    
    if [[ "$file" == *.tpp ]]; then
        if [ "$mode" = "check" ]; then
            local temp_output=$(mktemp)
            if clang-tidy $clang_tidy_args "$file" -- -std=c++20 -I./include -x c++-header 2>&1 > "$temp_output"; then
                exit_code=0
            else
                exit_code=1
            fi
            cat "$temp_output" | filter_project_files
            rm -f "$temp_output"
        else
            clang-tidy $clang_tidy_args "$file" -- -std=c++20 -I./include -x c++-header
            exit_code=$?
        fi
    else
        if [ "$mode" = "check" ]; then
            local temp_output=$(mktemp)
            if clang-tidy $clang_tidy_args "$file" $context_args -- -std=c++20 -I./include 2>&1 > "$temp_output"; then
                exit_code=0
            else
                exit_code=1
            fi
            cat "$temp_output" | filter_project_files
            rm -f "$temp_output"
        else
            clang-tidy $clang_tidy_args "$file" $context_args -- -std=c++20 -I./include
            exit_code=$?
        fi
    fi
    
    if [ $exit_code -eq 0 ] && [ "$mode" = "check" ]; then
        echo -e "${GREEN} ${file} - OK${NC}"
    elif [ "$mode" != "check" ]; then
        if [ "$mode" = "auto-fix" ]; then
            echo -e "${CYAN} ${file} - Автоисправления применены${NC}"
        else
            echo -e "${YELLOW} ${file} - Исправления применены${NC}"
        fi
    else
        echo -e "${RED} ${file} - Найдены проблемы${NC}"
    fi
    
    return $exit_code
}

FIX_MODE=""
TARGET_FILE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --fix)
            FIX_MODE="fix"
            shift
            ;;
        --auto-fix)
            FIX_MODE="auto-fix"
            shift
            ;;
        --file)
            TARGET_FILE="$2"
            shift 2
            ;;
        *)
            echo "Неизвестный аргумент: $1"
            echo "Использование: $0 [--fix|--auto-fix] [--file <путь_к_файлу>]"
            exit 1
            ;;
    esac
done

if [ ! -z "$TARGET_FILE" ]; then
    if [ ! -z "$FIX_MODE" ]; then
        check_file "$TARGET_FILE" "" "$FIX_MODE"
    else
        check_file "$TARGET_FILE" "" "check"
    fi
    exit 0
fi

echo -e "${YELLOW} Проверка header файлов (.hpp)...${NC}"
find ./include -name "*.hpp" -not -path "*/impl/*" | while read -r file; do
    if [ ! -z "$FIX_MODE" ]; then
        check_file "$file" "" "$FIX_MODE"
    else
        check_file "$file" "" "check"
    fi
done

echo -e "${YELLOW} Проверка template implementation файлов (.tpp)...${NC}"
find ./include/impl -name "*.tpp" | while read -r file; do
    if [ ! -z "$FIX_MODE" ]; then
        check_file "$file" "" "$FIX_MODE"
    else
        check_file "$file" "" "check"
    fi
done

if find ./src -name "*.cpp" | grep -q .; then
    echo -e "${YELLOW} Проверка source файлов (.cpp)...${NC}"
    find ./src -name "*.cpp" | while read -r file; do
        if [ ! -z "$FIX_MODE" ]; then
            check_file "$file" "-p compile_commands.json" "$FIX_MODE"
        else
            check_file "$file" "-p compile_commands.json" "check"
        fi
    done
fi

if [ "$FIX_MODE" = "auto-fix" ]; then
    echo -e "${GREEN} Автоматические исправления clang-tidy завершены!${NC}"
    echo -e "${CYAN} Применены только безопасные исправления. Для полного исправления используйте --fix${NC}"
elif [ "$FIX_MODE" = "fix" ]; then
    echo -e "${GREEN} Исправления clang-tidy завершены!${NC}"
else
    echo -e "${GREEN} Анализ clang-tidy завершен! (показаны только ошибки в файлах проекта)${NC}"
    echo -e "${CYAN} Для автоматических исправлений используйте --auto-fix${NC}"
fi
