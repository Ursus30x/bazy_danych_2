#include "tape.hpp"
#include "tapeSort.hpp"
#include <getopt.h>
#include <string>
#include "logger.hpp"

#define DEFAULT_FILENAME "tape_data.bin"

int main(int argc, char* argv[]){
    int opt;
    int long_index = 0;

    size_t      records  = 0;
    size_t      pageSize = 0;
    size_t      buffers  = 0;
    std::string filename = DEFAULT_FILENAME;
    std::string loadFromFile = "";
    bool        loadFromKeyboard = false;

    static struct option long_opts[] = {
        {"help",        no_argument,        0,  'h'},
        {"file",        required_argument,  0,  'f'},
        {"records",     required_argument,  0,  'r'},
        {"pageSize",    required_argument,  0,  'p'},
        {"buffers",     required_argument,  0,  'b'},
        {"verbose",     no_argument,        0,  'v'},
        {"load-file",   required_argument,  0,  'l'},
        {"load-keyboard",no_argument,       0,  'k'},

        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "hr:p:b:vl:k", long_opts, &long_index)) != -1) {
        switch (opt) {
            case 'h':   // Help
                Logger::log("Usage: tape_sort [OPTIONS]\n"
                           "Options:\n"
                           "  -h, --help            Show this help message\n"
                           "  -f, --file FILE       Specify input file (default: tape_data.bin)\n"
                           "  -r, --records N       Generate N random records\n"
                           "  -p, --pageSize N      Set page size in records (default: 100)\n"
                           "  -b, --buffers N       Set number of buffers (default: 10)\n"
                           "  -v, --verbose         Enable verbose output\n"
                           "  -l, --load-file FILE  Load records from comma-separated text file\n"
                           "  -k, --load-keyboard   Load records from keyboard input\n"
                           "\n"
                           "Either specify a file or generate random records, not both.\n"
                           "If neither is specified, defaults to generating 1000 random records.\n");
                return 0;
            //==============These are exclusive to eachother=============
            case 'f':   // Source file for data
                filename = optarg;
                // Check if records was also specified
                if (records != 0) {
                    Logger::log("Error: Cannot specify both --file and --records\n");
                    return 1;
                }
                break;
            case 'r':   // Amount of records to generate
                records = std::stoi(optarg);
                // Check if file was also specified
                if (filename != DEFAULT_FILENAME) {
                    Logger::log("Error: Cannot specify both --file and --records\n");
                    return 1;
                }
                break;
            //===========================================================
            case 'p':   // Size of a single page in a file
                pageSize = std::stoi(optarg)*sizeof(time_record_type);
                break;
            case 'b':   // Amount of large buffers
                buffers = std::stoi(optarg);
                break;
            case 'v':   // Shows sorting steps and phases
                Logger::verbose = true;  // Set Logger's verbose flag too
                break;
            case 'l':   // Load from text file
                loadFromFile = optarg;
                if (filename != DEFAULT_FILENAME) {
                    Logger::log("Error: Cannot specify both --file and --load-file\n");
                    return 1;
                }
                if (records != 0) {
                    Logger::log("Error: Cannot specify both --records and --load-file\n");
                    return 1;
                }
                if (loadFromKeyboard) {
                    Logger::log("Error: Cannot specify both --load-keyboard and --load-file\n");
                    return 1;
                }
                break;
            case 'k':   // Load from keyboard
                loadFromKeyboard = true;
                if (filename != DEFAULT_FILENAME) {
                    Logger::log("Error: Cannot specify both --file and --load-keyboard\n");
                    return 1;
                }
                if (records != 0) {
                    Logger::log("Error: Cannot specify both --records and --load-keyboard\n");
                    return 1;
                }
                if (!loadFromFile.empty()) {
                    Logger::log("Error: Cannot specify both --load-file and --load-keyboard\n");
                    return 1;
                }
                break;
            default:
                return 1;
        }
    }

    Logger::log("=========================================\n"
                "File sorting with large buffer merging\n"
                "=========================================\n\n");

    // Check if pageSize or buffers are not set
    if (pageSize == 0) {
        Logger::log("Error: pageSize must be specified\n");
        return 1;
    }
    if (buffers == 0) {
        Logger::log("Error: buffers must be specified\n");
        return 1;
    }

    Tape tape(filename, pageSize);

    // Check if filename and records are both specified
    if (!loadFromFile.empty()) {
        Logger::log("Loading from text file: %s\n", loadFromFile.c_str());
        tape.load_txt_file(loadFromFile);

    } else if (loadFromKeyboard) {
        Logger::log("Loading from keyboard input...\n");
        tape.load_records_from_keyboard();
    
    } else if (filename != DEFAULT_FILENAME && records != 0) {
        Logger::log("Error: Cannot specify both --file and --records\n");
        return 1;
    } else if (filename != DEFAULT_FILENAME) {
        Logger::log("Opening %s\n", filename.c_str());
    } else if (records != 0) {
        Logger::log("Generating random tape...\n\n");
        tape.generate_random_file(records);
    } else {
        // Neither filename nor records specified - use default
        Logger::log("No input specified, generating 1000 random records...\n\n");
        tape.generate_random_file(1000);  // Use 1000 as default
    }

    Logger::log("Initial tape content:\n");
    tape.display();
    Logger::log("\n");

    sort_tape(&tape, buffers);

    tape.close();

    return 0;
}
