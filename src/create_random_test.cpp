/**
 * @file create_random_test.cpp
 * @brief Utility to generate test data for external sort algorithm
 */

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {
/**
 * @brief Writes a uint64_t value to an output stream in binary format
 * @param out Output stream to write to
 * @param num Value to write
 */
void WriteNum(std::ostream& out, uint64_t num) {
    out.write(reinterpret_cast<char*>(&num), sizeof(uint64_t));
}

/**
 * @brief Generates a vector of random uint64_t values
 * @param n Number of values to generate
 * @param min_val Minimum value in range (inclusive)
 * @param max_val Maximum value in range (inclusive)
 * @return Vector containing the generated values
 */
std::vector<uint64_t> GenerateRandomVector(
    int n, uint64_t min_val = 1, uint64_t max_val = 100) {
    std::vector<uint64_t> vec(n);
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(min_val, max_val);

    for (int i = 0; i < n; ++i) {
        vec[i] = dis(gen);
    }

    return vec;
}

/**
 * @brief Shuffles a vector in-place
 * @param vec Vector to shuffle
 */
void ShuffleVector(std::vector<uint64_t>& vec) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(vec.begin(), vec.end(), gen);
}
}  // namespace

/**
 * @brief Main entry point for test data generation
 * @param argc Number of command-line arguments
 * @param argv Array of command-line arguments
 * @return 0 on success, non-zero on error
 */
int main(int argc, char* argv[]) {
    std::string output_filename = "input.bin";
    int num_elements = 1'000'000;
    uint64_t min_val = 1;
    uint64_t max_val = 1'000'000;
    bool shuffle = true;
    
    if (argc > 1) output_filename = argv[1];
    if (argc > 2) {
        try {
            num_elements = std::stoi(argv[2]);
        } catch (const std::exception& e) {
            std::cerr << "Invalid number of elements: " << argv[2] << std::endl;
            return 1;
        }
    }
    if (argc > 3) {
        try {
            min_val = std::stoull(argv[3]);
        } catch (const std::exception& e) {
            std::cerr << "Invalid minimum value: " << argv[3] << std::endl;
            return 1;
        }
    }
    if (argc > 4) {
        try {
            max_val = std::stoull(argv[4]);
        } catch (const std::exception& e) {
            std::cerr << "Invalid maximum value: " << argv[4] << std::endl;
            return 1;
        }
    }
    
    try {
        std::ofstream out(output_filename, std::ios::binary);
        if (!out) {
            std::cerr << "Error: Could not open file " << output_filename << " for writing." << std::endl;
            return 1;
        }
        
        std::vector<uint64_t> data = GenerateRandomVector(num_elements, min_val, max_val);
        if (shuffle) {
            ShuffleVector(data);
        }

        uint64_t n = data.size();
        WriteNum(out, n);
        
        for (uint64_t val : data) {
            WriteNum(out, val);
        }

        out.close();
        std::cout << "Created " << output_filename << " with " << n << " numbers in range [" 
                  << min_val << ", " << max_val << "]" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
