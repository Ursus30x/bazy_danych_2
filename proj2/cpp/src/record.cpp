// record.cpp
#include "record.hpp"
#include <iomanip>
#include <sstream>

Record::Record() 
    : key(0), timestamp(0), nextPointer(NULL_POINTER), isDeleted(true) {}

Record::Record(uint32_t k, uint32_t t) 
    : key(k), timestamp(t), nextPointer(NULL_POINTER), isDeleted(false) {}

std::string Record::toString() const {
    if (isDeleted) return "[DELETED]";
    std::stringstream ss;
    ss << "K:" << key << " D:" << timestamp;
    if (nextPointer != NULL_POINTER) ss << " ->" << nextPointer;
    return ss.str();
}

bool Record::operator<(const Record& other) const { return key < other.key; }
bool Record::operator>(const Record& other) const { return key > other.key; }
bool Record::operator==(const Record& other) const { return key == other.key; }
bool Record::operator<=(const Record& other) const { return key <= other.key; }
bool Record::operator>=(const Record& other) const { return key >= other.key; }

std::ostream& operator<<(std::ostream& os, const Record& rec) {
    os << rec.toString();
    return os;
}