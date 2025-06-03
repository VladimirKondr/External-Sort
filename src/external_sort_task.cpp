/**
 * @file external_sort_task.cpp
 * @brief Решение задачи внешней сортировки
 * @author External Sort Library
 * @version 1.0
 */

#include "file_stream.hpp"
#include "k_way_merge_sorter.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <algorithm>

namespace es = external_sort;



int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);

    const std::string input_filename = "input.bin";
    const std::string output_filename = "output.bin";
    const std::string temp_input_id = "temp_input.bin";
    const std::string temp_output_id = "temp_output.bin";

    const uint64_t KB = 1024;
    const uint64_t sizeof_element = sizeof(uint64_t);

    const uint64_t TOTAL_MEMORY_LIMIT_BYTES = 1024 * KB;
    const uint64_t ESTIMATED_BASE_OVERHEAD_BYTES = 970 * KB;

    uint64_t total_dynamic_budget =
        TOTAL_MEMORY_LIMIT_BYTES > ESTIMATED_BASE_OVERHEAD_BYTES
            ? TOTAL_MEMORY_LIMIT_BYTES - ESTIMATED_BASE_OVERHEAD_BYTES
            : sizeof_element;

    const uint64_t main_conversion_io_buffer_bytes = std::max(sizeof_element, 2 * KB);
    const uint64_t main_conversion_buffer_elements =
        main_conversion_io_buffer_bytes / sizeof_element;

    uint64_t sorter_internal_budget =
        total_dynamic_budget > main_conversion_io_buffer_bytes
            ? total_dynamic_budget - main_conversion_io_buffer_bytes
            : sizeof_element;

    uint64_t num_elements = 0;
    std::ifstream pre_input_file(input_filename, std::ios::binary);
    if (pre_input_file) {
        pre_input_file.read(reinterpret_cast<char*>(&num_elements), sizeof_element);
        pre_input_file.close();
    }

    uint64_t memory_for_sorter_runs_bytes;
    uint64_t sorter_io_elements_per_buffer;
    uint64_t k_val;

    if (num_elements == 0) {
        memory_for_sorter_runs_bytes = sizeof_element;
        sorter_io_elements_per_buffer = 1;
        k_val = 2;
    } else {
        uint64_t target_b_io = std::max(sizeof_element, sorter_internal_budget / 5);
        target_b_io = std::min(target_b_io, 2 * KB);
        uint64_t sorter_io_buffer_bytes = std::max(sizeof_element, target_b_io);
        sorter_io_elements_per_buffer = sorter_io_buffer_bytes / sizeof_element;
        if (sorter_io_elements_per_buffer == 0) {
            sorter_io_elements_per_buffer = 1;
        }
        sorter_io_buffer_bytes = sorter_io_elements_per_buffer * sizeof_element;

        uint64_t temp_b_run_estimate = sizeof_element;
        if (sorter_internal_budget > 2 * sorter_io_buffer_bytes) {
            temp_b_run_estimate = sorter_internal_budget - 2 * sorter_io_buffer_bytes;
        }
        uint64_t elements_per_run_est =
            std::max(static_cast<uint64_t>(1), temp_b_run_estimate / sizeof_element);
        uint64_t max_runs_est = (num_elements + elements_per_run_est - 1) / elements_per_run_est;

        const uint64_t COST_PER_FILENAME_ID_APPROX = 32;
        uint64_t estimated_m_ids_cost = max_runs_est * COST_PER_FILENAME_ID_APPROX;

        estimated_m_ids_cost = std::min(estimated_m_ids_cost, sorter_internal_budget / 3);

        uint64_t operational_budget_after_ids =
            sorter_internal_budget > estimated_m_ids_cost
                ? sorter_internal_budget - estimated_m_ids_cost
                : sizeof_element;

        if (operational_budget_after_ids > 2 * sorter_io_buffer_bytes) {
            memory_for_sorter_runs_bytes =
                operational_budget_after_ids - 2 * sorter_io_buffer_bytes;
        } else {
            memory_for_sorter_runs_bytes = sizeof_element;
        }
        memory_for_sorter_runs_bytes = std::max(memory_for_sorter_runs_bytes, sizeof_element);

        uint64_t num_concurrent_streams = 0;
        if (sorter_io_buffer_bytes > 0 && operational_budget_after_ids > sorter_io_buffer_bytes) {
            num_concurrent_streams = operational_budget_after_ids / sorter_io_buffer_bytes;
        }
        k_val = num_concurrent_streams > 2 ? num_concurrent_streams - 1 : 2;
    }

