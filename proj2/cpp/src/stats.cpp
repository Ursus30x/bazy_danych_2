#include "stats.hpp"

namespace Stats {
    long long totalReads = 0;
    long long totalWrites = 0;
    long long totalReorgReads = 0;
    long long totalReorgWrites = 0;
    long long totalReorgs = 0;
    long long totalInserts = 0;
    long long totalSearches = 0;

    void reset() {
        totalReads = 0;
        totalWrites = 0;
        totalReorgReads = 0;
        totalReorgWrites = 0;
        totalReorgs = 0;
        totalInserts = 0;
        totalSearches = 0;
    }
}