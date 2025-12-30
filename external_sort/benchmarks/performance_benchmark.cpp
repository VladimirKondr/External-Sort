/**
 * @file performance_benchmark.cpp
 * @brief Benchmarks for the external sort library.
 *
 * Measures the performance of KWayMergeSorter under various conditions, including:
 * - Different memory limits for sorting runs.
 * - Different K-way merge degrees.
 * - Varying input file sizes.
 * - Different data distributions (random, sorted, reverse-sorted).
 * - Various data types (uint64_t, std::string, custom structs).
 */

#include <benchmark/benchmark.h>

#include "k_way_merge_sorter.hpp"
#include "file_stream.hpp"
#include "NullLogger.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

/// @brief Test struct with an optimized GetSerializedSize() method.
class WithMethodsAndOptimizedSize {
public:
    uint64_t key;
    std::array<char, 120> payload;

    WithMethodsAndOptimizedSize() : key(0), payload{} {
    }
    WithMethodsAndOptimizedSize(uint64_t k) : key(k) {
        for (size_t i = 0; i < payload.size(); ++i) {
            payload[i] = static_cast<char>(k + i);
        }
    }
    bool Serialize(FILE* file) const {
        if (fwrite(&key, sizeof(key), 1, file) != 1) {
            return false;
        }
        if (fwrite(payload.data(), sizeof(char), payload.size(), file) != payload.size()) {
            return false;
        }
        return true;
    }
    bool Deserialize(FILE* file) {
        if (fread(&key, sizeof(key), 1, file) != 1) {
            return false;
        }
        if (fread(payload.data(), sizeof(char), payload.size(), file) != payload.size()) {
            return false;
        }
        return true;
    }
    uint64_t GetSerializedSize() const {
        return sizeof(key) + payload.size();
    }
    bool operator<(const WithMethodsAndOptimizedSize& other) const {
        return key < other.key;
    }
    bool operator>(const WithMethodsAndOptimizedSize& other) const {
        return key > other.key;
    }
};

/// @brief Test struct without an optimized GetSerializedSize() method.
class WithMethodsNoSizeOptimization {
public:
    uint64_t key;
    std::array<char, 120> payload;

    WithMethodsNoSizeOptimization() : key(0), payload{} {
    }
    WithMethodsNoSizeOptimization(uint64_t k) : key(k) {
        for (size_t i = 0; i < payload.size(); ++i) {
            payload[i] = static_cast<char>(k + i);
        }
    }
    bool Serialize(FILE* file) const {
        if (fwrite(&key, sizeof(key), 1, file) != 1) {
            return false;
        }
        if (fwrite(payload.data(), sizeof(char), payload.size(), file) != payload.size()) {
            return false;
        }
        return true;
    }
    bool Deserialize(FILE* file) {
        if (fread(&key, sizeof(key), 1, file) != 1) {
            return false;
        }
        if (fread(payload.data(), sizeof(char), payload.size(), file) != payload.size()) {
            return false;
        }
        return true;
    }
    bool operator<(const WithMethodsNoSizeOptimization& other) const {
        return key < other.key;
    }
    bool operator>(const WithMethodsNoSizeOptimization& other) const {
        return key > other.key;
    }
};

/// @brief Contains helper functions for generating test data files.
namespace data_gen {

/**
 * @brief Generates a file with a specified number of elements.
 * @tparam T The type of elements to generate.
 * @tparam GeneratorFunc The type of the generator function.
 * @param full_file_path The absolute path to the output file.
 * @param num_elements The number of elements to write.
 * @param gen_func A function that takes a uint64_t index and returns an element of type T.
 */
template <typename T, typename GeneratorFunc>
void GenerateFile(const std::string& full_file_path, uint64_t num_elements,
                  GeneratorFunc gen_func) {
    io::FileOutputStream<T> output_stream(full_file_path, 8192);
    for (uint64_t i = 0; i < num_elements; ++i) {
        output_stream.Write(gen_func(i));
    }
    output_stream.Finalize();
}

auto RndU64(uint64_t) {
    static std::mt19937_64 g(std::random_device{}());
    static std::uniform_int_distribution<uint64_t> d;
    return d(g);
}
auto SrtU64(uint64_t i) {
    return i;
}
auto RevU64(uint64_t i, uint64_t t) {
    return t - i - 1;
}
auto RndStr(uint64_t) {
    static std::mt19937 g(std::random_device{}());
    static std::uniform_int_distribution<int> d(0, 51);
    const std::string c = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string s(16, ' ');
    for (char& x : s) {
        x = c[d(g)];
    }
    return s;
}
template <typename S>
auto RndStruct(uint64_t) {
    static std::mt19937 g(std::random_device{}());
    static std::uniform_int_distribution<uint64_t> d;
    return S(d(g));
}

}  // namespace data_gen

