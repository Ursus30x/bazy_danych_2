#include "isam.hpp"
#include "logger.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include <cstdio> // remove, rename

ISAM::ISAM(std::string prefix, double alphaVal, double reorgThresh) 
    : filenamePrefix(prefix), alpha(alphaVal), reorgThreshold(reorgThresh) {
    
    primaryFile = new DiskManager(prefix + "_primary.bin");
    overflowFile = new DiskManager(prefix + "_overflow.bin");
    indexFile = new DiskManager(prefix + "_index.bin");

    if (primaryFile->getFileSize(getPageOnDiskSize()) == 0) {
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
    primaryFile->clear();
    overflowFile->clear();
    indexFile->clear();

    Page rootPage;
    primaryFile->writePage(0, rootPage);
    
    std::vector<IndexEntry> idx;
    idx.push_back({0, 0});
    saveIndex(idx);
}

void ISAM::clearDatabase() {
    Logger::log("Clearing database files...\n");
    primaryFile->close();
    overflowFile->close();
    indexFile->close();

    std::remove((filenamePrefix + "_primary.bin").c_str());
    std::remove((filenamePrefix + "_overflow.bin").c_str());
    std::remove((filenamePrefix + "_index.bin").c_str());

    primaryFile->open();
    overflowFile->open();
    indexFile->open();

    initStructure();
}

// === OBSŁUGA INDEKSU (POPRAWIONA - STRONICOWANIE) ===

void ISAM::saveIndex(const std::vector<IndexEntry>& index) {
    indexFile->clear();
    
    IndexPage page;
    page.count = 0;
    int pageIdx = 0;
    
    for (const auto& entry : index) {
        page.entries[page.count++] = entry;
        
        if (page.count >= INDEX_ENTRIES_PER_PAGE) {
            indexFile->writeIndexPage(pageIdx++, page);
            page.count = 0;
        }
    }
    
    // Zapisz ostatnią stronę, jeśli zawiera dane
    if (page.count > 0) {
        indexFile->writeIndexPage(pageIdx, page);
    }
}

std::vector<IndexEntry> ISAM::loadIndex() {
    std::vector<IndexEntry> index;
    IndexPage page;
    int pageIdx = 0;
    
    // Czytaj kolejne strony indeksu, aż do końca pliku
    while (indexFile->readIndexPage(pageIdx++, page)) {
        for (int i = 0; i < page.count; i++) {
            index.push_back(page.entries[i]);
        }
    }
    return index;
}

int ISAM::findPrimaryPageIndex(uint32_t key) {
    // W tej symulacji wczytujemy cały indeks do RAM (zgodnie z FAQ)
    // ale robimy to stronami przez DiskManager (również zgodnie z FAQ)
    std::vector<IndexEntry> index = loadIndex();
    
    if (index.empty()) return 0;
    
    int targetPage = 0;
    for (const auto& entry : index) {
        if (entry.key <= key) targetPage = entry.pageIndex;
        else break; 
    }
    return targetPage;
}

// === CRUD ===

bool ISAM::insertRecord(uint32_t key, uint32_t data) {
    if (readRecord(key) != nullptr) return false; // Duplikat

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
    
    // Automatyczna Reorganizacja
    int primaryPages = primaryFile->getFileSize(getPageOnDiskSize());
    int overflowRecords = overflowFile->getFileSize(sizeof(Record));
    int primaryCapacity = primaryPages * RECORDS_PER_PAGE; 
    
    if (primaryCapacity > 0 && overflowRecords > MIN_OVERFLOW_RECORDS_FOR_REORG) {
        double currentRatio = (double)overflowRecords / (double)primaryCapacity;
        if (currentRatio >= reorgThreshold) {
            Logger::log_verbose("[Auto-Reorg] Ratio %.2f >= %.2f. Triggering...\n", currentRatio, reorgThreshold);
            reorganize();
        }
    }
    return true; 
}

void ISAM::addToOverflow(int pageIdx, Page& page, Record newRec) {
    int newRecAddr = overflowFile->appendRecord(newRec);
    
    if (page.overflowPointer == NULL_POINTER) {
        page.overflowPointer = newRecAddr;
        primaryFile->writePage(pageIdx, page);
        return;
    }
    
    // Wstawianie na początek łańcucha
    Record headRec;
    overflowFile->readRecord(page.overflowPointer, headRec);
    if (newRec.key < headRec.key) {
        newRec.nextPointer = page.overflowPointer;
        overflowFile->writeRecord(newRecAddr, newRec);
        
        page.overflowPointer = newRecAddr; 
        primaryFile->writePage(pageIdx, page);
        return;
    }
    
    // Wstawianie w środek/koniec
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

// === REORGANIZACJA ===

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

    int totalPages = primaryFile->getFileSize(getPageOnDiskSize());
    bool isFirstRecordOnPage = true;

    for (int i = 0; i < totalPages; ++i) {
        Page oldPage;
        primaryFile->readPage(i, oldPage);
        std::vector<Record> recordsToProcess;
        
        // Zbieranie rekordów ze strony i overflow
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

        // WAŻNE: Sortowanie, aby zachować porządek w nowym pliku
        std::sort(recordsToProcess.begin(), recordsToProcess.end());

        // Wstawianie do nowej struktury
        for (auto& r : recordsToProcess) {
            r.nextPointer = NULL_POINTER; // Reset wskaźnika
            
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

void ISAM::display() {
    int numPages = primaryFile->getFileSize(getPageOnDiskSize());
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
    int numPages = primaryFile->getFileSize(getPageOnDiskSize());
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