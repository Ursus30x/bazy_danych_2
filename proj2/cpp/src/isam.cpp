#include "isam.hpp"
#include "logger.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include <cstdio> // remove, rename

// ==========================================
// Konstruktor / Destruktor / Init
// ==========================================

ISAM::ISAM(std::string prefix, double alphaVal, double reorgThresh) 
    : filenamePrefix(prefix), alpha(alphaVal), reorgThreshold(reorgThresh) {
    
    primaryFile = new DiskManager(prefix + "_primary.bin");
    overflowFile = new DiskManager(prefix + "_overflow.bin");
    indexFile = new DiskManager(prefix + "_index.bin");

    // Jeśli plik główny jest pusty, zainicjuj strukturę
    if (primaryFile->getFileSize(sizeof(Page)) == 0) {
        initStructure();
    }
}

ISAM::~ISAM() {
    delete primaryFile;
    delete overflowFile;
    delete indexFile;
}

void ISAM::initStructure() {
    Logger::log_verbose("Initializing empty ISAM structure...\n");
    
    // Czyścimy/Tworzymy pliki
    primaryFile->clear();
    overflowFile->clear();
    indexFile->clear();

    // Tworzymy pierwszą pustą stronę
    Page rootPage;
    primaryFile->writePage(0, rootPage);
    
    // Indeks początkowy: Klucz 0 wskazuje na Stronę 0
    std::vector<IndexEntry> idx;
    idx.push_back({0, 0});
    saveIndex(idx);
}

void ISAM::clearDatabase() {
    Logger::log("Clearing database files...\n");
    
    // Zamknij uchwyty do plików
    primaryFile->close();
    overflowFile->close();
    indexFile->close();

    // Usuń pliki fizyczne
    std::remove((filenamePrefix + "_primary.bin").c_str());
    std::remove((filenamePrefix + "_overflow.bin").c_str());
    std::remove((filenamePrefix + "_index.bin").c_str());

    // Otwórz na nowo (stworzy puste pliki)
    primaryFile->open();
    overflowFile->open();
    indexFile->open();

    // Zainicjuj strukturę (strona 0 i indeks)
    initStructure();
}

// ==========================================
// Reszta metod pomocniczych (bez zmian)
// ==========================================
// (saveIndex, loadIndex, findPrimaryPageIndex, addToOverflow - skopiuj z poprzedniej wersji)
// ...

void ISAM::saveIndex(const std::vector<IndexEntry>& index) {
    indexFile->clear();
    if (index.empty()) return;
    std::ofstream out(indexFile->getFilename(), std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(index.data()), index.size() * sizeof(IndexEntry));
    out.close();
}

std::vector<IndexEntry> ISAM::loadIndex() {
    std::vector<IndexEntry> index;
    std::ifstream in(indexFile->getFilename(), std::ios::binary);
    if (!in.is_open()) return index;
    in.seekg(0, std::ios::end);
    size_t fileSize = in.tellg();
    in.seekg(0, std::ios::beg);
    if (fileSize > 0) {
        size_t count = fileSize / sizeof(IndexEntry);
        index.resize(count);
        in.read(reinterpret_cast<char*>(index.data()), fileSize);
    }
    in.close();
    return index;
}

int ISAM::findPrimaryPageIndex(uint32_t key) {
    std::vector<IndexEntry> index = loadIndex();
    if (index.empty()) return 0;
    int targetPage = 0;
    for (const auto& entry : index) {
        if (entry.key <= key) targetPage = entry.pageIndex;
        else break; 
    }
    return targetPage;
}

// ==========================================
// Operacje CRUD (Zmodyfikowany insertRecord)
// ==========================================

bool ISAM::insertRecord(uint32_t key, uint32_t data) {
    if (readRecord(key) != nullptr) return false; 

    Record newRec(key, data);
    int pageIdx = findPrimaryPageIndex(key);
    
    Page page;
    if (!primaryFile->readPage(pageIdx, page)) {
        Logger::log("Critical Error: Could not read page %d\n", pageIdx);
        return false;
    }

    if (page.recordCount < RECORDS_PER_PAGE) {
        if (page.insertRecord(newRec)) {
            primaryFile->writePage(pageIdx, page);
        } else {
             addToOverflow(pageIdx, page, newRec);
        }
    } else {
        addToOverflow(pageIdx, page, newRec);
    }
    
    // --- AUTOMATYCZNA REORGANIZACJA ---
    // Sprawdzamy warunek V/N > threshold
    // N (Primary Records) przybliżamy jako liczbę stron * pojemność (dla uproszczenia)
    // lub dokładnie iterując (kosztowne). 
    // Użyjemy rozmiaru plików jako prostej metryki.
    
    int primaryPages = primaryFile->getFileSize(sizeof(Page));
    // Liczba rekordów w overflow:
    int overflowRecords = overflowFile->getFileSize(sizeof(Record));
    
    // Szacowana liczba rekordów w primary (zakładając pełne wypełnienie alpha po reorgu)
    // Możemy po prostu odnieść się do liczby miejsc na stronach:
    int primaryCapacity = primaryPages * RECORDS_PER_PAGE; 
    
    // Aby uniknąć reorganizacji przy bardzo małych plikach (np. 1 rekord w ov, 4 w prim),
    // dodajemy warunek minimalnej liczby rekordów w overflow (np. > 5).
    if (primaryCapacity > 0 && overflowRecords > 5) {
        double currentRatio = (double)overflowRecords / (double)primaryCapacity;
        
        if (currentRatio >= reorgThreshold) {
            Logger::log_verbose("[Auto-Reorg] Ratio %.2f >= %.2f. Triggering...\n", currentRatio, reorgThreshold);
            reorganize();
        }
    }

    return true; 
}

