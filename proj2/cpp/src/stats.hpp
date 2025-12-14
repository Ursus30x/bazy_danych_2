#pragma once

namespace Stats {
    extern long long totalReads;
    extern long long totalWrites;
    extern long long totalReorgs;
    extern long long totalInserts;
    extern long long totalSearches;
    
    // Funkcja resetująca wszystkie liczniki (np. przy komendzie clear)
    void reset();
}