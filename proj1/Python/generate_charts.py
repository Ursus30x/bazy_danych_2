import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

# Read the data from the file
df = pd.read_csv('sorting_results.txt', sep=' ')

# Automatically detect buffer numbers from the data
buffer_nums = sorted(df['BUFFER_NUM'].unique().tolist())

# Create output directory for charts
os.makedirs('charts', exist_ok=True)

def calculate_theoretical_phases(N, n, b):
    """Calculate theoretical phases using log_n(r) where r = ceil(N/(n*b))"""
    r = np.ceil(N / (n * b))
    # Handle case where r <= 0 to avoid log issues
    r = np.maximum(r, 1)
    return np.ceil(np.log(r) / np.log(n))

def calculate_theoretical_disk_ops(N, n, b):
    """Calculate theoretical disk operations using 2*N/b * (1 + phase_amount)"""
    r = np.ceil(N / (n * b))
    # Handle case where r <= 0 to avoid log issues
    r = np.maximum(r, 1)
    phases = np.log(r) / np.log(n)
    return 2 * N / b * (1 + phases)

def calculate_theoretical_disk_ops_simple(N, n, b):
    """Alternative simpler calculation for disk operations"""
    # For simple estimation: 2*N/b * (1 + log_n(N/(n*b)))
    r = np.ceil(N / (n * b))
    r = np.maximum(r, 1)
    phases = np.log(r) / np.log(n)
    return 2 * N / b * (1 + phases)

# Process each buffer number
for buffer_num in buffer_nums:
    # Filter data for current buffer number
    buffer_data = df[df['BUFFER_NUM'] == buffer_num]
    
    # Create figures for both charts
    fig1, ax1 = plt.subplots(figsize=(10, 6))
    fig2, ax2 = plt.subplots(figsize=(10, 6))
    fig3, ax3 = plt.subplots(figsize=(10, 6))
    
    # Get blocking factor from data (assuming it's consistent for each buffer)
    # If not available in the data, you can set it manually
    blocking_factor = 10  # You might want to extract this from your data or set it
    
    # Chart 1: Phases vs Record Numbers - Actual vs Theoretical
    ax1.plot(buffer_data['RECORD_NUM'], buffer_data['PHASES'], marker='o', linewidth=2, markersize=8, label='Actual')
    
    # Add theoretical curve
    N_vals = buffer_data['RECORD_NUM'].values
    theoretical_phases = calculate_theoretical_phases(N_vals, buffer_num, blocking_factor)
    ax1.plot(N_vals, theoretical_phases, marker='x', linewidth=2, markersize=8, 
             linestyle='--', label='Theoretical')
    
    ax1.set_xlabel('Record Count')
    ax1.set_ylabel('Number of Phases')
    ax1.set_title(f'Phases vs Record Count (Buffer Size: {buffer_num})')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Add some styling
    ax1.tick_params(axis='x', rotation=45)
    
    # Chart 2: Disk Operations vs Record Numbers - Actual vs Theoretical
    total_disk_ops = buffer_data['READ_COUNT'] + buffer_data['WRITE_COUNT']
    ax2.plot(buffer_data['RECORD_NUM'], total_disk_ops, marker='s', linewidth=2, markersize=8, 
             color='orange', label='Actual')
    
    # Add theoretical curve for disk operations
    theoretical_disk_ops = calculate_theoretical_disk_ops(N_vals, buffer_num, blocking_factor)
    ax2.plot(N_vals, theoretical_disk_ops, marker='d', linewidth=2, markersize=8,
             linestyle='--', color='red', label='Theoretical')
    
    ax2.set_xlabel('Record Count')
    ax2.set_ylabel('Total Disk Operations')
    ax2.set_title(f'Total Disk Operations vs Record Count (Buffer Size: {buffer_num})')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    # Add some styling
    ax2.tick_params(axis='x', rotation=45)

    # Chart 3: phases vs Record Numbers - Log scale with theoretical
    ax3.plot(buffer_data['RECORD_NUM'], buffer_data['PHASES'], marker='o', linewidth=2, markersize=8, label='Actual')
    
    # Add theoretical curve for log scale chart
    ax3.plot(N_vals, theoretical_phases, marker='x', linewidth=2, markersize=8,
             linestyle='--', label='Theoretical')
    
    ax3.set_xlabel('Record Count')
    ax3.set_ylabel('Number of Phases')
    ax3.set_title(f'Phases vs Record Count (Buffer Size: {buffer_num})')
    ax3.legend()
    ax3.grid(True, alpha=0.3)
    
    # Add some styling
    ax3.set_xscale('log', base=10)  # Log scale for better visualization
    ax3.tick_params(axis='x', rotation=45)
    
    # Adjust layout and save
    plt.tight_layout()
    
    # Save charts
    fig1.savefig(f'charts/phases_vs_records_buffer_{buffer_num}.png', dpi=300, bbox_inches='tight')
    fig2.savefig(f'charts/disk_ops_vs_records_buffer_{buffer_num}.png', dpi=300, bbox_inches='tight')
    fig3.savefig(f'charts/phases_vs_records_buffer_{buffer_num}_log.png', dpi=300, bbox_inches='tight')
    
    # Close figures to free memory
    plt.close(fig1)
    plt.close(fig2)
    plt.close(fig3)

    print(f"Generated charts for buffer size {buffer_num}")

