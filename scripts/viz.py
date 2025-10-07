
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import matplotlib.animation as animation
import time
import networkx as nx


class AnimatedGrid:
    """
    Animated grid visualization for simulated annealing progress.
    """

    def __init__(self, NROWS, NCOLS, title="Simulated Annealing Progress", use_color_map=False, num_nodes=1):
        self.NROWS = NROWS
        self.NCOLS = NCOLS
        self.fig, self.ax = plt.subplots(figsize=(10, 8))
        self.title = title
        self.use_color_map = use_color_map
        self.num_nodes = num_nodes

        if use_color_map and num_nodes < 1:
            raise ValueError("num_nodes must be at least 1 when use_color_map is True")

        self.color_map = plt.cm.get_cmap('plasma', num_nodes) if use_color_map else None

        # Setup the plot
        self.setup_plot()
        
        # Storage for current state
        self.circles = []
        self.texts = []
        self.current_positions = []
        self.current_indices = []
        
        # Animation properties
        self.is_running = False
        self.animation_obj = None
        
    def setup_plot(self):
        """Initialize the plot with grid background."""
        # Remove axes, ticks, and labels
        self.ax.set_xlim(-0.5, self.NCOLS - 0.5)
        self.ax.set_ylim(-0.5, self.NROWS - 0.5)
        self.ax.set_xticks([])
        self.ax.set_yticks([])
        self.ax.set_aspect('equal')
        
        # Draw grid squares background
        for i in range(self.NROWS):
            for j in range(self.NCOLS):
                color = 'lightgray' if (i + j) % 2 == 0 else 'white'
                square = patches.Rectangle((j - 0.5, i - 0.5), 1, 1, 
                                         linewidth=0.5, edgecolor='gray', 
                                         facecolor=color, alpha=0.3)
                self.ax.add_patch(square)
        
        # Add border
        for spine in self.ax.spines.values():
            spine.set_visible(True)
            spine.set_linewidth(2)
        
        # Invert y-axis
        self.ax.invert_yaxis()
        
    def update_positions(self, new_positions, new_indices=None, iteration=None, score=None, draw=True):
        """Update node positions and redraw."""
        # Clear existing nodes
        for circle in self.circles:
            circle.remove()
        for text in self.texts:
            text.remove()
        
        self.circles.clear()
        self.texts.clear()
        
        # Store new state
        self.current_positions = new_positions.copy()
        self.current_indices = new_indices.copy() if new_indices else list(range(len(new_positions)))
        
        # Draw new nodes
        for idx, (row, col) in enumerate(new_positions):
            # Draw circle
            if self.use_color_map and self.color_map is not None:
                color = self.color_map(idx)
            else:
                color = 'steelblue'
            circle = plt.Circle((col, row), 0.3, color=color, 
                                edgecolor='black', linewidth=1.5, zorder=3)
            
            self.ax.add_patch(circle)
            self.circles.append(circle)
            
            # Add text
            label = str(self.current_indices[idx]) if idx < len(self.current_indices) else str(idx)
            text = self.ax.text(col, row, label, ha='center', va='center', 
                               color='white', fontweight='bold', fontsize=15, zorder=4)
            self.texts.append(text)
        
        # Update title with iteration info
        title_text = self.title
        if iteration is not None:
            title_text += f" - Iteration: {iteration}"
        if score is not None:
            title_text += f" - Score: {score}"
        
        self.ax.set_title(title_text, pad=20)

        if draw:
            # Force redraw
            self.fig.canvas.draw()
            self.fig.canvas.flush_events()
        
    def live_update_mode(self):
        """
        Set up for live updates during simulation.
        Call update_positions() whenever you want to update the display.
        """
        plt.ion()  # Turn on interactive mode
        plt.show()
        self.is_running = True
        
    def close(self):
        """Close the animation."""
        self.is_running = False
        plt.ioff()
        plt.close(self.fig)


