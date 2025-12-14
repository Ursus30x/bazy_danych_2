#!/bin/bash

echo "-------------------------- Running Basic Test --------------------------"
./isam -b 4 < test_basic.txt
echo -e "\n\n"

echo "-------------------------- Running Overflow Test --------------------------"
./isam -b 4 < test_overflow.txt
echo -e "\n\n"

echo "-------------------------- Running Reorg Test --------------------------"
./isam -b 4 < test_reorg.txt
echo -e "\n\n"

echo "-------------------------- Running Edge Cases Test --------------------------"
./isam -b 4 < test_edge_cases.txt
echo -e "\n\n"

echo "-------------------------- Running Reverse Test --------------------------"
./isam -b 4 < test_reverse.txt
echo -e "\n\n"

