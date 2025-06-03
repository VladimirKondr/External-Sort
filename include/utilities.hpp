/**
 * @file utilities.hpp
 * @brief Утилитные функции для тестирования и вспомогательных операций
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include "debug_logger.hpp"
#include "interfaces.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace external_sort {

/**
 * @brief Создает тестовые данные в указанном хранилище
 *
 * Функция заполняет хранилище тестовыми данными заданного размера.
 * Данные могут быть случайными или упорядоченными по убыванию.
 *
 * @tparam T Тип элементов данных
 * @param factory Ссылка на фабрику потоков
 * @param id Идентификатор хранилища для создания данных
 * @param num_elements Количество элементов для создания
 * @param random_data Флаг генерации случайных данных (по умолчанию true)
 *
 * @throws std::runtime_error если не удается создать выходной поток
 *
 * @note При random_data=false создает данные в порядке убывания: num_elements-1, num_elements-2,
 * ..., 0
 * @note При random_data=true создает случайные данные в диапазоне [0, num_elements*10]
 *
 * @example
 * @code
 * FileStreamFactory<uint64_t> factory("temp_dir");
 * CreateTestDataInStorage(factory, "test_input.bin", 1000, true);
 * @endcode
 */
template <typename T>
void CreateTestDataInStorage(
    IStreamFactory<T>& factory, const StorageId& id, uint64_t num_elements,
    bool random_data = true);

/**
 * @brief Проверяет корректность сортировки данных в хранилище
 *
 * Функция читает данные из хранилища и проверяет, что они упорядочены
 * в соответствии с заданным направлением сортировки.
 *
 * @tparam T Тип элементов данных
 * @param factory Ссылка на фабрику потоков
 * @param id Идентификатор хранилища для проверки
 * @param ascending Направление сортировки для проверки (по умолчанию true)
 * @return true, если данные корректно отсортированы, иначе false
 *
 * @throws std::runtime_error если не удается открыть поток для чтения
 *
 * @note Пустое хранилище считается корректно отсортированным
 * @note При ascending=true проверяет порядок по возрастанию
 * @note При ascending=false проверяет порядок по убыванию
 *
 * @example
 * @code
 * FileStreamFactory<uint64_t> factory("temp_dir");
 * bool is_sorted = VerifySortedStorage(factory, "sorted_output.bin", true);
 * @endcode
 */
template <typename T>
bool VerifySortedStorage(IStreamFactory<T>& factory, const StorageId& id, bool ascending = true);

/**
 * @brief Выполняет полный тест сортировки с измерением времени
 *
 * Функция создает тестовые данные, выполняет сортировку и проверяет результат.
 * Включает очистку временных данных и вывод результатов.
 *
 * @param test_name Название теста для вывода
 * @param factory Ссылка на фабрику потоков uint64_t
 * @param input_id Идентификатор входного хранилища
 * @param output_id Идентификатор выходного хранилища
 * @param num_elements Количество элементов для сортировки
 * @param memory_limit Лимит памяти для сортировки в байтах
 * @param k_degree Степень k-путевого слияния
 * @param io_buffer_elems Размер буфера ввода-вывода в элементах
 * @param ascending Направление сортировки
 *
 * @note Функция автоматически очищает входные и выходные данные
 * @note Выводит подробную информацию о ходе выполнения теста
 * @note Измеряет и выводит время выполнения сортировки
 *
 * @example
 * @code
 * FileStreamFactory<uint64_t> factory("temp_dir");
 * run_sort_test("Large File Sort", factory, "input.bin", "output.bin",
 *               100000, 64*1024, 4, 256, true);
 * @endcode
 */
void run_sort_test(
    const std::string& test_name, IStreamFactory<uint64_t>& factory, const StorageId& input_id,
    const StorageId& output_id, uint64_t num_elements, uint64_t memory_limit, uint64_t k_degree,
    uint64_t io_buffer_elems, bool ascending);

#include "impl/utilities.tpp"

}  // namespace external_sort