#ifndef ONLINE_JUDGE
    std::cout << "--- Calculated Parameters ---" << std::endl;
    std::cout << "Total Dynamic Budget: " << total_dynamic_budget / KB << " KB" << std::endl;
    std::cout << "Main Conv IO Buffer: " << main_conversion_io_buffer_bytes / KB << " KB ("
              << main_conversion_buffer_elements << " elements)" << std::endl;
    std::cout << "Sorter Internal Budget: " << sorter_internal_budget / KB << " KB" << std::endl;
    std::cout << "Num Elements: " << num_elements << std::endl;
    std::cout << "Sorter B_run (Initial Runs): " << memory_for_sorter_runs_bytes / KB << " KB ("
              << memory_for_sorter_runs_bytes / sizeof_element << " elements)" << std::endl;
    std::cout << "Sorter B_io (Stream Buffer): "
              << (sorter_io_elements_per_buffer * sizeof_element) / KB << " KB ("
              << sorter_io_elements_per_buffer << " elements)" << std::endl;
    std::cout << "Sorter k_val (Merge Degree): " << k_val << std::endl;
    std::cout << "--- End Calculated Parameters ---" << std::endl;
#endif

    try {
        es::FileStreamFactory<uint64_t> factory("ts");

#ifndef ONLINE_JUDGE
        if (std::filesystem::exists(temp_input_id)) {
            std::filesystem::remove(temp_input_id);
        }
        if (std::filesystem::exists(temp_output_id)) {
            std::filesystem::remove(temp_output_id);
        }
#endif

        {
            std::ifstream actual_input_file(input_filename, std::ios::binary);

            uint64_t num_elements_check;
            actual_input_file.read(reinterpret_cast<char*>(&num_elements_check), sizeof_element);

            auto temp_in_writer =
                factory.CreateOutputStream(temp_input_id, main_conversion_buffer_elements);
            for (uint64_t i = 0; i < num_elements; ++i) {
                uint64_t element;
                actual_input_file.read(reinterpret_cast<char*>(&element), sizeof_element);
                temp_in_writer->Write(element);
            }
            temp_in_writer->Finalize();
        }

        es::KWayMergeSorter<uint64_t> sorter(
            factory, temp_input_id, temp_output_id, memory_for_sorter_runs_bytes, k_val,
            sorter_io_elements_per_buffer, true);

        sorter.Sort();

        {
            std::ofstream final_output_file(output_filename, std::ios::binary);

            final_output_file.write(reinterpret_cast<const char*>(&num_elements), sizeof_element);

            auto sorted_reader =
                factory.CreateInputStream(temp_output_id, main_conversion_buffer_elements);
            uint64_t elements_written_count = 0;
            while (!sorted_reader->IsExhausted()) {
                final_output_file.write(
                    reinterpret_cast<const char*>(&sorted_reader->Value()), sizeof_element);
                sorted_reader->Advance();
                elements_written_count++;
            }
            if (elements_written_count != num_elements) {
                throw std::runtime_error(
                    "KWayMergeSorter: Mismatch in number of elements written to output file.");
            }
        }

#ifndef ONLINE_JUDGE
        factory.DeleteStorage(temp_input_id);
        factory.DeleteStorage(temp_output_id);
#endif

#ifndef ONLINE_JUDGE
        std::cout << "Successfully sorted " << num_elements << " elements." << std::endl;
#endif

    } catch (const std::exception& e) {
#ifndef ONLINE_JUDGE
        std::cerr << "Error: " << e.what() << std::endl;
#endif
        return 1;
    }

    return 0;
}