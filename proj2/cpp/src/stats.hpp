#pragma once

namespace Stats {
    extern long long totalReads;       // Całkowite odczyty (Suma)
    extern long long totalWrites;      // Całkowite zapisy (Suma)
    
    extern long long totalReorgReads;  // Odczyty tylko z reorganizacji
    extern long long totalReorgWrites; // Zapisy tylko z reorganizacji
    
    extern long long totalReorgs;
    extern long long totalInserts;
    extern long long totalSearches;
    
    void reset();
}