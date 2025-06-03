/**
 * @file storage_types.hpp
 * @brief Основные типы и макросы для системы внешней сортировки
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include "debug_logger.hpp"

#include <string>

/**
 * @namespace external_sort
 * @brief Пространство имён для системы внешней сортировки
 */
namespace external_sort {

/**
 * @brief Тип для идентификации хранилища данных (например, имя файла или ключ в map)
 */
using StorageId = std::string;

}  // namespace external_sort
