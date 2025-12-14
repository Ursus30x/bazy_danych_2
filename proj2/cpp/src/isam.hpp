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
    
    std::string filenamePrefix;
    double alpha;
    
    // NOWE: Próg reorganizacji (stosunek V/N)
    double reorgThreshold; 

    // Pomocnicze metody prywatne
    int findPrimaryPageIndex(uint32_t key);
    void addToOverflow(int pageIndex, Page& page, Record newRecord);
    void saveIndex(const std::vector<IndexEntry>& index);
    std::vector<IndexEntry> loadIndex();
    
    // NOWE: Metoda inicjująca pustą strukturę (wywoływana w konstruktorze i po clear)
    void initStructure();

public:
    // Zaktualizowany konstruktor o parametr threshold
    ISAM(std::string prefix, double alphaVal = 0.5, double reorgThresh = 0.2);
    ~ISAM();

    // Operacje CRUD
    bool insertRecord(uint32_t key, uint32_t data);
    Record* readRecord(uint32_t key);
    bool deleteRecord(uint32_t key);
    void updateRecord(uint32_t key, uint32_t newData);

    // Zarządzanie strukturą
    void reorganize();
    void display();
    void browse();
    
    // NOWE: Usuwanie bazy danych
    void clearDatabase();
};