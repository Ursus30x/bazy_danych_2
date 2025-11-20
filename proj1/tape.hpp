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
    size_t numOfRecordInBlock;

public:
    Tape(const std::string& name, size_t block = 4096);

    bool open(std::ios::openmode mode);
    void close();

    void write_block(size_t blockNum, RecordType* records, size_t recordCount = 0);
    bool read_block(size_t blockNum, std::vector<RecordType>& buffer);

    void generate_random_file(size_t records);

    void display_block(size_t block);
    void display();

    size_t get_total_blocks();

    size_t get_read_count() const;
    size_t get_write_count() const;
    size_t get_block_size() const;
    size_t get_num_of_record_in_block() const;
    std::string get_filename() const;
};
