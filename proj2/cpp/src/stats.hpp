#pragma once

namespace Stats {
    extern long long totalReads;  
    extern long long totalWrites;
    
    extern long long totalReorgReads; 
    extern long long totalReorgWrites; 
    
    extern long long totalReorgs;
    extern long long totalInserts;
    extern long long totalSearches;
    
    void reset();
}