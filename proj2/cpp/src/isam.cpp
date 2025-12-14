#include "isam.hpp"
#include "logger.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include <cstdio> // remove, rename

// ==========================================
// Konstruktor / Destruktor
// ==========================================

ISAM::ISAM(std::string prefix, double alphaVal) 
    : filenamePrefix(prefix), alpha(alphaVal) {
    
    primaryFile = new DiskManager(prefix + "_primary.bin");
    overflowFile = new DiskManager(prefix + "_overflow.bin");
    indexFile = new DiskManager(prefix + "_index.bin");

    // Inicjalizacja pustej struktury, jeśli plik główny jest pusty
    if (primaryFile->getFileSize(sizeof(Page)) == 0) {
        Logger::log_verbose("Initializing empty ISAM structure...\n");
        Page rootPage;
        primaryFile->writePage(0, rootPage);
        
        // Indeks początkowy: Klucz 0 wskazuje na Stronę 0
        std::vector<IndexEntry> idx;
        idx.push_back({0, 0});
        saveIndex(idx);
    }
}

ISAM::~ISAM() {
    delete primaryFile;
    delete overflowFile;
    delete indexFile;
}

// ==========================================
// Zarządzanie Indeksem (Index File)
// ==========================================

void ISAM::saveIndex(const std::vector<IndexEntry>& index) {
    // Czyścimy plik indeksu i zapisujemy wektor
    indexFile->clear();
    // Używamy "brutalnej" metody zapisu binarnego wektora
    if (index.empty()) return;
    
    // Hack: DiskManager jest generyczny, ale tutaj musimy zapisać tablicę struktur
    // Otwieramy plik "ręcznie" przez fstream wewnątrz DiskManager lub (bezpieczniej)
    // dodajemy metodę do DiskManager. Tutaj założymy, że indexFile->file jest dostępny
    // lub użyjemy standardowego fstreamu, bo indeks jest mały.
    
    std::ofstream out(indexFile->getFilename(), std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(index.data()), index.size() * sizeof(IndexEntry));
    out.close();
}

std::vector<IndexEntry> ISAM::loadIndex() {
    std::vector<IndexEntry> index;
    std::ifstream in(indexFile->getFilename(), std::ios::binary);
    
    if (!in.is_open()) return index;

    // Pobierz rozmiar pliku
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
    // Indeks rzadki: szukamy ostatniego wpisu, którego klucz <= szukany klucz
    for (const auto& entry : index) {
        if (entry.key <= key) {
            targetPage = entry.pageIndex;
        } else {
            // Skoro lista jest posortowana, a trafiliśmy na większy klucz,
            // to znaczy, że szukany rekord musi być na poprzedniej stronie (targetPage)
            break; 
        }
    }
    return targetPage;
}

// ==========================================
// Operacje CRUD
// ==========================================

bool ISAM::insertRecord(uint32_t key, uint32_t data) {
    // KROK 1: Sprawdzenie unikalności klucza
    // Jeśli rekord istnieje, zwracamy false (bez logowania błędu tutaj,
    // aby nie śmiecić przy losowym generowaniu)
    if (readRecord(key) != nullptr) {
        return false; 
    }

    Record newRec(key, data);
    int pageIdx = findPrimaryPageIndex(key);
    
    Page page;
    if (!primaryFile->readPage(pageIdx, page)) {
        Logger::log("Critical Error: Could not read page %d\n", pageIdx);
        return false;
    }

    // 1. Próba wstawienia na stronę główną
    if (page.recordCount < RECORDS_PER_PAGE) {
        if (page.insertRecord(newRec)) {
            primaryFile->writePage(pageIdx, page);
        } else {
             // Teoretycznie niemożliwe przy sprawdzeniu recordCount, ale dla spójności
             addToOverflow(pageIdx, page, newRec);
        }
    } else {
        // 2. Strona pełna -> wstaw do Overflow
        addToOverflow(pageIdx, page, newRec);
    }
    
    return true; // Sukces
}