/**
 * @brief A benchmark fixture for external sort tests.
 *
 * Manages the creation and cleanup of test files and directories.
 * Templated on the data type to be sorted.
 * @tparam T The data type for the benchmark.
 */
template <typename T>
class ExternalSortFixture : public benchmark::Fixture {
protected:
    std::filesystem::path base_dir_path_;
    std::filesystem::path temp_dir_path_;
    std::string input_file_path_str_;
    uint64_t num_elements_;

    ExternalSortFixture() {
        logging::SetLogger(std::make_shared<logging::NullLogger>());
        base_dir_path_ = std::filesystem::current_path() / "benchmark_data";
        temp_dir_path_ = base_dir_path_ / "temp_files";
    }

    void SetUp(const benchmark::State& state) override {
        std::filesystem::remove_all(base_dir_path_);
        std::filesystem::create_directories(temp_dir_path_);

        input_file_path_str_ = (base_dir_path_ / "input.bin").string();
        num_elements_ = state.range(0);

        if constexpr (std::is_same_v<T, uint64_t>) {
            if (state.name().find("Sorted") != std::string::npos) {
                data_gen::GenerateFile<uint64_t>(input_file_path_str_, num_elements_,
                                                 data_gen::SrtU64);
            } else if (state.name().find("Reverse") != std::string::npos) {
                data_gen::GenerateFile<uint64_t>(
                    input_file_path_str_, num_elements_,
                    [this](uint64_t i) { return data_gen::RevU64(i, num_elements_); });
            } else {
                data_gen::GenerateFile<uint64_t>(input_file_path_str_, num_elements_,
                                                 data_gen::RndU64);
            }
        } else if constexpr (std::is_same_v<T, std::string>) {
            data_gen::GenerateFile<std::string>(input_file_path_str_, num_elements_,
                                                data_gen::RndStr);
        } else {
            data_gen::GenerateFile<T>(input_file_path_str_, num_elements_, data_gen::RndStruct<T>);
        }
    }

    void TearDown(const benchmark::State&) override {
        std::filesystem::remove_all(base_dir_path_);
    }
};

// Type aliases for fixtures to simplify benchmark registration.
using Uint64Fixture = ExternalSortFixture<uint64_t>;
using StringFixture = ExternalSortFixture<std::string>;
using OptimizedStructFixture = ExternalSortFixture<WithMethodsAndOptimizedSize>;
using NonOptimizedStructFixture = ExternalSortFixture<WithMethodsNoSizeOptimization>;

#define RUN_SORT(T, factory, input, output, mem, k, io_buf)                                 \
    external_sort::KWayMergeSorter<T> sorter(factory, input, output, mem, k, io_buf, true); \
    sorter.Sort()

// --- Benchmarks for Sorter Configuration ---

BENCHMARK_DEFINE_F(Uint64Fixture, BM_RamLimit_Random)(benchmark::State& state) {
    const std::string output_path = (base_dir_path_ / "output.bin").string();
    for (auto _ : state) {
        state.PauseTiming();
        io::FileStreamFactory<uint64_t> factory(temp_dir_path_.string());
        state.ResumeTiming();
        RUN_SORT(uint64_t, factory, input_file_path_str_, output_path, state.range(1), 16, 8192);
    }
}
BENCHMARK_REGISTER_F(Uint64Fixture, BM_RamLimit_Random)
    ->Args({10'000'000, 16 << 20})
    ->Args({10'000'000, 64 << 20})
    ->Args({10'000'000, 256 << 20})
    ->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(Uint64Fixture, BM_KDegree_Random)(benchmark::State& state) {
    const std::string output_path = (base_dir_path_ / "output.bin").string();
    for (auto _ : state) {
        state.PauseTiming();
        io::FileStreamFactory<uint64_t> factory(temp_dir_path_.string());
        state.ResumeTiming();
        RUN_SORT(uint64_t, factory, input_file_path_str_, output_path, 64 << 20, state.range(1),
                 8192);
    }
}
BENCHMARK_REGISTER_F(Uint64Fixture, BM_KDegree_Random)
    ->Args({10'000'000, 2})
    ->Args({10'000'000, 8})
    ->Args({10'000'000, 32})
    ->Args({10'000'000, 128})
    ->Unit(benchmark::kMillisecond);

// --- Benchmarks for Workload Characteristics ---

constexpr uint64_t kOptimalMem = 128 << 20;
constexpr uint64_t kOptimalK = 16;
constexpr uint64_t kOptimalIOBuf = 8192;

