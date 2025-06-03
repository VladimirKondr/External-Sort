/**
 * @file debug_logger.hpp
 * @brief Система цветного логирования для отладки
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

namespace external_sort::debug {

/**
 * @brief ANSI цветовые коды для терминала
 */
namespace colors {
constexpr const char* kReset = "\033[0m";
constexpr const char* kGreen = "\033[32m";
constexpr const char* kYellow = "\033[33m";
constexpr const char* kRed = "\033[31m";
constexpr const char* kBlue = "\033[34m";
constexpr const char* kCyan = "\033[36m";
}  // namespace colors

/**
 * @brief Уровни логирования
 */
enum class LogLevel : std::uint8_t { INFO, SUCCESS, WARNING, ERROR, DBG };

/**
 * @brief Получить цветовой код для уровня логирования
 */
inline const char* GetColorCode(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:
        case LogLevel::SUCCESS:
            return colors::kGreen;
        case LogLevel::WARNING:
            return colors::kYellow;
        case LogLevel::ERROR:
            return colors::kRed;
        case LogLevel::DBG:
            return colors::kCyan;
        default:
            return colors::kReset;
    }
}

/**
 * @brief Получить префикс для уровня логирования
 */
inline const char* GetLevelPrefix(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:
            return "[INFO]";
        case LogLevel::SUCCESS:
            return "[SUCCESS]";
        case LogLevel::WARNING:
            return "[WARNING]";
        case LogLevel::ERROR:
            return "[ERROR]";
        case LogLevel::DBG:
            return "[DEBUG]";
        default:
            return "[LOG]";
    }
}

/**
 * @brief Удалить GCC суффикс " [with" из имени функции
 */
inline void RemoveGccSuffix(std::string& func_str) {
    const uint64_t gcc_suffix_pos = func_str.find(" [with");
    if (gcc_suffix_pos != std::string::npos) {
        func_str.erase(gcc_suffix_pos);
    }
}

/**
 * @brief Удалить префикс до последнего пробела, если после него есть скобка
 */
inline void RemovePrefixBeforeLastSpace(std::string& func_str) {
    const uint64_t space_pos = func_str.find_last_of(' ');
    if (space_pos != std::string::npos) {
        const uint64_t paren_pos_check = func_str.find('(', space_pos);
        if (paren_pos_check != std::string::npos) {
            func_str = func_str.substr(space_pos + 1);
        }
    }
}

/**
 * @brief Удалить аргументы функции (всё после первой скобки)
 */
inline void RemoveFunctionArgs(std::string& func_str) {
    const uint64_t paren_pos = func_str.find('(');
    if (paren_pos != std::string::npos) {
        func_str = func_str.substr(0, paren_pos);
    }
}

/**
 * @brief Удалить простые шаблонные параметры
 */
inline void RemoveSimpleTemplateParams(std::string& func_str) {
    const uint64_t template_pos = func_str.rfind('<');
    const uint64_t scope_pos = func_str.rfind("::");

    if (template_pos == std::string::npos) {
        return;
    }

    if (scope_pos != std::string::npos && template_pos <= scope_pos) {
        return;
    }

    const uint64_t closing_template_pos = func_str.rfind('>');
    if (closing_template_pos == std::string::npos || closing_template_pos <= template_pos) {
        return;
    }

    for (uint64_t i = template_pos + 1; i < closing_template_pos; ++i) {
        if (func_str[i] == '<' || func_str[i] == '>') {
            return;
        }
    }

    func_str = func_str.substr(0, template_pos);
}

/**
 * @brief Извлечь имя функции без шаблонных параметров и аргументов
 */
inline std::string ExtractFunctionName(const char* func) {
    std::string func_str(func);

    RemoveGccSuffix(func_str);
    RemovePrefixBeforeLastSpace(func_str);
    RemoveFunctionArgs(func_str);
    RemoveSimpleTemplateParams(func_str);

    return func_str;
}

}  // namespace external_sort::debug

#ifdef DEBUG
/**
 * @brief Цветной макрос для логирования с указанием уровня и места вызова
 * @param level Уровень логирования (LogLevel)
 * @param x Выражение для вывода
 */
#define DEBUG_COUT_LEVEL(level, x)                                                        \
    std::cout << external_sort::debug::GetColorCode(level)                                \
              << external_sort::debug::GetLevelPrefix(level) << " "                       \
              << " in " << external_sort::debug::ExtractFunctionName(__PRETTY_FUNCTION__) \
              << "()] " << x << external_sort::debug::colors::RESET

/**
 * @brief Макросы для конкретных уровней логирования
 */
#define DEBUG_COUT_INFO(x) DEBUG_COUT_LEVEL(external_sort::debug::LogLevel::INFO, x)
#define DEBUG_COUT_SUCCESS(x) DEBUG_COUT_LEVEL(external_sort::debug::LogLevel::SUCCESS, x)
#define DEBUG_COUT_WARNING(x) DEBUG_COUT_LEVEL(external_sort::debug::LogLevel::WARNING, x)
#define DEBUG_COUT_ERROR(x) DEBUG_COUT_LEVEL(external_sort::debug::LogLevel::ERROR, x)
#define DEBUG_COUT_DEBUG(x) DEBUG_COUT_LEVEL(external_sort::debug::LogLevel::DBG, x)

/**
 * @brief Старый макрос DEBUG_COUT теперь соответствует INFO уровню
 */
#define DEBUG_COUT(x) DEBUG_COUT_INFO(x)

#else
/**
 * @brief В release режиме все макросы неактивны
 */
#define DEBUG_COUT_LEVEL(level, x)
#define DEBUG_COUT_INFO(x)
#define DEBUG_COUT_SUCCESS(x)
#define DEBUG_COUT_WARNING(x)
#define DEBUG_COUT_ERROR(x)
#define DEBUG_COUT_DEBUG(x)
#define DEBUG_COUT(x)
#endif
