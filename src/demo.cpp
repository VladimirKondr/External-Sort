/**
 * @file main.cpp
 * @brief Главная программа для демонстрации и тестирования внешней сортировки
 * @author External Sort Library
 * @version 1.0
 */

#include "../include/debug_logger.hpp"
#include "file_stream.hpp"
#include "memory_stream.hpp"
#include "utilities.hpp"

#include <iostream>

namespace es = external_sort;

/**
 * @brief Главная функция программы
 *
 * Выполняет серию тестов внешней сортировки для демонстрации
 * работы с файловыми и in-memory хранилищами.
 *
 * @param argc Количество аргументов командной строки
 * @param argv Аргументы командной строки
 * @return 0 при успешном выполнении
 */
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    const uint64_t NUM_ELEMENTS_LARGE = 100'000;
    const uint64_t NUM_ELEMENTS_SMALL = 100;
    const uint64_t MEMORY_LIMIT_BYTES = 64 * 1024;
    const uint64_t K_DEGREE = 4;
    const uint64_t IO_BUFFER_ELEMENTS = 256;

    std::cout << "=== External Sort Library Tests ===" << std::endl;
    std::cout << "Large dataset: " << NUM_ELEMENTS_LARGE << " elements" << std::endl;
    std::cout << "Small dataset: " << NUM_ELEMENTS_SMALL << " elements" << std::endl;
    std::cout << "Memory limit: " << MEMORY_LIMIT_BYTES << " bytes" << std::endl;
    std::cout << "K-degree: " << K_DEGREE << std::endl;
    std::cout << "IO buffer: " << IO_BUFFER_ELEMENTS << " elements" << std::endl;

    {
        std::cout << "\n===============================================" << std::endl;
        std::cout << "Testing with File System Storage" << std::endl;
        std::cout << "===============================================" << std::endl;

        es::FileStreamFactory<uint64_t> file_factory("temp_sorting_dir_main");

        es::StorageId input_file = "input_main.bin";
        es::StorageId output_file = "output_main.bin";
        es::run_sort_test(
            "File System Sort (Large)", file_factory, input_file, output_file, NUM_ELEMENTS_LARGE,
            MEMORY_LIMIT_BYTES, K_DEGREE, IO_BUFFER_ELEMENTS, true);

        es::StorageId input_file_small_desc = "input_main_small_desc.bin";
        es::StorageId output_file_small_desc = "output_main_small_desc.bin";
        es::run_sort_test(
            "File System Sort (Small, Descending)", file_factory, input_file_small_desc,
            output_file_small_desc, NUM_ELEMENTS_SMALL, MEMORY_LIMIT_BYTES / 4, 2,
            IO_BUFFER_ELEMENTS / 2, false);

        es::StorageId input_empty = "input_empty.bin";
        es::StorageId output_empty = "output_empty.bin";
        es::run_sort_test(
            "File System Sort (Empty Input)", file_factory, input_empty, output_empty, 0,
            MEMORY_LIMIT_BYTES, K_DEGREE, IO_BUFFER_ELEMENTS, true);
    }
    {
        std::cout << "\n===============================================" << std::endl;
        std::cout << "Testing with In-Memory Storage" << std::endl;
        std::cout << "===============================================" << std::endl;

        es::InMemoryStreamFactory<uint64_t> memory_factory;

        es::StorageId input_mem = "mem_input_large";
        es::StorageId output_mem = "mem_output_large";
        es::run_sort_test(
            "In-Memory Sort (Large)", memory_factory, input_mem, output_mem, NUM_ELEMENTS_LARGE,
            MEMORY_LIMIT_BYTES, K_DEGREE, IO_BUFFER_ELEMENTS, true);

        es::StorageId input_mem_small_desc = "mem_input_small_desc";
        es::StorageId output_mem_small_desc = "mem_output_small_desc";
        es::run_sort_test(
            "In-Memory Sort (Small, Descending)", memory_factory, input_mem_small_desc,
            output_mem_small_desc, NUM_ELEMENTS_SMALL, MEMORY_LIMIT_BYTES / 4, 2,
            IO_BUFFER_ELEMENTS / 2, false);

        auto data_vec = memory_factory.GetStorageData(output_mem_small_desc);
        if (data_vec) {
            DEBUG_COUT_INFO(
                "In-Memory " << output_mem_small_desc << " final size: " << data_vec->size()
                             << ", declared: "
                             << memory_factory.GetStorageDeclaredSize(output_mem_small_desc)
                             << std::endl);
        }

        es::StorageId input_mem_empty = "mem_input_empty";
        es::StorageId output_mem_empty = "mem_output_empty";
        es::run_sort_test(
            "In-Memory Sort (Empty Input)", memory_factory, input_mem_empty, output_mem_empty, 0,
            MEMORY_LIMIT_BYTES, K_DEGREE, IO_BUFFER_ELEMENTS, true);
    }

    std::cout << "\n===============================================" << std::endl;
    std::cout << "All tests completed successfully!" << std::endl;
    std::cout << "===============================================" << std::endl;

    return 0;
}
