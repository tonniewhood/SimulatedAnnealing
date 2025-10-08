

import matplotlib.pyplot as plt

from dataclasses import dataclass

from AnimatedGrid import AnimatedGrid
from BasicDigraph import BasicDigraph

@dataclass
class VizUpdate:
    current_positions: list
    score: float | str = "N/A"


class Viz:

    grid: AnimatedGrid | None = None
    graph: BasicDigraph | None = None
    sequence: list = []
    plotting_flags: int = 0x00
    close_callback: callable = None
    figure: plt.Figure = None
    axes: list[plt.Axes] = None
    axis_iter: iter = None
    shutdown_requested: bool = False

    GRID_MASK = 0x01
    GRAPH_MASK = 0x02
    STATS_MASK = 0x04
    ALL_MASK = 0x07

    def __init__(self, plotting_flags=0, close_callback=None):

        if not (plotting_flags & self.ALL_MASK):
            raise ValueError("At least one of GRID_MASK, GRAPH_MASK, or STATS_MASK must be set in plotting_flags")
        
        num_plots = bin(plotting_flags).count("1")
        self.figure, self.axes = plt.subplots(1, num_plots, figsize=(10 * num_plots, 8))
        if num_plots == 1:
            self.axes = [self.axes]  # Ensure axes is always a list

        self.plotting_flags = plotting_flags
        self.axis_iter = iter(self.axes)

        # Connect close event
        self.figure.canvas.mpl_connect('close_event', self.on_close)
        self.close_callback = close_callback

    def init_grid(self, 
                  NROWS, 
                  NCOLS, 
                  num_nodes,
                  title="Simulated Annealing Progress", 
                  use_color_map=True):

        if self.plotting_flags & self.GRID_MASK:
            self.grid = AnimatedGrid(NROWS, NCOLS, title, use_color_map, num_nodes, self.figure, next(self.axis_iter, None))
        else:
            raise RuntimeError("GRID_MASK not set in plotting_flags; cannot initialize grid.")
        
    def init_graph(self, edges):
        if self.plotting_flags & self.GRAPH_MASK:
            self.graph = BasicDigraph(edges, self.figure, next(self.axis_iter, None))
        else:
            raise RuntimeError("GRAPH_MASK not set in plotting_flags; cannot initialize graph.")

    def init_stats(self):
        raise NotImplementedError("Statistics plotting not yet implemented.")

    def show(self):

        if self.plotting_flags & self.GRID_MASK:
            if not self.grid:
                raise RuntimeError("GRID_MASK set but grid not initialized.")

            self.grid.live_update_mode()

        if self.plotting_flags & self.GRAPH_MASK:
            if not self.graph:
                raise RuntimeError("GRAPH_MASK set but graph not initialized.")

            self.graph.display()

        if self.plotting_flags & self.STATS_MASK:
            raise RuntimeError("STATS_MASK set but stats not initialized.")
        
    def keep_alive(self, pause_time=0.1):
        plt.pause(pause_time)
        plt.show(block=False)

    def on_close(self, _):
        if self.close_callback:
            self.close_callback()
        self.shutdown_requested = True

    def update(self, update: VizUpdate):

        if self.plotting_flags & self.GRID_MASK:
            if not self.grid:
                raise RuntimeError("GRID_MASK set but grid not initialized.")
            self.grid.update_positions(update.current_positions, score=update.score)
            self.sequence.append((update.score, update.current_positions))
        else:
            raise RuntimeError("GRID_MASK not set in plotting_flags; cannot update grid.")

    def viz_active(self):
        return not self.shutdown_requested
    
    def save_figs(self, grid_animation_filename=None, grid_static_filename=None, graph_filename=None, stats_filename=None):
        if self.plotting_flags & self.GRID_MASK and self.grid and grid_animation_filename is not None and grid_static_filename is not None:
            self.grid.save_gif(filename=grid_animation_filename)
            self.grid.save_final_state(filename=grid_static_filename)
            print(f"Grid figure saved as '{grid_animation_filename}'")
        if self.plotting_flags & self.GRAPH_MASK and self.graph and graph_filename is not None:
            self.graph.save(graph_filename)
        if self.plotting_flags & self.STATS_MASK and self.stats and stats_filename is not None:
            raise NotImplementedError("Statistics saving not yet implemented.")

if __name__ == "__main__":

    viz = Viz(plotting_flags=Viz.GRID_MASK | Viz.GRAPH_MASK, close_callback=lambda: print("Closed callback invoked."))

    # Initialize grid
    viz.init_grid(NROWS=5, NCOLS=5, title="Simulated Annealing Progress", use_color_map=True, num_nodes=5)

    # Initialize graph
    edges = [(0, 2), (1, 2), (2, 3), (3, 4), (3, 5)]
    viz.init_graph(edges)

    current_positions = [(0, 0), (1, 1), (2, 2), (3, 3), (4, 4)]
    update = VizUpdate(current_positions=current_positions, score=0.0)
    # Show the visualizations
    viz.show()

    for idx in range(11):
        if not viz.viz_active():
            break

        # Simulate some position updates
        viz.update(update)
        update.current_positions = [((x + 1) % 5, y) for x, y in update.current_positions]
        update.score += 10.0
        viz.keep_alive(pause_time=0.1)

    while viz.viz_active():
        viz.keep_alive(pause_time=0.1)

    viz.save_figs(grid_filename="grid_animation.gif", graph_filename="final_graph.png")
