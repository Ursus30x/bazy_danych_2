#include "isam.hpp"
#include <getopt.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include "logger.hpp"
#include "stats.hpp" // <--- DODANO
#include <unistd.h> 

#define DEFAULT_FILENAME_PREFIX "database"

int RECORDS_PER_PAGE = 4; 

void print_menu() {
    if (isatty(fileno(stdin))) {
        std::cout << "\nCommands:\n"
                  << "  i <key> <data> : Insert record\n"
                  << "  r <key>        : Read record\n"
                  << "  u <key> <data> : Update record\n"
                  << "  d <key>        : Delete record\n"
                  << "  p              : Print structure\n"
                  << "  b              : Browse all records\n"
                  << "  x              : Force Reorganize\n"
                  << "  c              : Clear/Reset database\n"
                  << "  rnd <N>        : Insert N random records\n"
                  << "  srnd <N>       : Search N random records (Read Test)\n" // <--- NOWE
                  << "  q              : Quit and print Stats\n"
                  << "> " << std::flush;
    }
}

int main(int argc, char* argv[]) {
    int opt;
    std::string filenamePrefix = DEFAULT_FILENAME_PREFIX;
    double alpha = 0.5;
    double threshold = 0.2;
    bool verbose = false;

    static struct option long_opts[] = {
        {"help",        no_argument,        0,  'h'},
        {"file",        required_argument,  0,  'f'},
        {"alpha",       required_argument,  0,  'a'},
        {"threshold",   required_argument,  0,  't'},
        {"blocking",    required_argument,  0,  'b'},
        {"verbose",     no_argument,        0,  'v'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "hf:a:t:b:v", long_opts, nullptr)) != -1) {
        switch (opt) {
            case 'h': /* ... help msg ... */ return 0;
            case 'f': filenamePrefix = optarg; break;
            case 'a': alpha = std::stod(optarg); break;
            case 't': threshold = std::stod(optarg); break;
            case 'b': RECORDS_PER_PAGE = std::stoi(optarg); break;
            case 'v': verbose = true; break;
            default: return 1;
        }
    }

    if (RECORDS_PER_PAGE < 1) { /* error */ return 1; }

    Logger::verbose = verbose;
    if (verbose || isatty(fileno(stdin))) {
        Logger::log("Initializing ISAM. Prefix: %s, B: %d, Alpha: %.2f, ReorgThreshold: %.2f\n", 
                    filenamePrefix.c_str(), RECORDS_PER_PAGE, alpha, threshold);
    }

    ISAM isam(filenamePrefix, alpha, threshold);

    // Reset statystyk na starcie
    Stats::reset();

    std::string line, cmd;
    while (true) {
        print_menu();
        
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        std::stringstream ss(line);
        ss >> cmd;

        // Reset liczników sesyjnych DiskManagera
        DiskManager::diskReads = 0;
        DiskManager::diskWrites = 0;

        // ... (wewnątrz pętli while, obsługa komendy 'q') ...

        if (cmd == "q") {
            // Format: STATS alpha threshold reorgs inserts searches total_reads total_writes reorg_reads reorg_writes
            std::cout << "STATS " 
                      << alpha << " " 
                      << threshold << " "
                      << Stats::totalReorgs << " "
                      << Stats::totalInserts << " "
                      << Stats::totalSearches << " "
                      << Stats::totalReads << " "
                      << Stats::totalWrites << " "
                      << Stats::totalReorgReads << " "   // <--- NOWE
                      << Stats::totalReorgWrites         // <--- NOWE
                      << std::endl;
            break;
        }
        else if (cmd == "i") {
            uint32_t k, d;
            if (ss >> k >> d) {
                if (isam.insertRecord(k, d)) {
                    Stats::totalInserts++; // Zliczamy sukces
                    if (Logger::verbose || isatty(fileno(stdin)))
                        Logger::log("Inserted. R=%d W=%d\n", DiskManager::diskReads, DiskManager::diskWrites);
                } else {
                    std::cout << "Error: Key " << k << " already exists!\n";
                }
            }
        } 
        else if (cmd == "r") {
            uint32_t k;
            if (ss >> k) {
                isam.readRecord(k); // readRecord zwraca wskaźnik, ignorujemy tu
                Stats::totalSearches++;
            }
        } 
        // ... (u, d, p, b, x - bez zmian w logice, obsługa R/W na dole pętli) ...
        else if (cmd == "u") { uint32_t k, d; if (ss >> k >> d) isam.updateRecord(k, d); }
        else if (cmd == "d") { uint32_t k; if (ss >> k) isam.deleteRecord(k); }
        else if (cmd == "p") { isam.display(); }
        else if (cmd == "b") { isam.browse(); }
        else if (cmd == "x") { isam.reorganize(); }
        else if (cmd == "c") { 
            isam.clearDatabase(); 
            Stats::reset(); // Reset statystyk po czyszczeniu bazy
        }
        else if (cmd == "rnd") {
            int n;
            if (ss >> n) {
                std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<uint32_t> distKey(1, n * 10); 
                std::uniform_int_distribution<uint32_t> distData(1, 9999);
                
                int insertedCount = 0;
                while (insertedCount < n) {
                    uint32_t k = distKey(rng);
                    uint32_t d = distData(rng);
                    
                    int readsBefore = DiskManager::diskReads;
                    int writesBefore = DiskManager::diskWrites;

                    if (isam.insertRecord(k, d)) {
                        insertedCount++;
                        Stats::totalInserts++;
                    } else {
                        // Rollback liczników przy duplikacie
                        DiskManager::diskReads = readsBefore;
                        DiskManager::diskWrites = writesBefore;
                    }
                }
                Logger::log("Batch Insert Done. %d records.\n", insertedCount);
            }
        }
        else if (cmd == "srnd") { 
            int n;
            if (ss >> n) {
                std::mt19937 rng(std::random_device{}());
                // Zakładamy, że szukamy w zakresie, w którym wstawialiśmy (np. n*10)
                // lub po prostu losowych liczb, żeby sprawdzić też missy.
                // Aby test był miarodajny dla "znalezienia rekordu", lepiej żeby klucze istniały,
                // ale w ISAM koszt missa jest podobny do hita (przejście overflow).
                std::uniform_int_distribution<uint32_t> distKey(1, 100000); 
                
                int foundCount = 0;
                for(int i=0; i<n; i++) {
                    uint32_t k = distKey(rng);
                    if(isam.readRecord(k) != nullptr) foundCount++;
                    Stats::totalSearches++;
                }
                Logger::log("Batch Search Done. Found %d/%d.\n", foundCount, n);
            }
        }

        // === AGREGACJA STATYSTYK NA KOŃCU PĘTLI ===
        Stats::totalReads += DiskManager::diskReads;
        Stats::totalWrites += DiskManager::diskWrites;
    }

    return 0;
}