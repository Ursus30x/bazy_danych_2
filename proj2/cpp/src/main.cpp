#include "isam.hpp"
#include <getopt.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include "logger.hpp"

#define DEFAULT_FILENAME_PREFIX "database"

void print_menu() {
    std::cout << "\nCommands:\n"
              << "  i <key> <data> : Insert record\n"
              << "  r <key>        : Read record\n"
              << "  u <key> <data> : Update record\n"
              << "  d <key>        : Delete record\n"
              << "  p              : Print structure (pages & overflow)\n"
              << "  b              : Browse all records sequentially\n"
              << "  x              : Reorganize file\n"
              << "  rnd <N>        : Insert N random records\n"
              << "  q              : Quit\n"
              << "> ";
}

int main(int argc, char* argv[]) {
    int opt;
    std::string filenamePrefix = DEFAULT_FILENAME_PREFIX;
    std::string loadFromFile = "";
    double alpha = 0.5;
    bool verbose = false;

    static struct option long_opts[] = {
        {"help",        no_argument,        0,  'h'},
        {"file",        required_argument,  0,  'f'},
        {"load",        required_argument,  0,  'l'},
        {"alpha",       required_argument,  0,  'a'},
        {"verbose",     no_argument,        0,  'v'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "hf:l:a:v", long_opts, nullptr)) != -1) {
        switch (opt) {
            case 'h':
                std::cout << "Usage: isam [OPTIONS]\n"
                          << "  -f, --file PREF   Set filename prefix (default: database)\n"
                          << "  -l, --load FILE   Load initial data from text file (key data)\n"
                          << "  -a, --alpha VAL   Set alpha factor for reorganization (default: 0.5)\n"
                          << "  -v, --verbose     Enable verbose logging\n";
                return 0;
            case 'f': filenamePrefix = optarg; break;
            case 'l': loadFromFile = optarg; break;
            case 'a': alpha = std::stod(optarg); break;
            case 'v': verbose = true; break;
            default: return 1;
        }
    }

    Logger::verbose = verbose;
    Logger::log("Initializing ISAM on files: %s_*.bin\n", filenamePrefix.c_str());

    ISAM isam(filenamePrefix, alpha);

    // Opcjonalne ładowanie z pliku na start
    if (!loadFromFile.empty()) {
        std::ifstream infile(loadFromFile);
        if (infile.is_open()) {
            uint32_t k, d;
            int count = 0;
            while (infile >> k >> d) {
                isam.insertRecord(k, d);
                count++;
            }
            Logger::log("Loaded %d records from %s\n", count, loadFromFile.c_str());
        } else {
            Logger::log("Error: Cannot open file %s\n", loadFromFile.c_str());
        }
    }

    // Pętla interaktywna
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
            break;
        } 
        else if (cmd == "i") {
            uint32_t k, d;
            if (ss >> k >> d) {
                // Teraz sprawdzamy wynik operacji
                if (isam.insertRecord(k, d)) {
                    Logger::log("Inserted. Disk Ops: R=%d W=%d\n", DiskManager::diskReads, DiskManager::diskWrites);
                } else {
                    std::cout << "Error: Key " << k << " already exists!\n";
                }
            } else std::cout << "Usage: i <key> <data>\n";
        }
        else if (cmd == "r") {
            uint32_t k;
            if (ss >> k) {
                Record* r = isam.readRecord(k);
                if (r) std::cout << "Found: " << r->toString() << "\n";
                else std::cout << "Record not found.\n";
                Logger::log("Disk Ops: R=%d W=%d\n", DiskManager::diskReads, DiskManager::diskWrites);
            }
        } 
        else if (cmd == "u") {
            uint32_t k, d;
            if (ss >> k >> d) {
                isam.updateRecord(k, d);
                Logger::log("Disk Ops: R=%d W=%d\n", DiskManager::diskReads, DiskManager::diskWrites);
            }
        }
        else if (cmd == "d") {
            uint32_t k;
            if (ss >> k) {
                if(isam.deleteRecord(k)) std::cout << "Deleted.\n";
                else std::cout << "Not found.\n";
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
            Logger::log("Disk Ops (Reorg): R=%d W=%d\n", DiskManager::diskReads, DiskManager::diskWrites);
        }
        else if (cmd == "rnd") {
            int n;
            if (ss >> n) {
                std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<uint32_t> distKey(1, 100); // Mały zakres wymusi kolizje
                std::uniform_int_distribution<uint32_t> distData(1, 100);
                
                std::cout << "Inserting " << n << " unique random records...\n";
                
                int insertedCount = 0;
                // Pętla wykonuje się dopóki nie wstawimy n UNIKALNYCH rekordów
                while (insertedCount < n) {
                    uint32_t k = distKey(rng);
                    uint32_t d = distData(rng);
                    
                    // Jeśli insert zwróci true, zwiększamy licznik.
                    // Jeśli false (duplikat), pętla kręci się dalej z nowym losowaniem.
                    if (isam.insertRecord(k, d)) {
                        insertedCount++;
                    }
                }
                
                Logger::log("Batch complete. Inserted %d records. Disk Ops: R=%d W=%d\n", 
                            insertedCount, DiskManager::diskReads, DiskManager::diskWrites);
            }
        }
        else {
            std::cout << "Unknown command.\n";
        }
    }

    return 0;
}