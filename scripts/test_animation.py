#!/usr/bin/env python3
"""
Simple test to verify that PyQtGraph animations are working
"""
import sys
import numpy as np
from PyQt5 import QtWidgets, QtCore
from PyQt5.QtCore import QThread, QTime, QEventLoop
import pyqtgraph as pg

from Viz import Viz, VizUpdate

def test_simple_animation():
    print("Testing PyQtGraph animation visibility...")
    
    # Create visualizer with all components
    viz = Viz(plotting_flags=Viz.GRID_MASK | Viz.STATS_MASK)
    
    # Initialize with a small grid for easy viewing
    viz.init_grid(NROWS=3, NCOLS=3, title="Animation Test", use_color_map=True, num_nodes=3)
    viz.init_stats()
    
    # Show window
    viz.show()
    print("Window should be visible now...")
    
    # Simple test: move 3 dots around a 3x3 grid
    positions = [(0, 0), (1, 1), (2, 2)]
    
    print("Starting animation loop...")
    for step in range(15):
        if not viz.viz_active():
            print("Window closed by user")
            break
            
        # Create very obvious position changes
        new_positions = []
        for i, (x, y) in enumerate(positions):
            new_x = (x + 1) % 3
            new_y = (y + step // 5) % 3  # Change row every 5 steps
            new_positions.append((new_x, new_y))
        
        positions = new_positions
        
        # Create obvious stat changes
        time_val = step * 0.5
        temp_val = 100 - step * 5  # Decreasing temperature
        score_val = step * 10      # Increasing score
        
        update = VizUpdate(
            time_stamps=[time_val],
            temperatures=[temp_val],
            scores=[score_val],
            best_scores=[score_val],
            delta_scores=[10.0],
            acceptance_rates=[50.0],
            current_positions=positions,
            score=score_val
        )
        
        print(f"Step {step}: Positions={positions}, Score={score_val}, Temp={temp_val}")
        
        # Update and force processing
        viz.update(update)
        viz.app.processEvents()
        
        # Long pause to make changes very obvious
        viz.keep_alive(pause_time=1.0)  # 1 second per step
    
    print("Keeping window open for final inspection...")
    for i in range(50):  # 5 more seconds
        if not viz.viz_active():
            break
        viz.keep_alive(pause_time=0.1)
    
    print("Test complete!")

if __name__ == "__main__":
    test_simple_animation()