# Create combined charts for comparison with theoretical values
fig3, (ax3, ax4) = plt.subplots(2, 1, figsize=(12, 10))

# Combined phases chart - All buffer sizes with theoretical
for buffer_num in buffer_nums:
    buffer_data = df[df['BUFFER_NUM'] == buffer_num]
    ax3.plot(buffer_data['RECORD_NUM'], buffer_data['PHASES'], 
             marker='o', linewidth=2, markersize=8, label=f'Actual Buffer {buffer_num}')
    
    # Add theoretical curve for each buffer size
    N_vals = buffer_data['RECORD_NUM'].values
    blocking_factor = 10  # Set appropriate value or extract from data
    theoretical_phases = calculate_theoretical_phases(N_vals, buffer_num, blocking_factor)
    ax3.plot(N_vals, theoretical_phases, 
             linestyle='--', linewidth=2, label=f'Theoretical Buffer {buffer_num}')

ax3.set_xlabel('Record Count')
ax3.set_ylabel('Number of Phases')
ax3.set_title('Phases vs Record Count (All Buffer Sizes)')
ax3.legend()
ax3.grid(True, alpha=0.3)
ax3.set_xscale('log', base=10)
ax3.tick_params(axis='x', rotation=45)

# Combined Disk Operations chart - All buffer sizes with theoretical
for buffer_num in buffer_nums:
    buffer_data = df[df['BUFFER_NUM'] == buffer_num]
    total_disk_ops = buffer_data['READ_COUNT'] + buffer_data['WRITE_COUNT']
    ax4.plot(buffer_data['RECORD_NUM'], total_disk_ops, 
             marker='s', linewidth=2, markersize=8, label=f'Actual Buffer {buffer_num}')
    
    # Add theoretical curve for disk operations
    N_vals = buffer_data['RECORD_NUM'].values
    blocking_factor = 10  # Set appropriate value or extract from data
    theoretical_disk_ops = calculate_theoretical_disk_ops(N_vals, buffer_num, blocking_factor)
    ax4.plot(N_vals, theoretical_disk_ops, 
             linestyle='--', linewidth=2, label=f'Theoretical Buffer {buffer_num}')

ax4.set_xlabel('Record Count')
ax4.set_ylabel('Total Disk Operations')
ax4.set_title('Total Disk Operations vs Record Count (All Buffer Sizes)')
ax4.legend()
ax4.grid(True, alpha=0.3)
ax4.set_xscale('log', base=10)
ax4.tick_params(axis='x', rotation=45)

plt.tight_layout()
plt.savefig('charts/combined_comparison.png', dpi=300, bbox_inches='tight')
plt.close()

print(f"Generated combined comparison charts for buffer sizes: {buffer_nums}")
print("Charts saved to 'charts/' directory")
