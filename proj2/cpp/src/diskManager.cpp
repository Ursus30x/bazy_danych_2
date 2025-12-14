// diskManager.cpp
#include "diskManager.hpp"
#include <iostream>

int DiskManager::diskReads = 0;
int DiskManager::diskWrites = 0;

DiskManager::DiskManager(std::string fname) : filename(fname) {
    // Upewnij się, że plik istnieje
    std::ofstream out(filename, std::ios::app | std::ios::binary);
    out.close();
}

DiskManager::~DiskManager() { close(); }

bool DiskManager::open(std::ios::openmode mode) {
    if (file.is_open()) file.close();
    file.open(filename, mode);
    return file.is_open();
}

void DiskManager::close() {
    if (file.is_open()) file.close();
}

void DiskManager::clear() {
    close();
    std::ofstream out(filename, std::ios::trunc | std::ios::binary);
    out.close();
    open();
}

void DiskManager::writePage(int pageIndex, const Page& page) {
    if (!file.is_open()) open();
    file.clear();
    file.seekp(pageIndex * sizeof(Page), std::ios::beg);
    file.write(reinterpret_cast<const char*>(&page), sizeof(Page));
    file.flush();
    diskWrites++;
}

bool DiskManager::readPage(int pageIndex, Page& page) {
    if (!file.is_open()) open();
    file.clear();
    file.seekg(pageIndex * sizeof(Page), std::ios::beg);
    if (file.read(reinterpret_cast<char*>(&page), sizeof(Page))) {
        diskReads++;
        return true;
    }
    return false;
}

void DiskManager::writeRecord(int recordIndex, const Record& rec) {
    if (!file.is_open()) open();
    file.clear();
    file.seekp(recordIndex * sizeof(Record), std::ios::beg);
    file.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
    diskWrites++;
}

int DiskManager::appendRecord(const Record& rec) {
    if (!file.is_open()) open();
    file.clear();
    file.seekp(0, std::ios::end);
    int index = (int)file.tellp() / sizeof(Record);
    file.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
    diskWrites++;
    return index;
}

bool DiskManager::readRecord(int recordIndex, Record& rec) {
    if (!file.is_open()) open();
    file.clear();
    file.seekg(recordIndex * sizeof(Record), std::ios::beg);
    if (file.read(reinterpret_cast<char*>(&rec), sizeof(Record))) {
        diskReads++;
        return true;
    }
    return false;
}

int DiskManager::getFileSize(size_t elementSize) {
    if (!file.is_open()) open();
    file.clear();
    file.seekg(0, std::ios::end);
    return (int)file.tellg() / elementSize;
}