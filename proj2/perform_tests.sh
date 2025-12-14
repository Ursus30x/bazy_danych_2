#!/bin/bash

# Define the parameter sets
RECORD_NUMS=(100 200 400 800 1600 3200 6400 12800) 
BUFFER_NUMS=(4 16)

# Create output file
OUTPUT_FILE="sorting_results.txt"
echo "RECORD_NUM BUFFER_NUM PHASES READ_COUNT WRITE_COUNT" > "$OUTPUT_FILE"

# Loop through all combinations
for RECORD_NUM in "${RECORD_NUMS[@]}"; do
    for BUFFER_NUM in "${BUFFER_NUMS[@]}"; do
        echo "Running: cpp/tape_sorting -r $RECORD_NUM -b $BUFFER_NUM -p 10 -v"
        
        # Execute the program and capture only the last 3 lines of output
        output=$(cpp/tape_sorting -r "$RECORD_NUM" -b "$BUFFER_NUM" -p 10 -v 2>/dev/null)
        
        # Extract the last 3 lines
        last_three_lines=$(echo "$output" | tail -n 3)
        
        # Parse the stats from the last three lines
        phases=$(echo "$last_three_lines" | head -n 1 | awk '{print $4}')
        read_count=$(echo "$last_three_lines" | head -n 2 | tail -n 1 | awk '{print $4}')
        write_count=$(echo "$last_three_lines" | tail -n 1 | awk '{print $4}')
        
        # Write to output file
        echo "$RECORD_NUM $BUFFER_NUM $phases $read_count $write_count" >> "$OUTPUT_FILE"
    done
done

echo "Results saved to $OUTPUT_FILE"