void ISAM::addToOverflow(int pageIdx, Page& page, Record newRec) {
    // Logger::log_verbose("Page full. Inserting into Overflow.\n");

    // Zapisz nowy rekord na końcu pliku Overflow
    int newRecAddr = overflowFile->appendRecord(newRec);
    
    // Przypadek A: Łańcuch jest pusty
    if (page.overflowPointer == NULL_POINTER) {
        page.overflowPointer = newRecAddr;
        primaryFile->writePage(pageIdx, page);
        return;
    }

    // Przypadek B: Wstawianie na początek łańcucha (newKey < firstOverflowKey)
    Record headRec;
    overflowFile->readRecord(page.overflowPointer, headRec);
    
    if (newRec.key < headRec.key) {
        // Nowy rekord staje się głową łańcucha
        newRec.nextPointer = page.overflowPointer;
        overflowFile->writeRecord(newRecAddr, newRec); // Nadpisz go z nowym pointerem
        
        page.overflowPointer = newRecAddr; // Aktualizuj wskaźnik na stronie
        primaryFile->writePage(pageIdx, page);
        return;
    }

    // Przypadek C: Wstawianie w środek lub na koniec łańcucha
    int currentAddr = page.overflowPointer;
    int prevAddr = -1;
    Record currentRec;

    // Iterujemy po łańcuchu szukając miejsca
    while (currentAddr != NULL_POINTER) {
        overflowFile->readRecord(currentAddr, currentRec);
        
        if (currentRec.key > newRec.key) {
            // Znaleźliśmy element większy, wstawiamy PRZED nim
            break; 
        }
        
        prevAddr = currentAddr;
        currentAddr = currentRec.nextPointer;
    }

    // W tym momencie: prevAddr wskazuje na element mniejszy od nowego,
    // a currentAddr na element większy (lub NULL).
    // Wstawiamy POMIĘDZY prevAddr a currentAddr.
    
    if (prevAddr != -1) {
        // 1. Nowy wskazuje na current
        newRec.nextPointer = currentAddr;
        overflowFile->writeRecord(newRecAddr, newRec);
        
        // 2. Poprzedni wskazuje na nowy
        Record prevRec;
        overflowFile->readRecord(prevAddr, prevRec);
        prevRec.nextPointer = newRecAddr;
        overflowFile->writeRecord(prevAddr, prevRec);
    }
}

Record* ISAM::readRecord(uint32_t key) {
    int pageIdx = findPrimaryPageIndex(key);
    static Record resultRec; // static dla bezpieczeństwa zwracania wskaźnika
    
    Page page;
    if (!primaryFile->readPage(pageIdx, page)) return nullptr;

    // 1. Sprawdź rekordy na stronie głównej
    Record* foundOnPage = page.findRecord(key);
    if (foundOnPage) {
        resultRec = *foundOnPage;
        return &resultRec;
    }

    // 2. Jeśli nie znaleziono i klucz jest większy od największego na stronie (lub strona pełna),
    // sprawdź Overflow.
    // Uwaga: Nawet jak klucz jest mniejszy, w ISAM rekord może być w overflow jeśli
    // został wypchnięty lub wstawiony gdy strona była pełna w dziwny sposób.
    // Ale w naszej implementacji Primary jest zawsze posortowane, a Overflow dopięty.
    
    int nextAddr = page.overflowPointer;
    while (nextAddr != NULL_POINTER) {
        overflowFile->readRecord(nextAddr, resultRec);
        
        if (!resultRec.isDeleted && resultRec.key == key) {
            return &resultRec;
        }
        // Optymalizacja: łańcuch overflow też jest posortowany rosnąco.
        if (resultRec.key > key) return nullptr; 
        
        nextAddr = resultRec.nextPointer;
    }

    return nullptr;
}

