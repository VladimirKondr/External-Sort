/**
 * @file k_way_merge_sorter.hpp
 * @brief K-way external merge sort
 */

#pragma once

#include "external_sort_logging.hpp"
#include "interfaces.hpp"
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
    T value;                      ///< Value from source
    io::IInputStream<T>* stream;  ///< Pointer to source stream

    /**
     * @brief Comparison operator for priority queue
     * @param other Another source to compare
     * @return true if this source is greater than other
     */
    bool operator>(const MergeSource<T>& other) const {
        return value > other.value;
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
            return ascending ? (a.value > b.value) : (a.value < b.value);
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

template <typename T>
std::vector<io::StorageId> KWayMergeSorter<T>::CreateInitialRuns() {
    std::unique_ptr<io::IInputStream<T>> input_stream =
        stream_factory_.CreateInputStream(input_id_, file_io_buffer_elements_);

    if (input_stream->IsEmptyOriginalStorage()) {
        detail::LogInfo("KWayMergeSorter: Input storage " + input_id_ + " is empty. No runs.");
        return {};
    }

    std::vector<io::StorageId> run_ids;
    std::vector<T> run_buffer_std_vec;
    const uint64_t elements_per_run = memory_for_runs_bytes_ / sizeof(T);
    if (elements_per_run == 0) {
        throw std::runtime_error("KWayMergeSorter: Memory limit too small.");
    }
    run_buffer_std_vec.reserve(elements_per_run);

    while (!input_stream->IsExhausted()) {
        run_buffer_std_vec.clear();
        while (run_buffer_std_vec.size() < elements_per_run && !input_stream->IsExhausted()) {
            run_buffer_std_vec.push_back(input_stream->TakeValue());
            input_stream->Advance();
        }

        if (!run_buffer_std_vec.empty()) {
            if (ascending_) {
                std::sort(run_buffer_std_vec.begin(), run_buffer_std_vec.end());
            } else {
                std::sort(run_buffer_std_vec.begin(), run_buffer_std_vec.end(), std::greater<T>());
            }

            io::StorageId run_id;
            std::unique_ptr<io::IOutputStream<T>> out_run =
                stream_factory_.CreateTempOutputStream(run_id, file_io_buffer_elements_);
            for (T& val : run_buffer_std_vec) {
                out_run->Write(std::move(val));
            }
            out_run->Finalize();
            run_ids.push_back(run_id);
            detail::LogInfo("KWayMergeSorter: Created initial run " + run_id + " with " +
                            std::to_string(out_run->GetTotalElementsWritten()) + " elements.");
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
            pq.push({stream_ptr->TakeValue(), stream_ptr.get()});
        }
        input_streams_store.push_back(std::move(stream_ptr));
    }

    std::unique_ptr<io::IOutputStream<T>> output_stream =
        stream_factory_.CreateOutputStream(output_run_id, file_io_buffer_elements_);

    while (!pq.empty()) {
        MergeSource<T> current_source = pq.top();
        pq.pop();
        output_stream->Write(std::move(current_source.value));
        current_source.stream->Advance();
        if (!current_source.stream->IsExhausted()) {
            pq.push({current_source.stream->TakeValue(), current_source.stream});
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
            next_pass_run_ids.push_back(merged_run_id);

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
