// page.hpp
#pragma once
#include "record.hpp"

// Stała b=4 z instrukcji
const int RECORDS_PER_PAGE = 4;

struct Page {
    Record records[RECORDS_PER_PAGE];
    int recordCount;         // Aktualna liczba rekordów na stronie
    int32_t overflowPointer; // Wskaźnik na początek łańcucha w Overflow Area (dla tej strony)

    Page();
    
    // Wstawia rekord na stronę zachowując porządek. Zwraca true jeśli się udało.
    bool insertRecord(const Record& rec);
    
    // Szuka rekordu na stronie. Zwraca wskaźnik lub nullptr.
    Record* findRecord(uint32_t key);
    
    // Usuwa logicznie rekord
    bool deleteRecord(uint32_t key);

    void print() const;
};