#include "file_stream.hpp"
#include "k_way_merge_sorter.hpp"
#include "storage_types.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace es = external_sort;

const std::string DEFAULT_INPUT_FILE = "input.bin";
const std::string DEFAULT_OUTPUT_FILE = "output.bin";
const uint64_t DEFAULT_MEMORY_LIMIT_MB = 64;
const uint32_t DEFAULT_K_DEGREE = 16;
const uint64_t DEFAULT_IO_BUFFER_ELEMENTS = 1024;
const std::string DEFAULT_TEMP_DIR = "temp_sorting_main_app_cli";

void print_usage(const char* prog_name) {
    std::cerr << "Usage: " << prog_name << " [input_file] [output_file] "
              << "[memory_limit_mb] [k_degree] [io_buffer_elements] [temp_dir]" << std::endl;
    std::cerr << "If arguments are provided, all preceding arguments must also be provided."
              << std::endl;
    std::cerr << "Defaults:" << std::endl;
    std::cerr << "  input_file: " << DEFAULT_INPUT_FILE << std::endl;
    std::cerr << "  output_file: " << DEFAULT_OUTPUT_FILE << std::endl;
    std::cerr << "  memory_limit_mb: " << DEFAULT_MEMORY_LIMIT_MB << std::endl;
    std::cerr << "  k_degree: " << DEFAULT_K_DEGREE << std::endl;
    std::cerr << "  io_buffer_elements: " << DEFAULT_IO_BUFFER_ELEMENTS << std::endl;
    std::cerr << "  temp_dir: " << DEFAULT_TEMP_DIR << std::endl;
}

int main(int argc, char* argv[]) {
    std::string input_file = DEFAULT_INPUT_FILE;
    std::string output_file = DEFAULT_OUTPUT_FILE;
    uint64_t memory_limit_bytes = DEFAULT_MEMORY_LIMIT_MB * 1024 * 1024;
    uint32_t k_degree = DEFAULT_K_DEGREE;
    uint64_t io_buffer_elements = DEFAULT_IO_BUFFER_ELEMENTS;
    std::string temp_dir = DEFAULT_TEMP_DIR;

    if (argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        print_usage(argv[0]);
        return 0;
    }

    if (argc > 1) {
        input_file = argv[1];
    }
    if (argc > 2) {
        output_file = argv[2];
    }
    if (argc > 3) {
        try {
            memory_limit_bytes = static_cast<uint64_t>(std::stoull(argv[3])) * 1024 * 1024;
        } catch (const std::exception& e) {
            std::cerr << "Invalid memory_limit_mb: " << argv[3] << ". Using default." << std::endl;
        }
    }
    if (argc > 4) {
        try {
            k_degree = static_cast<uint32_t>(std::stoul(argv[4]));
        } catch (const std::exception& e) {
            std::cerr << "Invalid k_degree: " << argv[4] << ". Using default." << std::endl;
        }
    }
    if (argc > 5) {
        try {
            io_buffer_elements = static_cast<uint64_t>(std::stoull(argv[5]));
        } catch (const std::exception& e) {
            std::cerr << "Invalid io_buffer_elements: " << argv[5] << ". Using default."
                      << std::endl;
        }
    }
    if (argc > 6) {
        temp_dir = argv[6];
    }

    std::cout << "Starting external sort..." << std::endl;
    std::cout << "  Input file: " << input_file << std::endl;
    std::cout << "  Output file: " << output_file << std::endl;
    std::cout << "  Memory limit: " << (memory_limit_bytes / (1024 * 1024)) << " MB" << std::endl;
    std::cout << "  K-degree: " << k_degree << std::endl;
    std::cout << "  I/O buffer elements: " << io_buffer_elements << std::endl;
    std::cout << "  Temporary directory for factory: " << temp_dir << std::endl;

    try {
        es::FileStreamFactory<uint64_t> file_factory(temp_dir);

        es::StorageId input_storage_id = input_file;
        es::StorageId output_storage_id = output_file;

        es::KWayMergeSorter<uint64_t> sorter(
            file_factory, input_storage_id, output_storage_id, memory_limit_bytes, k_degree,
            io_buffer_elements, true);

        std::cout << "Sorting..." << std::endl;
        sorter.Sort();
        std::cout << "Sorting completed successfully." << std::endl;
        std::cout << "Output written to: " << output_file << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}
