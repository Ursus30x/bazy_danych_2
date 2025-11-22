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
    RecordType();
    RecordType(time_record_type t);

    time_record_type get_timestamp() const;
    std::string get_date_time() const;

    RecordType& operator=(time_record_type t);
    bool operator<(const RecordType& other) const;
    bool operator>(const RecordType& other) const;
    bool operator==(const RecordType& other) const;

    friend std::ostream& operator<<(std::ostream& os, const RecordType& rec);
};
