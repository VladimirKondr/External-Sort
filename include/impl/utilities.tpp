/**
 * @file utilities.tpp
 * @brief Реализация утилитных функций для тестирования
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include "../debug_logger.hpp"

#include <chrono>
#include <iostream>
#include <random>

template <typename T>
void CreateTestDataInStorage(
    IStreamFactory<T>& factory, const StorageId& id, uint64_t num_elements, bool random_data) {
    auto outfile = factory.CreateOutputStream(id, 4096);
    std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<uint64_t> distrib(0, num_elements > 0 ? num_elements * 10 : 1000);

    for (uint64_t i = 0; i < num_elements; ++i) {
        uint64_t val = random_data ? distrib(rng) : (num_elements - 1 - i);
        outfile->Write(static_cast<T>(val));
    }
    outfile->Finalize();
    DEBUG_COUT_SUCCESS(
        "CreateTestDataInStorage: Created "
        << id << " with " << outfile->GetTotalElementsWritten() << " elements." << std::endl);
}

template <typename T>
bool VerifySortedStorage(IStreamFactory<T>& factory, const StorageId& id, bool ascending) {
    std::unique_ptr<IInputStream<T>> infile;
    try {
        infile = factory.CreateInputStream(id, 1024);
    } catch (const std::exception& e) {
        DEBUG_COUT_ERROR(
            "VerifySortedStorage: Cannot open storage for verification: "
            << id << " - " << e.what() << std::endl);
        return false;
    }

    if (infile->IsEmptyOriginalStorage()) {
        DEBUG_COUT_INFO(
            "VerifySortedStorage: Storage " << id << " is empty, considered sorted." << std::endl);

        if (!infile->IsExhausted()) {
            DEBUG_COUT_ERROR(
                "VerifySortedStorage: Storage "
                << id << " IsEmptyOriginalStorage is true, but stream not initially exhausted."
                << std::endl);
            return false;
        }

        try {
            infile->Value();
            DEBUG_COUT_ERROR(
                "VerifySortedStorage: Storage "
                << id << " IsEmptyOriginalStorage is true, but Value() did not fail." << std::endl);
        } catch (const std::logic_error& e) {
            DEBUG_COUT_INFO(
                "VerifySortedStorage: Expected exception caught: " << e.what() << std::endl);
        }
        return true;
    }

    T prev_val = infile->Value();
    infile->Advance();
    uint64_t count = 1;

    while (!infile->IsExhausted()) {
        T current_val = infile->Value();
        count++;
        if (ascending) {
            if (current_val < prev_val) {
                DEBUG_COUT_ERROR(
                    "VerifySortedStorage: Sort order violation (asc) in "
                    << id << ": prev=" << prev_val << " > current=" << current_val << " at element "
                    << count << std::endl);
                return false;
            }
        } else {
            if (current_val > prev_val) {
                DEBUG_COUT_ERROR(
                    "VerifySortedStorage: Sort order violation (desc) in "
                    << id << ": prev=" << prev_val << " < current=" << current_val << " at element "
                    << count << std::endl);
                return false;
            }
        }
        prev_val = current_val;
        infile->Advance();
    }

    DEBUG_COUT_SUCCESS(
        "VerifySortedStorage: Storage "
        << id << " verified with " << count << " elements." << std::endl);
    return true;
}
