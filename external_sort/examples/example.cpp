/**
 * @file external_sort_example.cpp
 * @brief Example usage of the External Sort library
 *
 * This example demonstrates how to use the KWayMergeSorter with file-based storage
 * to sort a large file of integers. The same approach can be used for custom types
 * that support serialization.
 */

#include "SpdlogWrapper.hpp"
#include "k_way_merge_sorter.hpp"
#include "file_stream.hpp"
#include <random>
#include <filesystem>

using external_sort::KWayMergeSorter;
using io::FileStreamFactory;

int main() {

    auto spdlog_logger = std::make_shared<logging::SpdlogWrapper>(
        "spdlog", logging::SpdlogSinkType::Both, "logs.log");
    logging::SetLogger(spdlog_logger);
    auto& logger = logging::detail::GetLoggerInstance();

    // Prepare test directory and file names
    std::string test_dir = "external_sort_example_dir";
    std::filesystem::remove_all(test_dir);
    std::filesystem::create_directory(test_dir);
    FileStreamFactory<int> factory(test_dir);
    std::string input_id = "input_file";
    std::string output_id = "output_file";

    // Generate and write random data to input file
    {
        auto output = factory.CreateOutputStream(input_id, 100);
        std::mt19937 gen(42);
        std::uniform_int_distribution<int> dist(0, 10000);
        for (int i = 0; i < 1000; ++i) {
            output->Write(dist(gen));
        }
        output->Finalize();
    }

    // Sort the file using external sort
    KWayMergeSorter<int> sorter(factory, input_id, output_id, /*memory_bytes=*/sizeof(int) * 100,
                                /*k=*/4, /*buffer_elems=*/50, /*ascending=*/true);
    sorter.Sort();

    // Read and log sorted data
    {
        auto input = factory.CreateInputStream(output_id, 100);
        std::string log_msg = "Sorted data (first 20 elements): ";
        int count = 0;
        while (!input->IsExhausted() && count < 20) {
            log_msg += std::to_string(input->TakeValue()) + " ";
            input->Advance();
            ++count;
        }
        logger->LogInfo(log_msg);
    }

    // Cleanup
    std::filesystem::remove_all(test_dir);
    return 0;
}