BENCHMARK_DEFINE_F(Uint64Fixture, BM_FileSize_Random)(benchmark::State& state) {
    const std::string output_path = (base_dir_path_ / "output.bin").string();
    for (auto _ : state) {
        state.PauseTiming();
        io::FileStreamFactory<uint64_t> factory(temp_dir_path_.string());
        state.ResumeTiming();
        RUN_SORT(uint64_t, factory, input_file_path_str_, output_path, kOptimalMem, kOptimalK,
                 kOptimalIOBuf);
    }
}
BENCHMARK_REGISTER_F(Uint64Fixture, BM_FileSize_Random)
    ->Arg(5'000'000)
    ->Arg(10'000'000)
    ->Arg(50'000'000)
    ->Arg(100'000'000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(Uint64Fixture, BM_DataDistribution_Sorted)(benchmark::State& state) {
    const std::string output_path = (base_dir_path_ / "output.bin").string();
    for (auto _ : state) {
        state.PauseTiming();
        io::FileStreamFactory<uint64_t> factory(temp_dir_path_.string());
        state.ResumeTiming();
        RUN_SORT(uint64_t, factory, input_file_path_str_, output_path, kOptimalMem, kOptimalK,
                 kOptimalIOBuf);
    }
}
BENCHMARK_REGISTER_F(Uint64Fixture, BM_DataDistribution_Sorted)
    ->Arg(20'000'000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(Uint64Fixture, BM_DataDistribution_Reverse)(benchmark::State& state) {
    const std::string output_path = (base_dir_path_ / "output.bin").string();
    for (auto _ : state) {
        state.PauseTiming();
        io::FileStreamFactory<uint64_t> factory(temp_dir_path_.string());
        state.ResumeTiming();
        RUN_SORT(uint64_t, factory, input_file_path_str_, output_path, kOptimalMem, kOptimalK,
                 kOptimalIOBuf);
    }
}
BENCHMARK_REGISTER_F(Uint64Fixture, BM_DataDistribution_Reverse)
    ->Arg(20'000'000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(Uint64Fixture, BM_DataType_Uint64)(benchmark::State& state) {
    const std::string output_path = (base_dir_path_ / "output.bin").string();
    for (auto _ : state) {
        state.PauseTiming();
        io::FileStreamFactory<uint64_t> factory(temp_dir_path_.string());
        state.ResumeTiming();
        RUN_SORT(uint64_t, factory, input_file_path_str_, output_path, kOptimalMem, kOptimalK,
                 kOptimalIOBuf);
    }
}
BENCHMARK_REGISTER_F(Uint64Fixture, BM_DataType_Uint64)
    ->Arg(5'000'000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(StringFixture, BM_DataType_StdString)(benchmark::State& state) {
    const std::string output_path = (base_dir_path_ / "output.bin").string();
    for (auto _ : state) {
        state.PauseTiming();
        io::FileStreamFactory<std::string> factory(temp_dir_path_.string());
        state.ResumeTiming();
        RUN_SORT(std::string, factory, input_file_path_str_, output_path, kOptimalMem, kOptimalK,
                 kOptimalIOBuf);
    }
}
BENCHMARK_REGISTER_F(StringFixture, BM_DataType_StdString)
    ->Arg(1'000'000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(OptimizedStructFixture, BM_DataType_OptimizedStruct)(benchmark::State& state) {
    const std::string output_path = (base_dir_path_ / "output.bin").string();
    for (auto _ : state) {
        state.PauseTiming();
        io::FileStreamFactory<WithMethodsAndOptimizedSize> factory(temp_dir_path_.string());
        state.ResumeTiming();
        RUN_SORT(WithMethodsAndOptimizedSize, factory, input_file_path_str_, output_path,
                 kOptimalMem, kOptimalK, kOptimalIOBuf);
    }
}
BENCHMARK_REGISTER_F(OptimizedStructFixture, BM_DataType_OptimizedStruct)
    ->Arg(1'000'000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(NonOptimizedStructFixture,
                   BM_DataType_NonOptimizedStruct)(benchmark::State& state) {
    const std::string output_path = (base_dir_path_ / "output.bin").string();
    for (auto _ : state) {
        state.PauseTiming();
        io::FileStreamFactory<WithMethodsNoSizeOptimization> factory(temp_dir_path_.string());
        state.ResumeTiming();
        RUN_SORT(WithMethodsNoSizeOptimization, factory, input_file_path_str_, output_path,
                 kOptimalMem, kOptimalK, kOptimalIOBuf);
    }
}
BENCHMARK_REGISTER_F(NonOptimizedStructFixture, BM_DataType_NonOptimizedStruct)
    ->Arg(1'000'000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
