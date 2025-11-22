#pragma once

#include "tape.hpp"
#include <algorithm>
#include <iostream>

void create_runs(Tape *tape, size_t bufferNumber);
void merge(Tape *tape, size_t bufferNumber);
void sort_tape(Tape *tape, size_t bufferNumber);