// (addToOverflow, readRecord, updateRecord, deleteRecord - BEZ ZMIAN z poprzedniej wersji)
// ...
void ISAM::addToOverflow(int pageIdx, Page& page, Record newRec) {
    int newRecAddr = overflowFile->appendRecord(newRec);
    if (page.overflowPointer == NULL_POINTER) {
        page.overflowPointer = newRecAddr;
        primaryFile->writePage(pageIdx, page);
        return;
    }
    Record headRec;
    overflowFile->readRecord(page.overflowPointer, headRec);
    if (newRec.key < headRec.key) {
        newRec.nextPointer = page.overflowPointer;
        overflowFile->writeRecord(newRecAddr, newRec);
        page.overflowPointer = newRecAddr; 
        primaryFile->writePage(pageIdx, page);
        return;
    }
    int currentAddr = page.overflowPointer;
    int prevAddr = -1;
    Record currentRec;
    while (currentAddr != NULL_POINTER) {
        overflowFile->readRecord(currentAddr, currentRec);
        if (currentRec.key > newRec.key) break; 
        prevAddr = currentAddr;
        currentAddr = currentRec.nextPointer;
    }
    if (prevAddr != -1) {
        newRec.nextPointer = currentAddr;
        overflowFile->writeRecord(newRecAddr, newRec);
        Record prevRec;
        overflowFile->readRecord(prevAddr, prevRec);
        prevRec.nextPointer = newRecAddr;
        overflowFile->writeRecord(prevAddr, prevRec);
    }
}

Record* ISAM::readRecord(uint32_t key) {
    int pageIdx = findPrimaryPageIndex(key);
    static Record resultRec; 
    Page page;
    if (!primaryFile->readPage(pageIdx, page)) return nullptr;
    Record* foundOnPage = page.findRecord(key);
    if (foundOnPage) {
        resultRec = *foundOnPage;
        return &resultRec;
    }
    int nextAddr = page.overflowPointer;
    while (nextAddr != NULL_POINTER) {
        overflowFile->readRecord(nextAddr, resultRec);
        if (!resultRec.isDeleted && resultRec.key == key) return &resultRec;
        if (resultRec.key > key) return nullptr; 
        nextAddr = resultRec.nextPointer;
    }
    return nullptr;
}

void ISAM::updateRecord(uint32_t key, uint32_t newData) {
    if (readRecord(key)) {
        deleteRecord(key);
        insertRecord(key, newData);
        Logger::log("Updated record %d.\n", key);
    } else {
        Logger::log("Record %d not found.\n", key);
    }
}

bool ISAM::deleteRecord(uint32_t key) {
    int pageIdx = findPrimaryPageIndex(key);
    Page page;
    if (!primaryFile->readPage(pageIdx, page)) return false;
    if (page.deleteRecord(key)) {
        primaryFile->writePage(pageIdx, page);
        return true;
    }
    int currentAddr = page.overflowPointer;
    Record rec;
    while (currentAddr != NULL_POINTER) {
        overflowFile->readRecord(currentAddr, rec);
        if (rec.key == key && !rec.isDeleted) {
            rec.isDeleted = true;
            overflowFile->writeRecord(currentAddr, rec);
            return true;
        }
        if (rec.key > key) return false;
        currentAddr = rec.nextPointer;
    }
    return false;
}

// ==========================================
// Reorganizacja (Bez zmian, tylko używa metod pomocniczych)
// ==========================================

