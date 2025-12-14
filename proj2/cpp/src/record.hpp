// record.hpp
#pragma once
#include <string>
#include <iostream>
#include <cstdint>

// Stała oznaczająca brak wskaźnika (NULL pointer w pliku)
const int32_t NULL_POINTER = -1;

class Record {
public:
    uint32_t key;           // Klucz główny (do sortowania)
    uint32_t timestamp;     // Dane (z zadania 1)
    int32_t  nextPointer;   // Wskaźnik do Overflow Area (offset/indeks rekordu)
    bool     isDeleted;     // Flaga usunięcia

    Record();
    Record(uint32_t k, uint32_t t);

    // Gettery/Settery (opcjonalne, skoro pola są publiczne dla ułatwienia operacji dyskowych)
    std::string toString() const;

    // Operatory porównania oparte na KLUCZU (zgodnie z instrukcją Zad 2)
    bool operator<(const Record& other) const;
    bool operator>(const Record& other) const;
    bool operator==(const Record& other) const;
    bool operator<=(const Record& other) const;
    bool operator>=(const Record& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Record& rec);
};