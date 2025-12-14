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

// ZMIANA: Ręczna serializacja strony
void DiskManager::writePage(int pageIndex, const Page& page) {
    if (!file.is_open()) open();
    file.clear();
    
    size_t pSize = getPageOnDiskSize();
    file.seekp(pageIndex * pSize, std::ios::beg);
    
    // 1. Zapisz nagłówki
    file.write(reinterpret_cast<const char*>(&page.recordCount), sizeof(int));
    file.write(reinterpret_cast<const char*>(&page.overflowPointer), sizeof(int32_t));
    
    // 2. Zapisz tablicę rekordów (dane wektora)
    // Zakładamy, że vector jest zawsze zresize'owany do RECORDS_PER_PAGE w konstruktorze Page
    file.write(reinterpret_cast<const char*>(page.records.data()), RECORDS_PER_PAGE * sizeof(Record));
    
    file.flush();
    diskWrites++;
}

// ZMIANA: Ręczna deserializacja strony
bool DiskManager::readPage(int pageIndex, Page& page) {
    if (!file.is_open()) open();
    file.clear();
    
    size_t pSize = getPageOnDiskSize();
    file.seekg(pageIndex * pSize, std::ios::beg);
    
    // 1. Czytaj nagłówki
    if (file.read(reinterpret_cast<char*>(&page.recordCount), sizeof(int))) {
        file.read(reinterpret_cast<char*>(&page.overflowPointer), sizeof(int32_t));
        
        // 2. Przygotuj wektor i wczytaj dane
        page.records.resize(RECORDS_PER_PAGE);
        file.read(reinterpret_cast<char*>(page.records.data()), RECORDS_PER_PAGE * sizeof(Record));
        
        diskReads++;
        return true;
    }
    return false;
}

// Metody dla Rekordów (Overflow) pozostają bez zmian, bo Record jest POD
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
    if (elementSize == 0) return 0; // Zabezpieczenie
    return (int)(size / elementSize);
}