def test_animated_grid():
    # Example usage
    NROWS, NCOLS = 5, 6  # 5 rows, 6 columns
    
    # Example node positions (row, col) for one node
    positions = [(0, 0)]


    print("Starting live animation...")
    
    # First, show the live animation
    sequence = []
    current_pos = positions.copy()
    grid = AnimatedGrid(NROWS, NCOLS, "Live Node Animation")
    grid.live_update_mode()
    
    for i in range(NROWS * NCOLS):
        # Store for GIF creation later
        sequence.append(current_pos.copy())
        
        # Show current position live
        grid.update_positions(current_pos, iteration=i, score=i)
        time.sleep(0.1)
        
        # Calculate next position, but don't wrap around at the end
        current_linear_pos = current_pos[0][0] * NCOLS + current_pos[0][1]
        next_linear_pos = current_linear_pos + 1
        
        # Stop if we've reached the end (don't wrap around)
        if next_linear_pos >= NROWS * NCOLS:
            break
            
        new_row = next_linear_pos // NCOLS
        new_col = next_linear_pos % NCOLS
        
        current_pos = [(new_row, new_col)]
    
    # Add the final position and show it
    sequence.append(current_pos.copy())
    grid.update_positions(current_pos, iteration="Final", score="Complete")
    
    print("Animation complete! Close the window when ready...")
    print("Waiting for window to close before saving GIF...")
    
    # Wait for user to close the window
    try:
        while plt.get_fignums():  # While there are still open figures
            plt.pause(0.1)
    except:
        pass
    
    print("Window closed. Now creating GIF...")
    
    # Add extra frames at the end for the GIF (hold final frame longer)
    final_frame_repeats = 10  # Hold final frame for 2 seconds at 200ms intervals
    sequence.extend([current_pos.copy()] * final_frame_repeats)
    
    # Create iteration and score sequences
    iteration_seq = list(range(len(sequence) - final_frame_repeats)) + ["Final"] * final_frame_repeats
    score_seq = list(range(len(sequence) - final_frame_repeats)) + ["Complete"] * final_frame_repeats
    
    print(f"Creating GIF with {len(sequence)} frames (final frame held for {final_frame_repeats} frames)...")
    
    # Create new grid instance for GIF creation (old one was closed)
    gif_grid = AnimatedGrid(NROWS, NCOLS, "Node Animation")
    
    # Create the animation object without displaying
    def animate_frame(frame):
        positions = sequence[frame]
        iteration = iteration_seq[frame] if frame < len(iteration_seq) else "Final"
        score = score_seq[frame] if frame < len(score_seq) else "Complete"
        gif_grid.update_positions(positions, None, iteration, score, draw=False)
        return gif_grid.circles + gif_grid.texts
    
    # Create animation object
    anim = animation.FuncAnimation(
        gif_grid.fig, animate_frame, frames=len(sequence),
        interval=200, blit=False, repeat=True
    )
    
    # Save GIF without showing
    print("Saving GIF...")
    anim.save("node_animation.gif", writer='pillow', fps=5)
    print("GIF saved as 'node_animation.gif'!")
    
    # Close the GIF creation window immediately
    plt.close('all')

def test_color_map():
    # Example usage
    NROWS, NCOLS = 5, 6  # 5 rows, 6 columns
    
    # Example node positions (row, col) for multiple nodes
    positions = [(0, 0), (0, 1), (0, 2), (0, 3), (0, 4), (0, 5)]
    
    print("Displaying static color map...")
    
    # Create the grid with color mapping
    grid = AnimatedGrid(NROWS, NCOLS, "Color Map Demo", use_color_map=True, num_nodes=len(positions))
    
    # Update with positions to show the color map
    grid.update_positions(positions, iteration="Static", score="Color Demo")
    
    # Show the plot and keep it open
    plt.show(block=True)  # block=True keeps the window open
    
    print("Window closed!")

def test_graph():
    """Display a basic directed graph: 0->2, 1->2, 2->3, 3->4, 3->5"""
    
    # Create a directed graph
    G = nx.DiGraph()
    
    # Add edges (nodes are created automatically)
    edges = [(0, 2), (1, 2), (2, 3), (3, 4), (3, 5)]
    G.add_edges_from(edges)
    
    # Create the plot
    plt.figure(figsize=(10, 8))
    
    # Position nodes using pygraphviz layouts (much better for automatic layout)
    # Try different layout algorithms:
    
    # Option 1: 'dot' - Hierarchical layout (best for directed graphs)
    pos = nx.nx_agraph.graphviz_layout(G, prog='dot')
    
    # Draw the graph
    nx.draw_networkx_nodes(G, pos, 
                          node_color='lightblue', 
                          node_size=1000, 
                          alpha=0.9)
    
    nx.draw_networkx_labels(G, pos, 
                           font_size=16, 
                           font_weight='bold')
    
    nx.draw_networkx_edges(G, pos, 
                          edge_color='gray', 
                          arrows=True, 
                          arrowsize=20, 
                          arrowstyle='->', 
                          width=2)
    
    # Add title and clean up the plot
    plt.title("Basic Directed Graph\n0→2, 1→2, 2→3, 3→4, 3→5", fontsize=14, pad=20)
    plt.axis('off')  # Remove axes
    
    # Show the graph and keep it open
    plt.tight_layout()
    plt.show(block=True)
    
    print("Graph window closed!")

if __name__ == "__main__":
    # test_animated_grid()
    # test_color_map()
    test_graph()