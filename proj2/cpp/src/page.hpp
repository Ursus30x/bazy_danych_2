#pragma once
#include "record.hpp"
#include <vector>

// Zmienna globalna zamiast stałej (definicja w main.cpp)
extern int RECORDS_PER_PAGE;

// Funkcja pomocnicza zwracająca rozmiar strony na dysku w bajtach
size_t getPageOnDiskSize();

struct Page {
    std::vector<Record> records; // Wektor zamiast tablicy statycznej
    int recordCount;         
    int32_t overflowPointer; 

    Page();
    
    bool insertRecord(const Record& rec);
    Record* findRecord(uint32_t key);
    bool deleteRecord(uint32_t key);

    void print() const;
};