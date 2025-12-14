#pragma once
#include "diskManager.hpp"
#include <string>
#include <vector>

// Struktura wpisu w indeksie (Klucz -> Numer Strony)
struct IndexEntry {
    uint32_t key;
    int pageIndex;
};

class ISAM {
private:
    DiskManager* primaryFile;
    DiskManager* overflowFile;
    DiskManager* indexFile;
    
    // Dodane brakujące pole:
    std::string filenamePrefix;
    
    // Parametr alpha dla reorganizacji (np. 0.5)
    double alpha;

    // Pomocnicze metody prywatne
    int findPrimaryPageIndex(uint32_t key);
    void addToOverflow(int pageIndex, Page& page, Record newRecord);
    void saveIndex(const std::vector<IndexEntry>& index);
    std::vector<IndexEntry> loadIndex();

public:
    ISAM(std::string prefix, double alphaVal = 0.5);
    ~ISAM();

    // Operacje CRUD
    bool insertRecord(uint32_t key, uint32_t data);
    Record* readRecord(uint32_t key);
    bool deleteRecord(uint32_t key);
    void updateRecord(uint32_t key, uint32_t newData);

    // Zarządzanie strukturą
    void reorganize();
    void display(); // Wyświetlanie struktury stron (debug)
    
    // Dodana brakująca deklaracja:
    void browse();  // Sekwencyjny przegląd rekordów
};