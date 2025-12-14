#!/bin/bash

# Konfiguracja wielkości eksperymentu
RECORD_NUM=5000   # Liczba wstawianych rekordów
SEARCH_NUM=1000   # Liczba wyszukiwań (do testu odczytu)
BLOCKING_FACTOR=4 # Domyślny blocking factor (zgodnie z instrukcją)

# Parametry do badania
# Alpha: Sprawdzimy standardowe 0.5 oraz "ciasne" 0.9
ALPHAS=(0.5 0.9)

# Threshold: Gęste próbkowanie dla małych wartości (gdzie dzieje się najwięcej)
# oraz rzadsze dla dużych.
THRESHOLDS=(0.05 0.1 0.15 0.2 0.3 0.4 0.6 0.8 1.0)

# Plik wynikowy
OUTPUT_FILE="isam_experiments.txt"

# Nagłówek pliku wynikowego (musi pasować do kolejności w main.cpp -> STATS)
echo "ALPHA THRESHOLD REORGS INSERTS SEARCHES TOTAL_READS TOTAL_WRITES REORG_READS REORG_WRITES" > "$OUTPUT_FILE"

echo "=================================================="
echo "Starting ISAM Experiments"
echo "Records: $RECORD_NUM, Searches: $SEARCH_NUM, B: $BLOCKING_FACTOR"
echo "Results will be saved to: $OUTPUT_FILE"
echo "=================================================="

# Pętla po parametrach
for alpha in "${ALPHAS[@]}"; do
    for thresh in "${THRESHOLDS[@]}"; do
        echo -n "Testing Alpha=$alpha Threshold=$thresh ... "
        
        # Przygotowanie ciągu komend dla programu:
        # 1. c          -> Wyczyść bazę (start od zera)
        # 2. rnd N      -> Wstaw N losowych rekordów (generuje Writes i Reorgs)
        # 3. srnd M     -> Wyszukaj M losowych rekordów (generuje Reads)
        # 4. q          -> Wyjdź i wypisz STATS
        commands=$(printf "c\nrnd %d\nsrnd %d\nq\n" $RECORD_NUM $SEARCH_NUM)
        
        # Uruchomienie programu
        # Przekazujemy komendy przez potok (pipe)
        # stderr (logi verbose) przekierowujemy do /dev/null, żeby nie śmiecić
        output=$(echo "$commands" | cpp/isam -b $BLOCKING_FACTOR -a $alpha -t $thresh 2>/dev/null)
        
        # Wyciągnięcie linii ze statystykami (zaczyna się od "STATS")
        stats_line=$(echo "$output" | grep "^STATS")
        
        if [ ! -z "$stats_line" ]; then
            # Usunięcie słowa "STATS" z początku linii (cut od 2 pola)
            clean_stats=$(echo "$stats_line" | cut -d' ' -f2-)
            
            # Zapis do pliku
            echo "$clean_stats" >> "$OUTPUT_FILE"
            echo "OK"
        else
            echo "FAILED (No stats output)"
        fi
    done
done

echo "=================================================="
echo "Experiments finished."