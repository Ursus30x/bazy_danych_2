#include "tapeSort.hpp"
#include "tape.hpp"
#include <algorithm>
#include <queue>
#include "logger.hpp"

size_t totalPhases = 0;

void create_runs(Tape *tape, size_t bufferNumber) {
    if (!tape) return;

    size_t totalBlocks = tape->get_total_blocks();
    size_t recordsPerBlock = tape->get_num_of_record_in_block();

    std::vector<RecordType> buffer;
    buffer.reserve(bufferNumber * recordsPerBlock);

    size_t currentBlock = 0;
    size_t runIndex = 0;


    Logger::log_verbose("Creating runs...\n");

    if (!tape->open(std::ios::in | std::ios::out)) {
            Logger::log("Failed to open tape file!\n");
            return;
    }

    while (currentBlock < totalBlocks) {
        buffer.clear();

        // Read up to bufferNumber blocks into memory
        for (size_t i = 0; i < bufferNumber && currentBlock < totalBlocks; ++i, ++currentBlock) {
            std::vector<RecordType> tmp;
            if (!tape->read_block(currentBlock, tmp)) break;
            buffer.insert(buffer.end(), tmp.begin(), tmp.end());
        }

        if (buffer.empty()) break;

        // Sort records in memory
        std::sort(buffer.begin(), buffer.end(), [](const RecordType& a, const RecordType& b) {
            return a.get_timestamp() < b.get_timestamp();
        });

        // Write sorted run back to the same region
        size_t totalRecords = buffer.size();
        size_t blocksNeeded = (totalRecords + recordsPerBlock - 1) / recordsPerBlock;

        RecordType* ptr = buffer.data();
        for (size_t b = 0; b < blocksNeeded; ++b) {
            size_t offset = b * recordsPerBlock;
            size_t remaining = totalRecords - offset;
            size_t count = std::min(remaining, recordsPerBlock);
            tape->write_block(runIndex * bufferNumber + b, ptr + offset, count);
        }

        Logger::log_verbose("| ");
        for(RecordType record : buffer){
            Logger::log_verbose("%d ", record.get_timestamp());
        }
        Logger::log_verbose("|\n");

        runIndex++;
    }
    Logger::log_verbose("\n");

    tape->close();
}

