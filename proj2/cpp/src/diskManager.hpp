#pragma once
#include <fstream>
#include <string>
#include <vector>
#include "page.hpp"
#include "record.hpp"

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

    // Dodany brakujący getter:
    std::string getFilename() const { return filename; }

    // Odczyt/Zapis Stron (Primary Area)
    void writePage(int pageIndex, const Page& page);
    bool readPage(int pageIndex, Page& page);

    // Odczyt/Zapis Rekordów (Overflow Area)
    void writeRecord(int recordIndex, const Record& rec);
    int appendRecord(const Record& rec); 
    bool readRecord(int recordIndex, Record& rec);

    int getFileSize(size_t elementSize);
};