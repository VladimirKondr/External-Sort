/**
 * @file k_way_merge_sorter.hpp
 * @brief K-way external merge sort
 */

#pragma once

#include "external_sort_logging.hpp"
#include "interfaces.hpp"
#include "serializers.hpp"
#include "storage_types.hpp"

#include <algorithm>
#include <queue>
#include <vector>

namespace external_sort {

/**
 * @brief Structure representing a data source for merging
 *
 * Used in priority queue for k-way merging.
 *
 * @tparam T Type of data elements
 */
template <typename T>
struct MergeSource {
    io::IInputStream<T>* stream;  ///< Pointer to source stream
};

/**
 * @brief Comparator for merge sources that compares the current value of the streams
 */
template <typename T>
struct MergeSourceComparatorStatic {
    bool ascending;
    explicit MergeSourceComparatorStatic(bool asc) : ascending(asc) {
    }

    bool operator()(const MergeSource<T>& a, const MergeSource<T>& b) const {
        const T& va = a.stream->Value();
        const T& vb = b.stream->Value();
        return ascending ? (va > vb) : (va < vb);
    }
};

/**
 * @brief Template class for performing k-way external merge sort
 *
 * The class is abstracted from specific storage implementation and works
 * through IStreamFactory, io::IInputStream and io::IOutputStream interfaces.
 *
 * @tparam T Type of elements to sort
 */
template <typename T>
class KWayMergeSorter {
private:
    io::IStreamFactory<T>& stream_factory_;  ///< Reference to stream factory
    io::StorageId input_id_;                 ///< Input storage identifier
    io::StorageId output_id_;                ///< Output storage identifier
    uint64_t memory_for_runs_bytes_;         ///< Memory for creating initial runs (bytes)
    uint64_t k_way_degree_;                  ///< Degree of k-way merging
    uint64_t file_io_buffer_elements_;       ///< Size of I/O buffer in elements
    bool ascending_;                         ///< Ascending sort flag

    /**
     * @brief Comparator for merge sources
     */
    struct MergeSourceComparator {
        bool ascending;  ///< Sort direction flag

        /**
         * @brief Comparator constructor
         * @param asc Sort direction (true - ascending)
         */
        explicit MergeSourceComparator(bool asc) : ascending(asc) {
        }

        /**
         * @brief Comparison operator
         * @param a First source
         * @param b Second source
         * @return Comparison result for priority queue
         */
        bool operator()(const MergeSource<T>& a, const MergeSource<T>& b) const {
            const T& va = a.stream->Value();
            const T& vb = b.stream->Value();
            return ascending ? (va > vb) : (va < vb);
        }
    };

    /**
     * @brief Creates initial sorted runs from input stream
     * @return Vector of created run identifiers
     */
    std::vector<io::StorageId> CreateInitialRuns();

    /**
     * @brief Merges a group of runs into one
     * @param group_run_ids Identifiers of runs to merge
     * @param output_run_id Output run identifier
     */
    void MergeGroupOfRuns(const std::vector<io::StorageId>& group_run_ids,
                          const io::StorageId& output_run_id);

public:
    /**
     * @brief Constructor for k-way merge sorter
     * @param factory Reference to stream factory
     * @param input_id Input storage identifier
     * @param output_id Output storage identifier
     * @param mem_bytes Memory for creating runs in bytes
     * @param k_degree Degree of k-way merging (minimum 2)
     * @param io_buf_elems Size of I/O buffer in elements
     * @param sort_ascending Sort direction (default ascending)
     * @throws std::invalid_argument if k_degree < 2
     * @throws std::runtime_error if output_id is in temporary storage context
     */
    KWayMergeSorter(io::IStreamFactory<T>& factory, io::StorageId input_id, io::StorageId output_id,
                    uint64_t mem_bytes, uint64_t k_degree, uint64_t io_buf_elems,
                    bool sort_ascending = true);

