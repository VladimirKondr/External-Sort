#!/bin/bash

# Скрипт для запуска clang-tidy для анализа и исправления C++ кода
# Использование: ./scripts/run-clang-tidy.sh [--fix] [--file <файл>] [--help]


set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

show_help() {
    echo -e "${BLUE}Использование:${NC}"
    echo -e "  ${YELLOW}$0${NC}                     - только анализ"
    echo -e "  ${YELLOW}$0 --fix${NC}              - анализ и исправление"
    echo -e "  ${YELLOW}$0 --file <файл>${NC}      - анализ конкретного файла"
    echo -e "  ${YELLOW}$0 --fix --file <файл>${NC} - исправление конкретного файла"
    echo -e "  ${YELLOW}$0 --help${NC}             - показать эту справку"
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

check_file() {
    local file="$1"
    local context_args="$2"
    
    echo -e "${BLUE} Проверка: ${file}${NC}"
    
    if [[ "$file" == *.tpp ]]; then
        if clang-tidy "$file" $context_args -- -std=c++20 -I./include -x c++-header 2>/dev/null; then
            echo -e "${GREEN} ${file} - OK${NC}"
            return 0
        else
            echo -e "${RED} ${file} - Найдены проблемы${NC}"
            return 1
        fi
    else
        if clang-tidy "$file" $context_args -- -std=c++20 -I./include 2>/dev/null; then
            echo -e "${GREEN} ${file} - OK${NC}"
            return 0
        else
            echo -e "${RED} ${file} - Найдены проблемы${NC}"
            return 1
        fi
    fi
}

fix_file() {
    local file="$1"
    local context_args="$2"
    
    echo -e "${YELLOW} Исправление: ${file}${NC}"
    
    if [[ "$file" == *.tpp ]]; then
        clang-tidy "$file" $context_args --fix -- -std=c++20 -I./include -x c++-header
    else
        clang-tidy "$file" $context_args --fix -- -std=c++20 -I./include
    fi
}

FIX_MODE=false
TARGET_FILE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --fix)
            FIX_MODE=true
            shift
            ;;
        --file)
            TARGET_FILE="$2"
            shift 2
            ;;
        *)
            echo "Неизвестный аргумент: $1"
            echo "Использование: $0 [--fix] [--file <путь_к_файлу>]"
            exit 1
            ;;
    esac
done

if [ ! -z "$TARGET_FILE" ]; then
    if [ "$FIX_MODE" = true ]; then
        fix_file "$TARGET_FILE" ""
    else
        check_file "$TARGET_FILE" ""
    fi
    exit 0
fi

echo -e "${YELLOW} Проверка header файлов (.hpp)...${NC}"
find ./include -name "*.hpp" -not -path "*/impl/*" | while read -r file; do
    if [ "$FIX_MODE" = true ]; then
        fix_file "$file" ""
    else
        check_file "$file" ""
    fi
done

echo -e "${YELLOW} Проверка template implementation файлов (.tpp)...${NC}"
find ./include/impl -name "*.tpp" | while read -r file; do
    if [ "$FIX_MODE" = true ]; then
        fix_file "$file" ""
    else
        check_file "$file" ""
    fi
done

if find ./src -name "*.cpp" | grep -q .; then
    echo -e "${YELLOW} Проверка source файлов (.cpp)...${NC}"
    find ./src -name "*.cpp" | while read -r file; do
        if [ "$FIX_MODE" = true ]; then
            fix_file "$file" "-p compile_commands.json"
        else
            check_file "$file" "-p compile_commands.json"
        fi
    done
fi

if [ "$FIX_MODE" = true ]; then
    echo -e "${GREEN} Исправления clang-tidy завершены!${NC}"
else
    echo -e "${GREEN} Анализ clang-tidy завершен!${NC}"
fi
