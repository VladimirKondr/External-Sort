/**
 * @file utilities.cpp
 * @brief Реализация утилитных функций, не являющихся шаблонными
 * @author External Sort Library
 * @version 1.0
 */

#include "utilities.hpp"

#include "k_way_merge_sorter.hpp"
#include "storage_types.hpp"

#include <chrono>
#include <iostream>

namespace external_sort {

void run_sort_test(
    const std::string& test_name, IStreamFactory<uint64_t>& factory, const StorageId& input_id,
    const StorageId& output_id, uint64_t num_elements, uint64_t memory_limit, uint64_t k_degree,
    uint64_t io_buffer_elems, bool ascending) {
    std::cout << "\n--- Running Test: " << test_name << " ---" << std::endl;

    if (factory.StorageExists(input_id)) {
        factory.DeleteStorage(input_id);
    }
    if (factory.StorageExists(output_id)) {
        factory.DeleteStorage(output_id);
    }

    std::cout << "Creating test data in '" << input_id << "'..." << std::endl;
    CreateTestDataInStorage<uint64_t>(factory, input_id, num_elements, true);

    std::cout << "Starting sort for '" << input_id << "' -> '" << output_id << "'..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();

    KWayMergeSorter<uint64_t> sorter(
        factory, input_id, output_id, memory_limit, k_degree, io_buffer_elems, ascending);
    sorter.Sort();

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end_time - start_time;
    std::cout << "Sort completed in " << diff.count() << " seconds." << std::endl;

    std::cout << "Verifying sorted output '" << output_id << "'..." << std::endl;
    if (VerifySortedStorage<uint64_t>(factory, output_id, ascending)) {
        std::cout << "Output '" << output_id << "' is correctly sorted." << std::endl;
    } else {
        std::cout << "ERROR: Output '" << output_id << "' is NOT correctly sorted." << std::endl;
    }

    factory.DeleteStorage(input_id);
    factory.DeleteStorage(output_id);
}

}  // namespace external_sort