void ISAM::reorganize() {
    Logger::log("\n=== Reorganization (Alpha: %.2f) ===\n", alpha);
    
    std::string newPrimName = filenamePrefix + "_new_prim.bin";
    std::string newOverName = filenamePrefix + "_new_over.bin";

    DiskManager newPrimary(newPrimName);
    newPrimary.clear();
    DiskManager newOverflow(newOverName);
    newOverflow.clear();

    std::vector<IndexEntry> newIndex;
    Page newPage;
    int newPageIndex = 0;
    int recordsOnCurrentPage = 0;
    int limitPerPage = (int)(RECORDS_PER_PAGE * alpha);
    if (limitPerPage < 1) limitPerPage = 1;

    int totalPages = primaryFile->getFileSize(sizeof(Page));
    bool isFirstRecordOnPage = true;

    for (int i = 0; i < totalPages; ++i) {
        Page oldPage;
        primaryFile->readPage(i, oldPage);
        std::vector<Record> recordsToProcess;
        
        for (int k = 0; k < RECORDS_PER_PAGE; k++) {
            if (k < oldPage.recordCount && !oldPage.records[k].isDeleted) {
                recordsToProcess.push_back(oldPage.records[k]);
            }
        }
        int ovPtr = oldPage.overflowPointer;
        while (ovPtr != NULL_POINTER) {
            Record ovRec;
            overflowFile->readRecord(ovPtr, ovRec);
            if (!ovRec.isDeleted) recordsToProcess.push_back(ovRec);
            ovPtr = ovRec.nextPointer;
        }

        std::sort(recordsToProcess.begin(), recordsToProcess.end());

        for (auto& r : recordsToProcess) {
            r.nextPointer = NULL_POINTER;
            if (isFirstRecordOnPage) {
                newIndex.push_back({r.key, newPageIndex});
                isFirstRecordOnPage = false;
            }
            newPage.insertRecord(r);
            recordsOnCurrentPage++;

            if (recordsOnCurrentPage >= limitPerPage) {
                newPrimary.writePage(newPageIndex, newPage);
                newPageIndex++;
                newPage = Page();
                recordsOnCurrentPage = 0;
                isFirstRecordOnPage = true;
            }
        }
    }

    if (recordsOnCurrentPage > 0) {
        newPrimary.writePage(newPageIndex, newPage);
    }

    primaryFile->close();
    overflowFile->close();
    indexFile->close();
    newPrimary.close();
    newOverflow.close();

    std::remove((filenamePrefix + "_primary.bin").c_str());
    std::remove((filenamePrefix + "_overflow.bin").c_str());
    std::remove((filenamePrefix + "_index.bin").c_str());

    std::rename(newPrimName.c_str(), (filenamePrefix + "_primary.bin").c_str());
    std::rename(newOverName.c_str(), (filenamePrefix + "_overflow.bin").c_str());

    primaryFile->open();
    overflowFile->open();
    indexFile->open();

    saveIndex(newIndex);
    
    Logger::log("Reorg complete. Pages: %d. Overflow cleared.\n", newPageIndex + 1);
}

// (Display i Browse - bez zmian)
void ISAM::display() {
    int numPages = primaryFile->getFileSize(sizeof(Page));
    std::vector<IndexEntry> idx = loadIndex();

    Logger::log("\n=== ISAM STRUCTURE (Alpha: %.2f) ===\n", alpha);
    Logger::log("INDEX: ");
    for(auto& e : idx) Logger::log("[%u->P%d] ", e.key, e.pageIndex);
    Logger::log("\n-----------------------------------\n");
    
    for (int i = 0; i < numPages; ++i) {
        Page p;
        if(!primaryFile->readPage(i, p)) break;
        Logger::log("Page %2d: ", i);
        p.print();
        int ov = p.overflowPointer;
        if (ov != NULL_POINTER) {
            Logger::log("        `-> OVERFLOW: ");
            int safety = 0;
            while (ov != NULL_POINTER && safety++ < 100) {
                Record r;
                overflowFile->readRecord(ov, r);
                if (r.isDeleted) Logger::log("[XX] -> ");
                else Logger::log("[%u] -> ", r.key);
                ov = r.nextPointer;
            }
            if (safety >= 100) Logger::log("... (loop detected)");
            Logger::log("NULL\n");
        }
    }
    Logger::log("===================================\n\n");
}

void ISAM::browse() {
    Logger::log("Browsing file sequentially:\n");
    int numPages = primaryFile->getFileSize(sizeof(Page));
    for(int i=0; i<numPages; i++) {
        Page p;
        primaryFile->readPage(i, p);
        std::vector<Record> allRecs;
        for(int k=0; k<p.recordCount; k++) if(!p.records[k].isDeleted) allRecs.push_back(p.records[k]);
        int ov = p.overflowPointer;
        while(ov != -1) {
            Record r;
            overflowFile->readRecord(ov, r);
            if(!r.isDeleted) allRecs.push_back(r);
            ov = r.nextPointer;
        }
        std::sort(allRecs.begin(), allRecs.end());
        for(auto& r : allRecs) {
            Logger::log("%u: %u, ", r.key, r.timestamp);
        }
    }
    Logger::log("\n");
}