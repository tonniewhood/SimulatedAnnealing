
import sys

import numpy as np

from dataclasses import dataclass
from PyQt5 import QtWidgets, QtCore
from PyQt5.QtCore import QThread, QEventLoop

from AnimatedGrid import AnimatedGrid   # new fast impl
from StatsGraph import StatsGraph       # new fast impl
from BasicDigraph import BasicDigraph   # new fast impl

@dataclass
class VizUpdate:
    time_stamps: list[float]
    temperatures: list[float]
    scores: list[float]
    best_scores: list[float]
    delta_scores: list[float]
    acceptance_rates: list[float]
    current_positions: list
    score: float | str = "N/A"

class Viz:
    GRID_MASK = 0x01
    GRAPH_MASK = 0x02
    STATS_MASK = 0x04
    ALL_MASK = 0x07

    def __init__(self, plotting_flags=0, close_callback=None):
        if not (plotting_flags & self.ALL_MASK):
            raise ValueError("At least one of GRID/GRAPH/STATS must be set")

        self.plotting_flags = plotting_flags
        self.close_callback = close_callback
        self.shutdown_requested = False

        # Ensure QApplication exists and configure for better threading
        if not QtWidgets.QApplication.instance():
            self.app = QtWidgets.QApplication(sys.argv)
            self.app.setAttribute(QtCore.Qt.AA_X11InitThreads, True)
        else:
            self.app = QtWidgets.QApplication.instance()
            
        self.win = QtWidgets.QMainWindow()
        self.win.setWindowTitle("Simulated Annealing")
        central = QtWidgets.QWidget()
        self.hbox = QtWidgets.QHBoxLayout(central)
        self.hbox.setContentsMargins(6, 6, 6, 6)
        self.hbox.setSpacing(8)
        self.win.setCentralWidget(central)

        # placeholders for subviews (created in init_* like before)
        self.grid = None
        self.graph = None
        self.stats = None

        # close handling
        self.win.closeEvent = self._on_close_event

    def init_grid(self, NROWS, NCOLS, num_nodes, title="Simulated Annealing Progress", use_color_map=True, size=(400, 400)):
        if not (self.plotting_flags & self.GRID_MASK):
            raise RuntimeError("GRID_MASK not set")
        self.grid = AnimatedGrid(NROWS, NCOLS, title, use_color_map, num_nodes, size)
        self.hbox.addWidget(self.grid.widget)

    def init_graph(self, edges, size=(400, 400)):
        if not (self.plotting_flags & self.GRAPH_MASK):
            raise RuntimeError("GRAPH_MASK not set")
        self.graph = BasicDigraph(edges, size)
        self.hbox.addWidget(self.graph.widget)

    def init_stats(self, size=(500, 400)):
        if not (self.plotting_flags & self.STATS_MASK):
            raise RuntimeError("STATS_MASK not set")
        self.stats = StatsGraph(title="Simulated Annealing Stats", size=size)
        self.hbox.addWidget(self.stats.widget)

    def show(self):
        # Show and raise window to ensure it's visible
        self.win.show()
        self.win.raise_()
        self.win.activateWindow()
        
        # Process events to ensure window is displayed
        self.app.processEvents()
        print(f"Window shown. Size: {self.win.size().width()}x{self.win.size().height()}")

    def keep_alive(self, pause_time=0.1):
        # Process events multiple times for smoother updates
        import time
        start_time = time.time()
        end_time = start_time + pause_time
        
        while time.time() < end_time:
            self.app.processEvents(QEventLoop.AllEvents, 50)
            QThread.msleep(10)  # Small sleep to prevent CPU spinning

    def on_close(self, _):
        if self.close_callback:
            self.close_callback()
        self.shutdown_requested = True

    def _on_close_event(self, evt):
        self.on_close(evt)
        evt.accept()

    def update(self, update: VizUpdate):
        if self.grid:
            self.grid.update_positions(update.current_positions, score=update.score)
        if self.stats:
            self.stats.update_stats(
                time_stamps=update.time_stamps,
                temperatures=update.temperatures,
                scores=update.scores,
                best_scores=update.best_scores,
                delta_scores=update.delta_scores,
                acceptance_rates=update.acceptance_rates
            )

    def viz_active(self):
        return not self.shutdown_requested

    def save_figs(self, grid_animation_filename=None, grid_static_filename=None, graph_filename=None, stats_filenames=None):
        if self.plotting_flags & self.GRID_MASK and self.grid and grid_animation_filename is not None and grid_static_filename is not None:
            self.grid.save_gif(filename=grid_animation_filename)
            self.grid.save_final_state(filename=grid_static_filename)
            print(f"Grid figure saved as '{grid_animation_filename}'")
        if self.plotting_flags & self.GRAPH_MASK and self.graph and graph_filename is not None:
            self.graph.save(graph_filename)
        if self.plotting_flags & self.STATS_MASK and self.stats and stats_filenames is not None:
            self.stats.save_figs(stats_filenames)

