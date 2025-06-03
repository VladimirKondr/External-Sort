/**
 * @file k_way_merge_sorter.tpp
 * @brief Реализация k-путевой внешней сортировки слиянием
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include <algorithm>
#include <iostream>

template <typename T>
std::vector<StorageId> KWayMergeSorter<T>::CreateInitialRuns() {
    std::unique_ptr<IInputStream<T>> input_stream =
        stream_factory_.CreateInputStream(input_id_, file_io_buffer_elements_);

    if (input_stream->IsEmptyOriginalStorage()) {
        DEBUG_COUT_INFO(
            "KWayMergeSorter: Input storage " << input_id_ << " is empty. No runs." << std::endl);
        return {};
    }

    std::vector<StorageId> run_ids;
    std::vector<T> run_buffer_std_vec;
    const uint64_t elements_per_run = memory_for_runs_bytes_ / sizeof(T);
    if (elements_per_run == 0) {
        throw std::runtime_error("KWayMergeSorter: Memory limit too small.");
    }
    run_buffer_std_vec.reserve(elements_per_run);

    while (!input_stream->IsExhausted()) {
        run_buffer_std_vec.clear();
        while (run_buffer_std_vec.size() < elements_per_run && !input_stream->IsExhausted()) {
            run_buffer_std_vec.push_back(input_stream->Value());
            input_stream->Advance();
        }

        if (!run_buffer_std_vec.empty()) {
            if (ascending_) {
                std::sort(run_buffer_std_vec.begin(), run_buffer_std_vec.end());
            } else {
                std::sort(run_buffer_std_vec.begin(), run_buffer_std_vec.end(), std::greater<T>());
            }

            StorageId run_id;
            std::unique_ptr<IOutputStream<T>> out_run =
                stream_factory_.CreateTempOutputStream(run_id, file_io_buffer_elements_);
            for (const T& val : run_buffer_std_vec) {
                out_run->Write(val);
            }
            out_run->Finalize();
            run_ids.push_back(run_id);
            DEBUG_COUT_SUCCESS(
                "KWayMergeSorter: Created initial run "
                << run_id << " with " << out_run->GetTotalElementsWritten() << " elements."
                << std::endl);
        }
    }
    return run_ids;
}

template <typename T>
void KWayMergeSorter<T>::MergeGroupOfRuns(
    const std::vector<StorageId>& group_run_ids, const StorageId& output_run_id) {
    DEBUG_COUT_INFO(
        "KWayMergeSorter: Merging "
        << group_run_ids.size() << " runs into " << output_run_id << std::endl);

    std::priority_queue<MergeSource<T>, std::vector<MergeSource<T>>, MergeSourceComparator> pq{
      MergeSourceComparator(ascending_)};
    std::vector<std::unique_ptr<IInputStream<T>>> input_streams_store;
    input_streams_store.reserve(group_run_ids.size());

    for (const auto& run_id : group_run_ids) {
        auto stream_ptr = stream_factory_.CreateInputStream(run_id, file_io_buffer_elements_);
        if (!stream_ptr->IsExhausted()) {
            pq.push({stream_ptr->Value(), stream_ptr.get()});
        }
        input_streams_store.push_back(std::move(stream_ptr));
    }

    std::unique_ptr<IOutputStream<T>> output_stream =
        stream_factory_.CreateOutputStream(output_run_id, file_io_buffer_elements_);

    while (!pq.empty()) {
        MergeSource<T> current_source = pq.top();
        pq.pop();
        output_stream->Write(current_source.value);
        current_source.stream->Advance();
        if (!current_source.stream->IsExhausted()) {
            pq.push({current_source.stream->Value(), current_source.stream});
        }
    }
    output_stream->Finalize();
    DEBUG_COUT_SUCCESS(
        "KWayMergeSorter: Merged group into "
        << output_run_id << " with " << output_stream->GetTotalElementsWritten() << " elements."
        << std::endl);
}

template <typename T>
KWayMergeSorter<T>::KWayMergeSorter(
    IStreamFactory<T>& factory, StorageId input_id, StorageId output_id, uint64_t mem_bytes,
    uint64_t k_degree, uint64_t io_buf_elems, bool sort_ascending)
    : stream_factory_(factory)
    , input_id_(std::move(input_id))
    , output_id_(std::move(output_id))
    , memory_for_runs_bytes_(mem_bytes)
    , k_way_degree_(k_degree)
    , file_io_buffer_elements_(io_buf_elems)
    , ascending_(sort_ascending) {
    if (k_way_degree_ < 2) {
        throw std::invalid_argument("KWayMergeSorter: k_way_degree must be at least 2.");
    }
    StorageId temp_context_id = stream_factory_.GetTempStorageContextId();
    if (!temp_context_id.empty() && output_id_.rfind(temp_context_id, 0) == 0 &&
        output_id_.length() > temp_context_id.length()) {
        throw std::runtime_error(
            "KWayMergeSorter: Output storage ID '" + output_id_ +
            "' seems to be inside the temporary storage context '" + temp_context_id + "'.");
    }
}

template <typename T>
void KWayMergeSorter<T>::Sort() {
    std::vector<StorageId> current_run_ids = CreateInitialRuns();

    if (current_run_ids.empty()) {
        DEBUG_COUT_INFO(
            "KWayMergeSorter: No initial runs. Creating empty output " << output_id_ << std::endl);
        auto empty_out = stream_factory_.CreateOutputStream(output_id_, file_io_buffer_elements_);
        empty_out->Finalize();
        return;
    }

    std::vector<StorageId> runs_to_delete_this_pass;
    while (current_run_ids.size() > 1) {
        std::vector<StorageId> next_pass_run_ids;
        runs_to_delete_this_pass.clear();
        DEBUG_COUT_INFO(
            "KWayMergeSorter: Merge pass with " << current_run_ids.size() << " runs." << std::endl);

        for (uint64_t i = 0; i < current_run_ids.size(); i += k_way_degree_) {
            std::vector<StorageId> group_to_merge;
            for (uint64_t j = 0; j < k_way_degree_ && (i + j) < current_run_ids.size(); ++j) {
                group_to_merge.push_back(current_run_ids[i + j]);
            }
            if (group_to_merge.empty()) {
                continue;
            }

            StorageId merged_run_id;
            bool is_final_merge_to_output = (current_run_ids.size() <= k_way_degree_) && (i == 0);

            if (is_final_merge_to_output) {
                merged_run_id = output_id_;
                DEBUG_COUT_SUCCESS(
                    "KWayMergeSorter: Merging to final output: " << output_id_ << std::endl);
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
            DEBUG_COUT_SUCCESS(
                "KWayMergeSorter: Finalizing "
                << current_run_ids[0] << " as " << output_id_ << std::endl);
            stream_factory_.MakeStoragePermanent(current_run_ids[0], output_id_);
        } else {
            DEBUG_COUT_INFO("KWayMergeSorter: Output is already in " << output_id_ << std::endl);
        }
    } else if (current_run_ids.empty() && !stream_factory_.StorageExists(output_id_)) {
        DEBUG_COUT_INFO(
            "KWayMergeSorter: No runs left and output "
            << output_id_ << " does not exist. Creating empty." << std::endl);
        auto empty_out = stream_factory_.CreateOutputStream(output_id_, file_io_buffer_elements_);
        empty_out->Finalize();
    } else if (!current_run_ids.empty()) {
        DEBUG_COUT_ERROR(
            "Error: KWayMergeSorter finished with unexpected runs: "
            << current_run_ids.size() << std::endl);
    }
}