void ISAM::updateRecord(uint32_t key, uint32_t newData) {
    Record* rec = readRecord(key);
    if (rec) {
        // Jeśli aktualizujemy tylko dane (timestamp), a klucz bez zmian:
        // Musimy znaleźć GDZIE on jest fizycznie i go nadpisać.
        // To wymagałoby zwracania adresu przez readRecord.
        // Uproszczenie: Delete + Insert
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

    // 1. Próba usunięcia ze strony głównej
    if (page.deleteRecord(key)) {
        primaryFile->writePage(pageIdx, page);
        return true;
    }

    // 2. Próba usunięcia z Overflow
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
// Reorganizacja
// ==========================================

void ISAM::reorganize() {
    Logger::log("\n=== Starting Reorganization (Alpha: %.2f) ===\n", alpha);
    
    // 1. Pliki tymczasowe
    std::string newPrimName = filenamePrefix + "_new_prim.bin";
    std::string newOverName = filenamePrefix + "_new_over.bin";
    std::string newIndxName = filenamePrefix + "_new_indx.bin"; // Tylko nazwa, zapiszemy na końcu

    DiskManager newPrimary(newPrimName);
    newPrimary.clear();
    DiskManager newOverflow(newOverName); // Będzie pusty na początku
    newOverflow.clear();

    std::vector<IndexEntry> newIndex;
    
    // Bufory do nowego pliku
    Page newPage;
    int newPageIndex = 0;
    int recordsOnCurrentPage = 0;
    int limitPerPage = (int)(RECORDS_PER_PAGE * alpha);
    if (limitPerPage < 1) limitPerPage = 1;

    // 2. Iteracja logiczna po STARYM pliku (In-Order Traversal)
    int totalPages = primaryFile->getFileSize(sizeof(Page));
    
    // Dodaj wpis do indeksu dla pierwszej strony
    bool isFirstRecordOnPage = true;

    for (int i = 0; i < totalPages; ++i) {
        Page oldPage;
        primaryFile->readPage(i, oldPage);

        // Zbierz wszystkie rekordy z tej strony i jej łańcucha overflow
        std::vector<Record> recordsToProcess;
        
        // a) Rekordy ze strony głównej
        for (int k = 0; k < RECORDS_PER_PAGE; k++) {
            if (k < oldPage.recordCount && !oldPage.records[k].isDeleted) {
                recordsToProcess.push_back(oldPage.records[k]);
            }
        }
        
        // b) Rekordy z Overflow
        int ovPtr = oldPage.overflowPointer;
        while (ovPtr != NULL_POINTER) {
            Record ovRec;
            overflowFile->readRecord(ovPtr, ovRec);
            if (!ovRec.isDeleted) {
                recordsToProcess.push_back(ovRec);
            }
            ovPtr = ovRec.nextPointer;
        }

        std::sort(recordsToProcess.begin(), recordsToProcess.end());

        // c) Wstawiaj je do NOWEJ struktury
        for (auto& r : recordsToProcess) {
            // Resetuj wskaźniki (w nowym pliku głównym nie ma overflow na start)
            r.nextPointer = NULL_POINTER;
            
            // Jeśli to pierwszy rekord na nowej stronie, zaktualizuj indeks
            if (isFirstRecordOnPage) {
                newIndex.push_back({r.key, newPageIndex});
                isFirstRecordOnPage = false;
            }

            newPage.insertRecord(r);
            recordsOnCurrentPage++;

            // Jeśli osiągnięto limit alfa, zapisz stronę i zacznij nową
            if (recordsOnCurrentPage >= limitPerPage) {
                newPrimary.writePage(newPageIndex, newPage);
                
                newPageIndex++;
                newPage = Page(); // Reset
                recordsOnCurrentPage = 0;
                isFirstRecordOnPage = true;
            }
        }
    }

    // Zapisz ostatnią stronę, jeśli nie jest pusta
    if (recordsOnCurrentPage > 0) {
        newPrimary.writePage(newPageIndex, newPage);
    }

    // 3. Zamknij i podmień pliki
    primaryFile->close();
    overflowFile->close();
    indexFile->close();
    newPrimary.close();
    newOverflow.close();

    // Usuń stare
    std::remove((filenamePrefix + "_primary.bin").c_str());
    std::remove((filenamePrefix + "_overflow.bin").c_str());
    std::remove((filenamePrefix + "_index.bin").c_str());

    // Zmień nazwy nowych
    std::rename(newPrimName.c_str(), (filenamePrefix + "_primary.bin").c_str());
    std::rename(newOverName.c_str(), (filenamePrefix + "_overflow.bin").c_str());

    // Otwórz ponownie główne menedżery
    primaryFile->open();
    overflowFile->open();
    indexFile->open();

    // Zapisz nowy indeks
    saveIndex(newIndex);
    
    Logger::log("Reorganization complete. New Primary Pages: %d\n", newPageIndex + 1);
}

// ==========================================
// Debug / Wyświetlanie
// ==========================================

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
        p.print(); // zakłada użycie std::cout, można przerobić na Logger
        
        // Wyświetl łańcuch overflow
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
    // Proste przejście sekwencyjne logiczne (zgodnie z zadaniem)
    // To jest to samo co pętla w Reorganizacji, tylko wyświetla dane
    Logger::log("Browsing file sequentially:\n");
    int numPages = primaryFile->getFileSize(sizeof(Page));
    
    for(int i=0; i<numPages; i++) {
        Page p;
        primaryFile->readPage(i, p);
        
        // Scalanie (merge) rekordów ze strony i overflow w locie dla wyświetlenia
        // Ponieważ oba zbiory (tablica na stronie i lista overflow) są posortowane,
        // można by to zrobić ładnie. Tu zrobimy prościej: strona -> overflow (bo overflow wstawiamy "pomiędzy" logicznie, ale w ISAM v1 overflow to często "nadmiar")
        // UWAGA: W poprawnym ISAM overflow pointer dotyczy rekordów WIĘKSZYCH niż ostatni na stronie
        // LUB (w naszej implementacji) wstawiamy rekordy tam gdzie pasują.
        // Wyświetlę po prostu posortowaną listę wszystkich połączonych.
        
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