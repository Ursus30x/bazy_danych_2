import pandas as pd
import matplotlib.pyplot as plt
import os
import sys

# Nazwa pliku z wynikami (generowany przez perform_tests.sh)
DATA_FILE = 'isam_experiments.txt'

# Sprawdzenie czy plik istnieje
if not os.path.exists(DATA_FILE):
    print(f"Błąd: Nie znaleziono pliku {DATA_FILE}. Uruchom najpierw ./perform_tests.sh")
    sys.exit(1)

# Wczytanie danych (separator to spacja)
try:
    df = pd.read_csv(DATA_FILE, sep=' ')
except Exception as e:
    print(f"Błąd podczas wczytywania danych: {e}")
    sys.exit(1)

# Utworzenie katalogu na wykresy
os.makedirs('charts', exist_ok=True)

# Obliczenie dodatkowych metryk
# Koszt operacyjny (czyste wyszukiwanie i wstawianie bez narzutu reorganizacji)
df['OPERATIONAL_READS'] = df['TOTAL_READS'] - df['REORG_READS']
df['OPERATIONAL_WRITES'] = df['TOTAL_WRITES'] - df['REORG_WRITES']

# Unikalne wartości Alpha do iteracji
alphas = sorted(df['ALPHA'].unique())

print(f"Znaleziono dane dla Alpha: {alphas}")

# ==========================================
# Wykres 1: Reorg Threshold vs Reorganization Amount
# Cel: Pokazać jak często musimy sprzątać w zależności od progu
# ==========================================
plt.figure(figsize=(10, 6))
for alpha in alphas:
    subset = df[df['ALPHA'] == alpha]
    # Sortowanie po threshold dla poprawności linii
    subset = subset.sort_values('THRESHOLD')
    plt.plot(subset['THRESHOLD'], subset['REORGS'], marker='o', linewidth=2, label=f'Alpha={alpha}')

plt.xlabel('Reorganization Threshold (V/N ratio)')
plt.ylabel('Number of Reorganizations')
plt.title('Impact of Threshold on Reorganization Frequency')
plt.legend()
plt.grid(True, alpha=0.3)
plt.savefig('charts/reorgs_vs_threshold.png', dpi=300)
plt.close()
print("Wygenerowano: charts/reorgs_vs_threshold.png")

# ==========================================
# Wykres 2: Reorg Threshold vs Read Amount (Trade-off)
# Cel: Pokazać "koszt utrzymania" vs "koszt operacyjny"
# Generujemy osobny wykres dla każdego Alpha (bo skale mogą być różne)
# ==========================================
for alpha in alphas:
    plt.figure(figsize=(10, 6))
    subset = df[df['ALPHA'] == alpha].sort_values('THRESHOLD')
    
    # Linia 1: Koszt utrzymania (Reorg Reads) - maleje wraz ze wzrostem threshold
    plt.plot(subset['THRESHOLD'], subset['REORG_READS'], 
             marker='s', color='red', linestyle='--', label='Maintenance (Reorg Reads)')
    
    # Linia 2: Koszt operacyjny (Operational Reads) - rośnie (lekko) wraz ze wzrostem threshold
    # (bo musimy skakać po dłuższych łańcuchach overflow)
    plt.plot(subset['THRESHOLD'], subset['OPERATIONAL_READS'], 
             marker='^', color='green', label='Operational (Search/Insert Reads)')
    
    # Linia 3: Całkowity koszt
    plt.plot(subset['THRESHOLD'], subset['TOTAL_READS'], 
             marker='o', color='blue', linewidth=2, label='Total Reads')

    plt.xlabel('Reorganization Threshold')
    plt.ylabel('Number of Disk Reads')
    plt.title(f'Reads Trade-off (Alpha={alpha})')
    plt.legend()
    plt.grid(True, alpha=0.3)
    
    # Formatowanie nazwy pliku
    filename = f'charts/reads_tradeoff_alpha_{str(alpha).replace(".", "_")}.png'
    plt.savefig(filename, dpi=300)
    plt.close()
    print(f"Wygenerowano: {filename}")

# ==========================================
# Wykres 3: Global Write Cost vs Threshold
# Cel: Pokazać, że częste sprzątanie jest bardzo kosztowne w zapisach
# ==========================================
plt.figure(figsize=(10, 6))
for alpha in alphas:
    subset = df[df['ALPHA'] == alpha].sort_values('THRESHOLD')
    plt.plot(subset['THRESHOLD'], subset['TOTAL_WRITES'], marker='o', linewidth=2, label=f'Alpha={alpha}')

plt.xlabel('Reorganization Threshold')
plt.ylabel('Total Writes')
plt.title('Impact of Threshold on Total Write Operations')
plt.legend()
plt.grid(True, alpha=0.3)
plt.savefig('charts/writes_vs_threshold.png', dpi=300)
plt.close()
print("Wygenerowano: charts/writes_vs_threshold.png")

print("Zakończono generowanie wykresów.")