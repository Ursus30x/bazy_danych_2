#include "page.hpp"
#include <iostream>
#include <algorithm>

size_t getPageOnDiskSize() {
    return sizeof(int) + sizeof(int32_t) + (RECORDS_PER_PAGE * sizeof(Record));
}

Page::Page() : recordCount(0), overflowPointer(NULL_POINTER) {
    records.resize(RECORDS_PER_PAGE);
}

bool Page::insertRecord(const Record& rec) {
    if (recordCount >= RECORDS_PER_PAGE) return false;

    int i = recordCount - 1;
    while (i >= 0 && records[i].key > rec.key) {
        records[i + 1] = records[i];
        i--;
    }
    records[i + 1] = rec;
    recordCount++;
    return true;
}

Record* Page::findRecord(uint32_t key) {
    for (int i = 0; i < recordCount; ++i) {
        if (!records[i].isDeleted && records[i].key == key) {
            return &records[i];
        }
    }
    return nullptr;
}

bool Page::deleteRecord(uint32_t key) {
    Record* rec = findRecord(key);
    if (rec) {
        rec->isDeleted = true;
        return true;
    }
    return false;
}

void Page::print() const {
    std::cout << "[ ";
    for (int i = 0; i < RECORDS_PER_PAGE; ++i) {
        if (i < recordCount) {
            if (records[i].isDeleted) std::cout << "XX ";
            else std::cout << records[i].key << " ";
        } else {
            std::cout << "-- ";
        }
    }
    std::cout << "]";
    if (overflowPointer != NULL_POINTER) {
        std::cout << " -> OV: " << overflowPointer;
    }
    std::cout << std::endl;
}