    /**
     * @brief Performs sorting
     *
     * Main method that performs complete k-way external sorting:
     * 1. Creates initial sorted runs
     * 2. Repeatedly merges runs until one remains
     * 3. Makes final run permanent with output_id name
     */
    void Sort();
};

/**
 * @note Performance tip: When sorting non-trivial or move-only types prefer
 * using stream implementations that support TakeValue() and rvalue Write(T&&).
 * The sorter uses these methods internally (when available) to minimize copies
 * by moving elements between streams and temporary buffers.
 */
template <typename T>
std::vector<io::StorageId> KWayMergeSorter<T>::CreateInitialRuns() {
    detail::LogInfo(">>> CreateInitialRuns started for input: '" + input_id_ +
                    "' with memory limit: " + std::to_string(memory_for_runs_bytes_ / (1024.0 * 1024.0)) + " MB");
    detail::LogInfo("    sizeof(T) = " + std::to_string(sizeof(T)) + " bytes");

    std::unique_ptr<io::IInputStream<T>> input_stream =
        stream_factory_.CreateInputStream(input_id_, file_io_buffer_elements_);

    if (input_stream->IsEmptyOriginalStorage()) {
        detail::LogInfo("KWayMergeSorter: Input storage is empty. No runs created.");
        return {};
    }

    auto serializer = serialization::CreateSerializer<T>();
    std::vector<io::StorageId> run_ids;
    std::vector<T> run_buffer;
    int run_counter = 1;

    while (!input_stream->IsExhausted()) {
        run_buffer.clear();
        uint64_t current_run_mem_usage = 0;
        int element_counter = 0;
        
        detail::LogInfo("--- Starting Run #" + std::to_string(run_counter++) + " ---");

        while (!input_stream->IsExhausted()) {
            const T& current_element = input_stream->Value();
            
            uint64_t serialized_size = serializer->GetSerializedSize(current_element);
            uint64_t estimated_element_footprint;

            if constexpr (std::is_trivial_v<T> && std::is_standard_layout_v<T>) {
                estimated_element_footprint = sizeof(T);
            } else {
                estimated_element_footprint = serialized_size + sizeof(T);
            }

            detail::LogInfo("[Loop " + std::to_string(element_counter++) + "]: "
                            "current_mem=" + std::to_string(current_run_mem_usage) +
                            ", next_elem_footprint=" + std::to_string(estimated_element_footprint) +
                            " (ser_size=" + std::to_string(serialized_size) + ")" +
                            ", limit=" + std::to_string(memory_for_runs_bytes_) +
                            ", vec_size=" + std::to_string(run_buffer.size()) +
                            ", vec_cap=" + std::to_string(run_buffer.capacity()));

            if (run_buffer.empty()) {
                if (estimated_element_footprint > memory_for_runs_bytes_) {
                    throw std::runtime_error("KWayMergeSorter: Memory limit is too small for a single element.");
                }
            } 
            else if (current_run_mem_usage + estimated_element_footprint > memory_for_runs_bytes_) {
                detail::LogInfo("    Limit reached. Breaking loop to finalize run.");
                break;
            }

            current_run_mem_usage += estimated_element_footprint;
            run_buffer.push_back(input_stream->TakeValue());
            input_stream->Advance();
        }

        if (!run_buffer.empty()) {
            detail::LogInfo("    Sorting " + std::to_string(run_buffer.size()) + " elements...");
            if (ascending_) {
                std::sort(run_buffer.begin(), run_buffer.end());
            } else {
                std::sort(run_buffer.begin(), run_buffer.end(), std::greater<T>());
            }

            io::StorageId run_id;
            std::unique_ptr<io::IOutputStream<T>> out_run =
                stream_factory_.CreateTempOutputStream(run_id, file_io_buffer_elements_);
            for (T& val : run_buffer) {
                out_run->Write(std::move(val));
            }
            out_run->Finalize();
            run_ids.push_back(std::move(run_id));
            detail::LogInfo("    Run created: " + run_id + " with " +
                            std::to_string(out_run->GetTotalElementsWritten()) +
                            " elements. Estimated mem usage: " + std::to_string(current_run_mem_usage) + " bytes.");
        }
    }
    return run_ids;
}

template <typename T>
void KWayMergeSorter<T>::MergeGroupOfRuns(const std::vector<io::StorageId>& group_run_ids,
                                          const io::StorageId& output_run_id) {
    detail::LogInfo("KWayMergeSorter: Merging " + std::to_string(group_run_ids.size()) +
                    " runs into " + output_run_id);

    std::priority_queue<MergeSource<T>, std::vector<MergeSource<T>>, MergeSourceComparator> pq{
        MergeSourceComparator(ascending_)};
    std::vector<std::unique_ptr<io::IInputStream<T>>> input_streams_store;
    input_streams_store.reserve(group_run_ids.size());

    for (const auto& run_id : group_run_ids) {
        auto stream_ptr = stream_factory_.CreateInputStream(run_id, file_io_buffer_elements_);
        if (!stream_ptr->IsExhausted()) {
            pq.push({stream_ptr.get()});
        }
        input_streams_store.push_back(std::move(stream_ptr));
    }

    std::unique_ptr<io::IOutputStream<T>> output_stream =
        stream_factory_.CreateOutputStream(output_run_id, file_io_buffer_elements_);

    while (!pq.empty()) {
        MergeSource<T> current_source = pq.top();
        pq.pop();
        T val = current_source.stream->TakeValue();
        output_stream->Write(std::move(val));
        current_source.stream->Advance();
        if (!current_source.stream->IsExhausted()) {
            pq.push({current_source.stream});
        }
    }
    output_stream->Finalize();
    detail::LogInfo("KWayMergeSorter: Merged group into " + output_run_id + " with " +
                    std::to_string(output_stream->GetTotalElementsWritten()) + " elements.");
}

template <typename T>
KWayMergeSorter<T>::KWayMergeSorter(io::IStreamFactory<T>& factory, io::StorageId input_id,
                                    io::StorageId output_id, uint64_t mem_bytes, uint64_t k_degree,
                                    uint64_t io_buf_elems, bool sort_ascending)
    : stream_factory_(factory),
      input_id_(std::move(input_id)),
      output_id_(std::move(output_id)),
      memory_for_runs_bytes_(mem_bytes),
      k_way_degree_(k_degree),
      file_io_buffer_elements_(io_buf_elems),
      ascending_(sort_ascending) {
    if (k_way_degree_ < 2) {
        throw std::invalid_argument("KWayMergeSorter: k_way_degree must be at least 2.");
    }
    io::StorageId temp_context_id = stream_factory_.GetTempStorageContextId();
    if (!temp_context_id.empty() && output_id_.rfind(temp_context_id, 0) == 0 &&
        output_id_.length() > temp_context_id.length()) {
        throw std::runtime_error("KWayMergeSorter: Output storage ID '" + output_id_ +
                                 "' seems to be inside the temporary storage context '" +
                                 temp_context_id + "'.");
    }
}

template <typename T>
void KWayMergeSorter<T>::Sort() {
    std::vector<io::StorageId> current_run_ids = CreateInitialRuns();

    if (current_run_ids.empty()) {
        detail::LogInfo("KWayMergeSorter: No initial runs. Creating empty output " + output_id_);
        auto empty_out = stream_factory_.CreateOutputStream(output_id_, file_io_buffer_elements_);
        empty_out->Finalize();
        return;
    }

    std::vector<io::StorageId> runs_to_delete_this_pass;
    while (current_run_ids.size() > 1) {
        std::vector<io::StorageId> next_pass_run_ids;
        runs_to_delete_this_pass.clear();
        detail::LogInfo("KWayMergeSorter: Merge pass with " +
                        std::to_string(current_run_ids.size()) + " runs.");

        for (uint64_t i = 0; i < current_run_ids.size(); i += k_way_degree_) {
            std::vector<io::StorageId> group_to_merge;
            for (uint64_t j = 0; j < k_way_degree_ && (i + j) < current_run_ids.size(); ++j) {
                group_to_merge.push_back(current_run_ids[i + j]);
            }
            if (group_to_merge.empty()) {
                continue;
            }

            io::StorageId merged_run_id;
            bool is_final_merge_to_output = (current_run_ids.size() <= k_way_degree_) && (i == 0);

            if (is_final_merge_to_output) {
                merged_run_id = output_id_;
                detail::LogInfo("KWayMergeSorter: Merging to final output: " + output_id_);
            } else {
                auto temp_stream =
                    stream_factory_.CreateTempOutputStream(merged_run_id, file_io_buffer_elements_);
                temp_stream->Finalize();
            }

            MergeGroupOfRuns(group_to_merge, merged_run_id);
            next_pass_run_ids.push_back(std::move(merged_run_id));

            for (const auto& id_to_del : group_to_merge) {
                runs_to_delete_this_pass.push_back(id_to_del);
            }
        }
        current_run_ids = next_pass_run_ids;
        for (const auto& id_del : runs_to_delete_this_pass) {
            if (id_del != output_id_) {
                stream_factory_.DeleteStorage(id_del);
            }
        }
    }

    if (current_run_ids.size() == 1) {
        if (current_run_ids[0] != output_id_) {
            detail::LogInfo("KWayMergeSorter: Finalizing " + current_run_ids[0] + " as " +
                            output_id_);
            stream_factory_.MakeStoragePermanent(current_run_ids[0], output_id_);
        } else {
            detail::LogInfo("KWayMergeSorter: Output is already in " + output_id_);
        }
    } else if (current_run_ids.empty() && !stream_factory_.StorageExists(output_id_)) {
        detail::LogInfo("KWayMergeSorter: No runs left and output " + output_id_ +
                        " does not exist. Creating empty.");
        auto empty_out = stream_factory_.CreateOutputStream(output_id_, file_io_buffer_elements_);
        empty_out->Finalize();
    } else if (!current_run_ids.empty()) {
        detail::LogError("Error: KWayMergeSorter finished with unexpected runs: " +
                         std::to_string(current_run_ids.size()));
    }
}

}  // namespace external_sort
