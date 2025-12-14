#!/bin/bash

# Wejdź do katalogu cpp i zbuduj projekt
cd cpp
make

# Sprawdź czy kompilacja się powiodła
if [ $? -eq 0 ]; then
    echo "Kompilacja zakończona sukcesem."
else
    echo "Błąd podczas kompilacji!"
    exit 1
fi

# Wyjdź z katalogu cpp i uruchom perform_tests.sh
cd ..
./perform_tests.sh

# Przejdź do katalogu Python i uruchom generate_charts.py

python3 Python/generate_charts.py

# # Wróć do głównego katalogu i skompiluj sprawozdanie w LaTeX

cd LaTeX
pdflatex sprawozdanie.tex
pdflatex sprawozdanie.tex

echo "Wszystkie zadania wykonane pomyślnie."