void merge(Tape* tape, size_t bufferNumber) {
    if (!tape->open(std::ios::in | std::ios::out)) {
            Logger::log("Failed to reopen tape!\n");
            return;
    }


    if (bufferNumber < 2) {
        Logger::log("Need at least 2 buffers for merging\n");
        return;
    }

    size_t totalBlocks = tape->get_total_blocks();
    size_t recordsPerBlock = tape->get_num_of_record_in_block();
    size_t initialRunSize = bufferNumber; // in blocks

    // Calculate number of initial runs
    size_t numRuns = (totalBlocks + initialRunSize - 1) / initialRunSize;

    if (numRuns <= 1) {
        Logger::log_verbose("File already sorted (only 1 run exists)\n\n");
        return;
    }

    size_t mergeWays = bufferNumber - 1; // n-1 input buffers, 1 output buffer
    size_t currentRunSize = initialRunSize;
    int phase = 1;

    // Create temporary tape for output
    Tape* outputTape = new Tape("temp_merge.bin", tape->get_block_size());

    while (numRuns > 1) {
        Logger::log_verbose("\n========== Merge Phase %d ==========\n", phase);
        Logger::log_verbose("Merging %zu runs of size %zu blocks\n",
                    numRuns, currentRunSize);

        if (!outputTape->open(std::ios::in | std::ios::out | std::ios::trunc)) {
            Logger::log("Failed to open output tape!\n");
            delete outputTape;
            return;
        }

        size_t runsProcessed = 0;
        size_t outputBlockNum = 0;
        size_t newRunCount = 0;

        // Process runs in groups of mergeWays
        while (runsProcessed < numRuns) {
            size_t runsInThisGroup = std::min(mergeWays, numRuns - runsProcessed);

            Logger::log_verbose("\nMerging runs %zu to %zu: ", runsProcessed,
                        runsProcessed + runsInThisGroup - 1);

            // Structure to track each input run
            struct RunInfo {
                size_t runIndex;
                size_t startBlock;
                size_t endBlock;
                size_t currentBlock;
                std::vector<RecordType> buffer;
                size_t bufferPos;
            };

            std::vector<RunInfo> runs(runsInThisGroup);

            // Initialize run information
            for (size_t i = 0; i < runsInThisGroup; ++i) {
                runs[i].runIndex = runsProcessed + i;
                runs[i].startBlock = runs[i].runIndex * currentRunSize;
                runs[i].endBlock = std::min(runs[i].startBlock + currentRunSize, totalBlocks);
                runs[i].currentBlock = runs[i].startBlock;
                runs[i].bufferPos = 0;

                // Load first block of this run
                tape->read_block(runs[i].currentBlock, runs[i].buffer);
                runs[i].currentBlock++;
            }

            // Min-heap: pair of (RecordType, runIndex)
            auto cmp = [](const std::pair<RecordType, size_t>& a, const std::pair<RecordType, size_t>& b) {
                return a.first.get_timestamp() > b.first.get_timestamp();
            };
            
            std::priority_queue<std::pair<RecordType, size_t>, 
                              std::vector<std::pair<RecordType, size_t>>, 
                              decltype(cmp)> minHeap(cmp);
            // Initialize heap with first record from each run
            for (size_t i = 0; i < runsInThisGroup; ++i) {
                if (!runs[i].buffer.empty()) {
                    minHeap.push({runs[i].buffer[runs[i].bufferPos], i});
                    runs[i].bufferPos++;
                }
            }

            // Output buffer and track merged records for display
            std::vector<RecordType> outputBuffer;
            outputBuffer.reserve(recordsPerBlock);
            std::vector<RecordType> mergedRun; // For displaying this merged run

            // Merge process
            while (!minHeap.empty()) {
                auto [minRecord, runIdx] = minHeap.top();
                minHeap.pop();

                // Add to output buffer
                outputBuffer.push_back(minRecord);
                mergedRun.push_back(minRecord);

                // If output buffer is full, write it
                if (outputBuffer.size() >= recordsPerBlock) {
                    outputTape->write_block(outputBlockNum++, outputBuffer.data(), outputBuffer.size());
                    outputBuffer.clear();
                }

                // Refill the run buffer if needed
                if (runs[runIdx].bufferPos >= runs[runIdx].buffer.size()) {
                    if (runs[runIdx].currentBlock < runs[runIdx].endBlock) {
                        runs[runIdx].buffer.clear();
                        tape->read_block(runs[runIdx].currentBlock, runs[runIdx].buffer);
                        runs[runIdx].currentBlock++;
                        runs[runIdx].bufferPos = 0;
                    }
                }

                // Add next record from same run to heap
                if (runs[runIdx].bufferPos < runs[runIdx].buffer.size()) {
                    minHeap.push({runs[runIdx].buffer[runs[runIdx].bufferPos], runIdx});
                    runs[runIdx].bufferPos++;
                }
            }

            // Write remaining records in output buffer
            if (!outputBuffer.empty()) {
                outputTape->write_block(outputBlockNum++, outputBuffer.data(), outputBuffer.size());
            }

            // Display the merged run
            Logger::log_verbose("| ");
            for (const auto& record : mergedRun) {
                Logger::log_verbose("%d ", record.get_timestamp());
            }
            Logger::log_verbose("|");

            runsProcessed += runsInThisGroup;
            newRunCount++;
        }

        Logger::log_verbose("\n\n");

        outputTape->close();

        // Swap tapes: copy output back to input
        tape->close();
        std::remove(tape->get_filename().c_str());
        std::rename("temp_merge.bin", tape->get_filename().c_str());

        if (!tape->open(std::ios::in | std::ios::out)) {
            Logger::log("Failed to reopen tape!\n");
            delete outputTape;
            return;
        }

        // Display complete state after this phase
        Logger::log_verbose("After phase %d - %zu runs:\n", phase, newRunCount);
        if(Logger::verbose)tape->display();

        // Update for next phase
        totalBlocks = tape->get_total_blocks();
        currentRunSize *= mergeWays;
        numRuns = newRunCount;
        phase++;
        totalPhases++;
    }

    delete outputTape;
    
    Logger::log("\n========================================\n");
    Logger::log("Merge complete! File is now sorted.\n");
    Logger::log("========================================\n\n");
}

void sort_tape(Tape *tape, size_t bufferNumber) {

    create_runs(tape, bufferNumber);
    merge(tape, bufferNumber);
    Logger::log("Sorted file contents:\n");
    tape->display();

    Logger::log_verbose("\nStats:\n");
    Logger::log_verbose("Total merge phases %ld\n", totalPhases);
    Logger::log_verbose("Total read count %ld\n",   Counts::totalReadCount);
    Logger::log_verbose("Total write count %ld\n",  Counts::totalWriteCount);
}
