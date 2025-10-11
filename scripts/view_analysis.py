
import os
import sys

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

def main(args):

    data_dir = args[0] if len(args) > 0 else "."
    figure_dir = args[1] if len(args) > 1 else "."

    if not os.path.isdir(data_dir):
        print(f"Error: '{data_dir}' is not a valid directory.")
        return 1

    if not os.path.isdir(figure_dir):
        print(f"Error: '{figure_dir}' is not a valid directory.")
        return 1

    # Find all CSV files in the directory
    csv_files = [f for f in os.listdir(data_dir) if f.endswith('.csv')]

    if not csv_files:
        print(f"No CSV files found in directory '{data_dir}'.")
        return 1
    
    print("Found CSV files:")
    for f in csv_files:
        print(f" - {f}")

    methods = [os.path.splitext(f)[0].replace('annealing_analysis_', '') for f in csv_files]
    data_frames = {method: pd.read_csv(os.path.join(data_dir, f)) for method, f in zip(methods, csv_files)}

    # Plotting - Create separate figures for each method
    for method, df in data_frames.items():
        plt.figure(figsize=(12, 6))
        x_transformed = -np.log10(1 - df['Cooling Rate'])
        
        # Score subplot
        plt.subplot(1, 2, 1)
        for col in df.columns:
            if 'score' in col.lower():
                plt.plot(x_transformed, df[col], "-o", label=f"{col}")
        plt.xlabel('Cooling Rate -(log10(1 - rate))')
        plt.ylabel('Score')
        plt.title(f'{method.title()}: Cooling Rate vs Score')
        plt.legend()
        
        # Time subplot  
        plt.subplot(1, 2, 2)
        for col in df.columns:
            if 'time' in col.lower():
                plt.plot(x_transformed, df[col], "-o", label=f"{col}")
        plt.xlabel('Cooling Rate -(log10(1 - rate))')
        plt.ylabel('Time (seconds)')
        plt.title(f'{method.title()}: Cooling Rate vs Time')
        plt.legend()
        
        plt.tight_layout()
        plt.savefig(f"{figure_dir}/{method}_cooling_v_time_and_quality.png")
        plt.clf()

    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))