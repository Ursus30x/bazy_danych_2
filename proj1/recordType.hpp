#pragma once
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdint>

typedef uint32_t time_record_type;

class RecordType {
private:
    time_record_type timestamp;

public:
    RecordType() : timestamp(0) {}
    RecordType(time_record_type t) : timestamp(t) {}

    time_record_type get_timestamp() const {
        return timestamp;
    }

    std::string get_date_time() const {
        std::time_t tt = static_cast<std::time_t>(timestamp);
        std::tm* tm_info = std::localtime(&tt);
        std::ostringstream oss;
        oss << std::put_time(tm_info, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    RecordType& operator=(time_record_type t) {
        timestamp = t;
        return *this;
    }

    bool operator<(const RecordType& other) const {
        return timestamp < other.timestamp;
    }

    bool operator>(const RecordType& other) const {
        return timestamp > other.timestamp;
    }

    bool operator==(const RecordType& other) const {
        return timestamp == other.timestamp;
    }

    friend std::ostream& operator<<(std::ostream& os, const RecordType& rec) {
        os << rec.get_timestamp();
        return os;
    }
};
