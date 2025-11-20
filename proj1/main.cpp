#include "tape.hpp"
#include "tapeSort.hpp"
#include <getopt.h>
#include <string>
#include "logger.hpp"

#define DEFAULT_FILENAME "tape_data.bin"

int main(int argc, char* argv[]){
    int opt;
    int long_index = 0;

    bool        verbose  = false;
    size_t      records  = 0;
    size_t      pageSize = 0;
    size_t      buffers  = 0;
    std::string filename = DEFAULT_FILENAME;

    static struct option long_opts[] = {
        {"help",        no_argument,        0,  'h'},
        {"file",        required_argument,  0,  'f'},
        {"records",     required_argument,  0,  'r'},
        {"pageSize",    required_argument,  0,  'p'},
        {"buffers",     required_argument,  0,  'b'},
        {"verbose",     no_argument,        0,  'v'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "hr:p:b:v", long_opts, &long_index)) != -1) {
        switch (opt) {
            case 'h':   // Help
                /// @todo write help for this program
                break;
            //==============These are exclusive to eachother=============
            case 'f':   // Source file for data
                filename = optarg;
                /// @todo write function for data loading from a file
                break;
            case 'r':   // Amount of records to generate
                records = std::stoi(optarg);
                break;
            //===========================================================
            case 'p':   // Size of a single page in a file
                pageSize = std::stoi(optarg);
                break;
            case 'b':   // Amount of large buffers
                buffers = std::stoi(optarg);
                break;
            case 'v':   // Shows sorting steps and phases
                verbose = true;
                Logger::verbose = true;  // Set Logger's verbose flag too
                break;
            default:
                return 1;
        }
    }

    Logger::log("=========================================\n"
                "File sorting with large buffer merging\n"
                "=========================================\n\n");

    Tape tape(filename, pageSize);

    /// @todo WRITE ERRORS IF pageSize or buffers ARE NOT SET

    if (filename != DEFAULT_FILENAME && records != 0) {
        /// @todo FILENAME AND RECORDS ARE EXCLUSIVE ERROR
    } else if (filename != DEFAULT_FILENAME) {
        if (!tape.open(std::ios::in | std::ios::out)) {
            Logger::log("Failed to open tape file!\n");
            return 1;
        }
        Logger::log("Successfully opened %s\n", filename.c_str());
    } else if (records != 0) {
        Logger::log("Generating random tape...\n");
        tape.generate_random_file(records);

        if (!tape.open(std::ios::in | std::ios::out)) {
            Logger::log("Failed to open tape file!\n");
            return 1;
        }
    } else {
        /// @todo FILENAME OR RECORDS OPTION IS REQUIRED ERROR
    }

    Logger::log("Initial tape content:\n");
    tape.display();
    Logger::log("\n");

    sort_tape(&tape, buffers);

    tape.close();

    return 0;
}
