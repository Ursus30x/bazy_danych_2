#include "isam.hpp"
#include <getopt.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include "logger.hpp"
#include "stats.hpp"
#include <unistd.h> 

#define DEFAULT_FILENAME_PREFIX "database"

// Zmienna globalna dla page.hpp
int RECORDS_PER_PAGE = 4; 

void print_menu() {
    // Wyświetl menu tylko jeśli wejście to terminal (a nie plik/pipe)
    if (isatty(fileno(stdin))) {
        std::cout << "\nCommands:\n"
                  << "  i <key> <data> : Insert record\n"
                  << "  r <key>        : Read record\n"
                  << "  u <key> <data> : Update record\n"
                  << "  d <key>        : Delete record\n"
                  << "  p              : Print structure (pages & overflow)\n"
                  << "  b              : Browse all records sequentially\n"
                  << "  x              : Force Reorganize\n"
                  << "  c              : Clear/Reset database\n"
                  << "  rnd <N>        : Insert N random records\n"
                  << "  srnd <N>       : Search N random records (Read Test)\n"
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
            case 'h':
                std::cout << "Usage: isam [OPTIONS] < commands.txt\n"
                          << "Options:\n"
                          << "  -h, --help            Show this help message\n"
                          << "  -f, --file PREF       Set filename prefix (default: database)\n"
                          << "  -b, --blocking N      Set records per page (default: 4)\n"
                          << "  -a, --alpha VAL       Set alpha factor (default: 0.5)\n"
                          << "  -t, --threshold VAL   Set reorg threshold (default: 0.2)\n"
                          << "  -v, --verbose         Enable verbose logging\n";
                return 0;
            case 'f': filenamePrefix = optarg; break;
            case 'a': alpha = std::stod(optarg); break;
            case 't': threshold = std::stod(optarg); break;
            case 'b': RECORDS_PER_PAGE = std::stoi(optarg); break;
            case 'v': verbose = true; break;
            default: return 1;
        }
    }

    if (RECORDS_PER_PAGE < 1) {
        std::cerr << "Error: Records per page must be >= 1\n";
        return 1;
    }

    Logger::verbose = verbose;
    
    // Logujemy parametry startowe (tylko w verbose lub trybie interaktywnym)
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

        // Resetowanie liczników dyskowych przed każdą komendą użytkownika
        DiskManager::diskReads = 0;
        DiskManager::diskWrites = 0;

        if (cmd == "q") {
            // Wypisanie statystyk w formacie dla skryptu Pythona
            std::cout << "STATS " 
                      << alpha << " " 
                      << threshold << " "
                      << Stats::totalReorgs << " "
                      << Stats::totalInserts << " "
                      << Stats::totalSearches << " "
                      << Stats::totalReads << " "
                      << Stats::totalWrites << " "
                      << Stats::totalReorgReads << " "  
                      << Stats::totalReorgWrites      
                      << std::endl;
            break;
        } 
        else if (cmd == "i") {
            uint32_t k, d;
            if (ss >> k >> d) {
                if (isam.insertRecord(k, d)) {
                    Stats::totalInserts++; 
                    if (Logger::verbose || isatty(fileno(stdin)))
                        Logger::log("Inserted. R=%d W=%d\n", DiskManager::diskReads, DiskManager::diskWrites);
                } else {
                    std::cout << "Error: Key " << k << " already exists!\n";
                }
            } else if (isatty(fileno(stdin))) std::cout << "Usage: i <key> <data>\n";
        } 
        else if (cmd == "r") {
            uint32_t k;
            if (ss >> k) {
                Record* r = isam.readRecord(k);
                if (r) std::cout << "Found: " << r->toString() << "\n";
                else std::cout << "Record not found.\n";
                
                Stats::totalSearches++;
                if (Logger::verbose || isatty(fileno(stdin)))
                    Logger::log("Disk Ops: R=%d W=%d\n", DiskManager::diskReads, DiskManager::diskWrites);
            }
        } 
        else if (cmd == "u") {
            uint32_t k, d;
            if (ss >> k >> d) {
                isam.updateRecord(k, d);
                if (Logger::verbose || isatty(fileno(stdin)))
                    Logger::log("Disk Ops: R=%d W=%d\n", DiskManager::diskReads, DiskManager::diskWrites);
            }
        }
        else if (cmd == "d") {
            uint32_t k;
            if (ss >> k) {
                if(isam.deleteRecord(k)) std::cout << "Deleted.\n";
                else std::cout << "Not found.\n";
                if (Logger::verbose || isatty(fileno(stdin)))
                    Logger::log("Disk Ops: R=%d W=%d\n", DiskManager::diskReads, DiskManager::diskWrites);
            }
        } 
        else if (cmd == "p") {
            isam.display();
        }
        else if (cmd == "b") {
            isam.browse();
        }
        else if (cmd == "x") {
            isam.reorganize();
            if (Logger::verbose || isatty(fileno(stdin)))
                Logger::log("Disk Ops (Reorg): R=%d W=%d\n", DiskManager::diskReads, DiskManager::diskWrites);
        }
        else if (cmd == "c") {
            isam.clearDatabase();
            Stats::reset(); // Reset statystyk po czyszczeniu
            if (Logger::verbose || isatty(fileno(stdin))) Logger::log("Database cleared.\n");
        }
        else if (cmd == "rnd") {
            int n;
            if (ss >> n) {
                std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<uint32_t> distKey(1, n * 10); 
                std::uniform_int_distribution<uint32_t> distData(1, 9999);
                
                if (isatty(fileno(stdin))) std::cout << "Inserting " << n << " unique random records...\n";
                
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
                Logger::log("Batch Insert Done. %d records. Disk Ops: R=%d W=%d\n", 
                            insertedCount, DiskManager::diskReads, DiskManager::diskWrites);
            }
        }
        else if (cmd == "srnd") { 
            int n;
            if (ss >> n) {
                std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<uint32_t> distKey(1, 100000); 
                
                if (isatty(fileno(stdin))) std::cout << "Searching " << n << " random records...\n";

                int foundCount = 0;
                for(int i=0; i<n; i++) {
                    uint32_t k = distKey(rng);
                    if(isam.readRecord(k) != nullptr) foundCount++;
                    Stats::totalSearches++;
                }
                Logger::log("Batch Search Done. Found %d/%d. Disk Ops: R=%d W=%d\n", 
                            foundCount, n, DiskManager::diskReads, DiskManager::diskWrites);
            }
        }
        // Ignorujemy nieznane komendy w pipe (np. puste linie), ale krzyczymy w terminalu
        else if (isatty(fileno(stdin))) {
            std::cout << "Unknown command.\n";
        }

        // Agregacja statystyk po każdej komendzie
        Stats::totalReads += DiskManager::diskReads;
        Stats::totalWrites += DiskManager::diskWrites;
    }

    return 0;
}