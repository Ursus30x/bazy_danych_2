// record.hpp
#pragma once
#include <string>
#include <iostream>
#include <cstdint>

const int32_t NULL_POINTER = -1;

class Record {
public:
    uint32_t key;        
    uint32_t timestamp;   
    int32_t  nextPointer;  
    bool     isDeleted;     

    Record();
    Record(uint32_t k, uint32_t t);


    std::string toString() const;

    bool operator<(const Record& other) const;
    bool operator>(const Record& other) const;
    bool operator==(const Record& other) const;
    bool operator<=(const Record& other) const;
    bool operator>=(const Record& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Record& rec);
};