if __name__ == "__main__":

    viz = Viz(plotting_flags=Viz.GRID_MASK | Viz.GRAPH_MASK | Viz.STATS_MASK, close_callback=lambda: print("Closed callback invoked."))

    # Initialize grid
    viz.init_grid(NROWS=5, NCOLS=5, title="Simulated Annealing Progress", use_color_map=True, num_nodes=5)

    # Initialize graph
    edges = [(0, 2), (1, 2), (2, 3), (3, 4), (3, 5)]
    viz.init_graph(edges, )

    # Initialize stats
    viz.init_stats()

    # Show the visualizations first
    viz.show()

    # Initialize simulation state
    current_positions = [(0, 0), (1, 1), (2, 2), (3, 3), (4, 4)]
    
    # Accumulating data for stats
    all_timestamps = []
    all_temperatures = []
    all_scores = []
    all_best_scores = []
    all_delta_scores = []
    all_acceptance_rates = []
    
    # Simulation loop
    for idx in range(20):  # More iterations to see changes
        if not viz.viz_active():
            break

        # Update simulation state
        current_time = idx * 0.5
        temperature = 100.0 * (0.95 ** idx)  # Cooling temperature
        score = 50.0 + 30.0 * np.sin(idx * 0.3) + np.random.normal(0, 5)  # Varying score
        best_score = min(all_scores + [score]) if all_scores else score
        delta_score = score - (all_scores[-1] if all_scores else score)
        acceptance_rate = max(0, 100 * np.exp(-abs(delta_score) / max(temperature, 0.1)))
        
        # Accumulate stats data
        all_timestamps.append(current_time)
        all_temperatures.append(temperature)
        all_scores.append(score)
        all_best_scores.append(best_score)
        all_delta_scores.append(delta_score)
        all_acceptance_rates.append(acceptance_rate)
        
        # Move positions in a more interesting pattern
        current_positions = [((x + 1) % 5, (y + idx // 4) % 5) for x, y in current_positions]
        
        # Create update with ONLY the new data point for this iteration
        update = VizUpdate(
            time_stamps=[current_time],  # Single new timestamp
            temperatures=[temperature],   # Single new temperature
            scores=[score],              # Single new score
            best_scores=[best_score],    # Single new best score
            delta_scores=[delta_score],  # Single new delta
            acceptance_rates=[acceptance_rate],  # Single new rate
            current_positions=current_positions, 
            score=score
        )
        
        # Update visualizations
        viz.update(update)
        
        # Force immediate GUI update
        viz.app.processEvents()
        
        # Give time to see the update
        viz.keep_alive(pause_time=0.5)  # Even longer pause to see changes clearly
        
        print(f"Step {idx}: Score={score:.1f}, Temp={temperature:.1f}, Pos={current_positions[0]}")

    print("\nAnimation complete! Window will stay open for 10 seconds...")
    print("You can also close the window manually to continue.")
    
    # Keep window open for 10 seconds after animation
    for i in range(100):  # 10 seconds = 100 * 0.1s
        if not viz.viz_active():
            break
        viz.keep_alive(pause_time=0.1)

    viz.save_figs(grid_animation_filename="grid_animation.gif", 
                  grid_static_filename="grid_static.png", 
                  graph_filename="final_graph.png", 
                  stats_filenames=[
                      "temp.png", 
                      "score.png",  # Added missing comma
                      "best_score.png",
                      "delta_score.png",
                      "acceptance_rate.png"
                ])