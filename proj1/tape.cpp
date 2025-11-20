#include "tape.hpp"

Tape::Tape(const std::string& name, size_t block)
    : filename(name), readCount(0), writeCount(0), blockSize(block) {
    numOfRecordInBlock = blockSize / sizeof(time_record_type);
}

bool Tape::open(std::ios::openmode mode) {
    file.open(filename, mode | std::ios::binary);
    return file.is_open();
}

void Tape::close() {
    if (file.is_open()) file.close();
}

void Tape::write_block(size_t blockNum, RecordType* records, size_t recordCount) {
    if (!file.is_open()) return;
    file.seekp(blockNum * blockSize, std::ios::beg);

    size_t count = recordCount ? recordCount : numOfRecordInBlock;
    for (size_t i = 0; i < numOfRecordInBlock; ++i) {
        time_record_type value;
        if (i < count) value = records[i].get_timestamp();
        else value = 0;
        file.write(reinterpret_cast<char*>(&value), sizeof(time_record_type));
    }
    writeCount++;
}

bool Tape::read_block(size_t blockNum, std::vector<RecordType>& buffer) {
    if (!file.is_open()) return false;
    size_t totalBlocks = get_total_blocks();
    if (blockNum >= totalBlocks) return false;

    buffer.clear();
    file.seekg(blockNum * blockSize, std::ios::beg);

    for (size_t i = 0; i < numOfRecordInBlock; ++i) {
        time_record_type value;
        if (!file.read(reinterpret_cast<char*>(&value), sizeof(time_record_type)))
            return false;
        if (value != 0) buffer.push_back(RecordType(value));
    }
    readCount++;
    return true;
}

void Tape::generate_random_file(size_t records) {
    std::ofstream out(filename, std::ios::binary | std::ios::trunc);
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<time_record_type> dist(1, 20);

    size_t fullBlocks = records / numOfRecordInBlock;
    size_t remainder = records % numOfRecordInBlock;

    for (size_t b = 0; b < fullBlocks; ++b) {
        for (size_t i = 0; i < numOfRecordInBlock; ++i) {
            time_record_type value = dist(rng);
            out.write(reinterpret_cast<char*>(&value), sizeof(time_record_type));
        }
    }

    if (remainder > 0) {
        for (size_t i = 0; i < remainder; ++i) {
            time_record_type value = dist(rng);
            out.write(reinterpret_cast<char*>(&value), sizeof(time_record_type));
        }
        time_record_type zero = 0;
        for (size_t i = remainder; i < numOfRecordInBlock; ++i) {
            out.write(reinterpret_cast<char*>(&zero), sizeof(time_record_type));
        }
    }

    out.close();
}

void Tape::display_block(size_t block) {
    std::ifstream in(filename, std::ios::binary);
    in.seekg(block * blockSize, std::ios::beg);

    for (size_t i = 0; i < numOfRecordInBlock; ++i) {
        time_record_type value;
        if (!in.read(reinterpret_cast<char*>(&value), sizeof(time_record_type))) break;
        if (value == 0) std::cout << "_ ";
        else std::cout << value << " ";
    }
    std::cout << std::endl;
    in.close();
}

void Tape::display() {
    std::ifstream in(filename, std::ios::binary);
    time_record_type value;
    size_t count = 0;

    std::cout << "| ";
    while (in.read(reinterpret_cast<char*>(&value), sizeof(time_record_type))) {
        if (value == 0) std::cout << "_ ";
        else std::cout << value << " ";
        count++;
        if (count % numOfRecordInBlock == 0) std::cout << "| ";
    }
    std::cout << std::endl;
    in.close();
}

size_t Tape::get_total_blocks() {
    std::ifstream in(filename, std::ios::binary | std::ios::ate);
    size_t fileSize = static_cast<size_t>(in.tellg());
    in.close();
    return fileSize / blockSize;
}


size_t Tape::get_read_count() const { return readCount; }
size_t Tape::get_write_count() const { return writeCount; }
size_t Tape::get_block_size() const { return blockSize; }
size_t Tape::get_num_of_record_in_block() const { return numOfRecordInBlock; }
std::string Tape::get_filename() const { return filename; }