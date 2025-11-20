#include "tapeSort.hpp"
#include "tape.hpp"
#include <algorithm>
#include <iostream>
#include <queue>

void create_runs(Tape *tape, size_t bufferNumber) {
    if (!tape) return;

    size_t totalBlocks = tape->get_total_blocks();
    size_t recordsPerBlock = tape->get_num_of_record_in_block();

    std::vector<RecordType> buffer;
    buffer.reserve(bufferNumber * recordsPerBlock);

    size_t currentBlock = 0;
    size_t runIndex = 0;

    std::cout << "Runs created: " << std::endl;
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

        std::cout << "| ";
        for(RecordType record : buffer){
            std::cout << record.get_timestamp() << " ";
        }

        runIndex++;
    }

    std::cout << "|" << std::endl;
}

void merge(Tape* tape, size_t bufferNumber) {
    if (bufferNumber < 2) {
        std::cout << "Need at least 2 buffers for merging" << std::endl;
        return;
    }

    size_t totalBlocks = tape->get_total_blocks();
    size_t recordsPerBlock = tape->get_num_of_record_in_block();
    size_t initialRunSize = bufferNumber; // in blocks
    
    // Calculate number of initial runs
    size_t numRuns = (totalBlocks + initialRunSize - 1) / initialRunSize;
    
    if (numRuns <= 1) {
        std::cout << "File already sorted (only 1 run exists)" << std::endl;
        return;
    }

    size_t mergeWays = bufferNumber - 1; // n-1 input buffers, 1 output buffer
    size_t currentRunSize = initialRunSize;
    int cycle = 1;

    // Create temporary tape for output
    Tape* outputTape = new Tape("temp_merge.bin", tape->get_block_size());
    
    while (numRuns > 1) {
        std::cout << "\n========== Merge Cycle " << cycle << " ==========" << std::endl;
        std::cout << "Merging " << numRuns << " runs of size " 
                  << currentRunSize << " blocks" << std::endl;

        if (!outputTape->open(std::ios::in | std::ios::out | std::ios::trunc)) {
            std::cerr << "Failed to open output tape!" << std::endl;
            delete outputTape;
            return;
        }

        size_t runsProcessed = 0;
        size_t outputBlockNum = 0;
        size_t newRunCount = 0;

        // Process runs in groups of mergeWays
        while (runsProcessed < numRuns) {
            size_t runsInThisGroup = std::min(mergeWays, numRuns - runsProcessed);
            
            std::cout << "\nMerging runs " << runsProcessed << " to " 
                      << (runsProcessed + runsInThisGroup - 1) << ": ";
            
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
            std::cout << "| ";
            for (const auto& record : mergedRun) {
                std::cout << record.get_timestamp() << " ";
            }
            std::cout << "|";

            runsProcessed += runsInThisGroup;
            newRunCount++;
        }

        std::cout << "\n" << std::endl;

        outputTape->close();

        // Swap tapes: copy output back to input
        tape->close();
        std::remove(tape->get_filename().c_str());
        std::rename("temp_merge.bin", tape->get_filename().c_str());
        
        if (!tape->open(std::ios::in | std::ios::out)) {
            std::cerr << "Failed to reopen tape!" << std::endl;
            delete outputTape;
            return;
        }

        // Display complete state after this cycle
        std::cout << "After Cycle " << cycle << " - " << newRunCount << " runs:" << std::endl;
        tape->display();

        // Update for next cycle
        totalBlocks = tape->get_total_blocks();
        currentRunSize *= mergeWays;
        numRuns = newRunCount;
        cycle++;
    }

    delete outputTape;
    std::cout << "\n========================================" << std::endl;
    std::cout << "Merge complete! File is now sorted." << std::endl;
    std::cout << "========================================" << std::endl;
}

void sort_tape(Tape *tape, size_t bufferNumber) {
    std::cout << "Creating runs...\n";
    create_runs(tape, bufferNumber);
    merge(tape, bufferNumber);
}
