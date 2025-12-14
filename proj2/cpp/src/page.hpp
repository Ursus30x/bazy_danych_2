#pragma once
#include "record.hpp"
#include <vector>

extern int RECORDS_PER_PAGE;

size_t getPageOnDiskSize();

struct Page {
    std::vector<Record> records;
    int recordCount;         
    int32_t overflowPointer; 

    Page();
    
    bool insertRecord(const Record& rec);
    Record* findRecord(uint32_t key);
    bool deleteRecord(uint32_t key);

    void print() const;
};