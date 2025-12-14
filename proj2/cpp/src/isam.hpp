#pragma once
#include "diskManager.hpp"
#include <string>
#include <vector>

class ISAM {
private:
    DiskManager* primaryFile;
    DiskManager* overflowFile;
    DiskManager* indexFile;
    
    std::string filenamePrefix;
    double alpha;
    double reorgThreshold; 

    // Stała: minimalna liczba rekordów w overflow, by rozważać reorganizację
    static const int MIN_OVERFLOW_RECORDS_FOR_REORG = 5;

    // Metody pomocnicze
    int findPrimaryPageIndex(uint32_t key);
    void addToOverflow(int pageIndex, Page& page, Record newRecord);
    
    // Zmienione implementacje korzystające z DiskManager
    void saveIndex(const std::vector<IndexEntry>& index);
    std::vector<IndexEntry> loadIndex();
    
    void initStructure();

public:
    ISAM(std::string prefix, double alphaVal = 0.5, double reorgThresh = 0.2);
    ~ISAM();

    // Operacje CRUD
    bool insertRecord(uint32_t key, uint32_t data);
    Record* readRecord(uint32_t key);
    bool deleteRecord(uint32_t key);
    void updateRecord(uint32_t key, uint32_t newData);

    // Zarządzanie
    void reorganize();
    void display();
    void browse();
    void clearDatabase();
};