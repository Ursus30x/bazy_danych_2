#include "recordType.hpp"
#include <iostream>

RecordType::RecordType() : timestamp(0) {}

RecordType::RecordType(time_record_type t) : timestamp(t) {}

time_record_type RecordType::get_timestamp() const {
    return timestamp;
}

std::string RecordType::get_date_time() const {
    std::time_t tt = static_cast<std::time_t>(timestamp);
    std::tm* tm_info = std::localtime(&tt);
    std::ostringstream oss;
    oss << std::put_time(tm_info, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

RecordType& RecordType::operator=(time_record_type t) {
    timestamp = t;
    return *this;
}

bool RecordType::operator<(const RecordType& other) const {
    return timestamp < other.timestamp;
}

bool RecordType::operator>(const RecordType& other) const {
    return timestamp > other.timestamp;
}

bool RecordType::operator==(const RecordType& other) const {
    return timestamp == other.timestamp;
}

std::ostream& operator<<(std::ostream& os, const RecordType& rec) {
    os << rec.get_timestamp();
    return os;
}
