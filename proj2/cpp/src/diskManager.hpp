#pragma once
#include <fstream>
#include <string>
#include <vector>
#include "page.hpp"
#include "record.hpp"

// Struktura wpisu w indeksie (Klucz -> Numer Strony)
// Przeniesiona tutaj, aby DiskManager mógł z niej korzystać
struct IndexEntry {
    uint32_t key;
    int pageIndex;
};

// Stała określająca ile wpisów indeksu mieści się na jednej stronie indeksowej
const int INDEX_ENTRIES_PER_PAGE = 128; 

// Struktura strony indeksu
struct IndexPage {
    IndexEntry entries[INDEX_ENTRIES_PER_PAGE];
    int count = 0;
};

class DiskManager {
private:
    std::fstream file;
    std::string filename;
    
public:
    // Statystyki (zerowane przy starcie programu lub w main)
    static int diskReads;
    static int diskWrites;

    DiskManager(std::string fname);
    ~DiskManager();

    bool open(std::ios::openmode mode = std::ios::in | std::ios::out | std::ios::binary);
    void close();
    void clear(); // Czyści plik (truncate)

    std::string getFilename() const { return filename; }

    // Odczyt/Zapis Stron Danych (Primary Area)
    void writePage(int pageIndex, const Page& page);
    bool readPage(int pageIndex, Page& page);

    // Odczyt/Zapis Stron Indeksu (Index Area) - NOWE METODY
    void writeIndexPage(int pageIndex, const IndexPage& page);
    bool readIndexPage(int pageIndex, IndexPage& page);

    // Odczyt/Zapis Rekordów (Overflow Area)
    void writeRecord(int recordIndex, const Record& rec);
    int appendRecord(const Record& rec); 
    bool readRecord(int recordIndex, Record& rec);

    int getFileSize(size_t elementSize);
};