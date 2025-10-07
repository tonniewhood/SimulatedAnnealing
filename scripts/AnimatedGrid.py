
import matplotlib.pyplot as plt
import matplotlib.patches as patches

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
        self.sequence = []
        
        # Animation properties
        self.is_running = False
        self.animation_obj = None
        
        # Callback for when window is closed
        self.close_callback = None
        
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
        self.sequence.append(new_positions.copy())
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
        
    def set_close_callback(self, callback):
        """
        Set a callback function that will be called when the window is closed.
        The callback should take no parameters and return nothing.
        """
        self.close_callback = callback
        self.fig.canvas.mpl_connect('close_event', self._on_close)
    
    def _on_close(self, event):
        """Internal method called when window is closed."""
        self.is_running = False
        if self.close_callback:
            self.close_callback()
    
    def live_update_mode(self):
        """
        Set up for live updates during simulation.
        Call update_positions() whenever you want to update the display.
        """
        plt.ion()  # Turn on interactive mode
        plt.show()
        self.is_running = True

    def hold(self):
        """Hold the animation open."""
        plt.ioff()
        plt.show(block=True)

    def continue_anim(self):
        """Continue the animation."""
        plt.ion()
        self.is_running = True

    def close(self):
        """Close the animation."""
        self.is_running = False
        plt.ioff()
        plt.close(self.fig)