#include "diskManager.hpp"
#include <iostream>

int DiskManager::diskReads = 0;
int DiskManager::diskWrites = 0;

DiskManager::DiskManager(std::string fname) : filename(fname) {
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

// === OBSŁUGA STRON DANYCH (PRIMARY) ===

void DiskManager::writePage(int pageIndex, const Page& page) {
    if (!file.is_open()) open();
    file.clear();
    
    size_t pSize = getPageOnDiskSize();
    file.seekp(pageIndex * pSize, std::ios::beg);
    
    file.write(reinterpret_cast<const char*>(&page.recordCount), sizeof(int));
    file.write(reinterpret_cast<const char*>(&page.overflowPointer), sizeof(int32_t));
    
    file.write(reinterpret_cast<const char*>(page.records.data()), RECORDS_PER_PAGE * sizeof(Record));
    
    file.flush();
    diskWrites++;
}

bool DiskManager::readPage(int pageIndex, Page& page) {
    if (!file.is_open()) open();
    file.clear();
    
    size_t pSize = getPageOnDiskSize();
    file.seekg(pageIndex * pSize, std::ios::beg);
    
    if (file.read(reinterpret_cast<char*>(&page.recordCount), sizeof(int))) {
        file.read(reinterpret_cast<char*>(&page.overflowPointer), sizeof(int32_t));
        
        page.records.resize(RECORDS_PER_PAGE);
        file.read(reinterpret_cast<char*>(page.records.data()), RECORDS_PER_PAGE * sizeof(Record));
        
        diskReads++;
        return true;
    }
    return false;
}

// === OBSŁUGA STRON INDEKSU (NOWE) ===

void DiskManager::writeIndexPage(int pageIndex, const IndexPage& page) {
    if (!file.is_open()) open();
    file.clear();
    
    // Rozmiar strony indeksu na dysku
    size_t size = sizeof(int) + (INDEX_ENTRIES_PER_PAGE * sizeof(IndexEntry));
    
    file.seekp(pageIndex * size, std::ios::beg);
    
    file.write(reinterpret_cast<const char*>(&page.count), sizeof(int));
    file.write(reinterpret_cast<const char*>(page.entries), INDEX_ENTRIES_PER_PAGE * sizeof(IndexEntry));
    
    file.flush();
    diskWrites++;
}

bool DiskManager::readIndexPage(int pageIndex, IndexPage& page) {
    if (!file.is_open()) open();
    file.clear();
    
    size_t size = sizeof(int) + (INDEX_ENTRIES_PER_PAGE * sizeof(IndexEntry));
    file.seekg(pageIndex * size, std::ios::beg);
    
    if (file.read(reinterpret_cast<char*>(&page.count), sizeof(int))) {
        file.read(reinterpret_cast<char*>(page.entries), INDEX_ENTRIES_PER_PAGE * sizeof(IndexEntry));
        diskReads++;
        return true;
    }
    return false;
}

// === OBSŁUGA REKORDÓW (OVERFLOW) ===

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
    size_t size = file.tellg();
    if (elementSize == 0) return 0;
    return (int)(size / elementSize);
}