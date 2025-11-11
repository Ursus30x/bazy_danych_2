#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>

#include "recordType.hpp"

class Tape {
private:
    std::fstream file;
    std::string filename;
    size_t readCount;
    size_t writeCount;
    size_t blockSize;
    size_t numOfRecords;

public:
    Tape(const std::string& name, size_t block = 4096)
        : filename(name), readCount(0), writeCount(0), blockSize(block) {
        numOfRecords = blockSize / sizeof(time_record_type);
    }

    bool open(std::ios::openmode mode) {
        file.open(filename, mode | std::ios::binary);
        return file.is_open();
    }

    void close() {
        if (file.is_open()) file.close();
    }

    void write_block(size_t blockNum, RecordType* records) {
        if (!file.is_open()) return;
        file.seekp(blockNum * blockSize, std::ios::beg);
        for (size_t i = 0; i < numOfRecords; ++i) {
            time_record_type value = records[i].get_timestamp();
            file.write(reinterpret_cast<char*>(&value), sizeof(time_record_type));
        }
        writeCount++;
    }

    bool read_block(size_t blockNum, std::vector<RecordType>& buffer) {
        if (!file.is_open()) return false;
        buffer.clear();
        file.seekg(blockNum * blockSize, std::ios::beg);
        for (size_t i = 0; i < numOfRecords; ++i) {
            time_record_type value;
            if (!file.read(reinterpret_cast<char*>(&value), sizeof(time_record_type))) {
                return false;
            }
            buffer.push_back(RecordType(value));
        }
        readCount++;
        return true;
    }

    void generate_random_file(size_t blocks) {
        std::ofstream out(filename, std::ios::binary | std::ios::trunc);
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<time_record_type> dist(1, 2000000000);
        for (size_t b = 0; b < blocks; ++b) {
            for (size_t i = 0; i < numOfRecords; ++i) {
                time_record_type value = dist(rng);
                out.write(reinterpret_cast<char*>(&value), sizeof(time_record_type));
            }
        }
        out.close();
    }

    void display_block(size_t block) {
        std::ifstream in(filename, std::ios::binary);
        in.seekg(block * blockSize, std::ios::beg);
        for (size_t i = 0; i < numOfRecords; ++i) {
            time_record_type value;
            if (!in.read(reinterpret_cast<char*>(&value), sizeof(time_record_type))) break;
            std::cout << value << " ";
        }
        std::cout << std::endl;
        in.close();
    }

    void display() {
        std::ifstream in(filename, std::ios::binary);
        time_record_type value;
        size_t count = 0;
        while (in.read(reinterpret_cast<char*>(&value), sizeof(time_record_type))) {
            std::cout << value << " ";
            count++;
            if (count % numOfRecords == 0) std::cout << "| ";
        }
        std::cout << std::endl;
        in.close();
    }

    size_t getReadCount() const { return readCount; }
    size_t getWriteCount() const { return writeCount; }
};
