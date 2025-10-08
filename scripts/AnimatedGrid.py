
import matplotlib.animation as animation
import matplotlib.pyplot as plt
import matplotlib.patches as patches

class AnimatedGrid:
    """
    Animated grid visualization for simulated annealing progress.
    """

    def __init__(self, NROWS, NCOLS, title="Simulated Annealing Progress", use_color_map=False, num_nodes=1, figure=None, ax=None):
        
        self.NROWS = NROWS
        self.NCOLS = NCOLS
        self.fig, self.ax = (figure, ax) if (figure and ax) else plt.subplots(figsize=(10, 8))
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
        
    def update_positions(self, new_positions, new_indices=None, score=None, draw=True):
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
        self.sequence.append((score, new_positions.copy()))
        self.current_indices = new_indices.copy() if new_indices else list(range(len(new_positions)))
        
        # Draw new nodes
        for idx, (row, col) in enumerate(new_positions):
            # Draw circle
            if self.use_color_map and self.color_map is not None:
                color = self.color_map(idx)
            else:
                color = 'steelblue'
            circle = plt.Circle((col, row), 0.3, color=color, linewidth=1.5, zorder=3)
            
            self.ax.add_patch(circle)
            self.circles.append(circle)
            
            # Add text
            label = str(self.current_indices[idx]) if idx < len(self.current_indices) else str(idx)
            text = self.ax.text(col, row, label, ha='center', va='center', 
                               color='white', fontweight='bold', fontsize=15, zorder=4)
            self.texts.append(text)
        
        # Update title with iteration info
        title_text = self.title
        if score is not None:
            title_text += f" - Score: {score}"
        
        self.ax.set_title(title_text, pad=20)

        if draw:
            # Force redraw and process events
            self.fig.canvas.draw()
            self.fig.canvas.flush_events()
    
    def live_update_mode(self):
        """
        Set up for live updates during simulation.
        Call update_positions() whenever you want to update the display.
        """
        plt.ion()  # Turn on interactive mode
        self.is_running = True

    def close(self):
        """Close the animation."""
        self.is_running = False
        plt.ioff()
        plt.close(self.fig)

    def save_gif(self, filename="animation.gif", final_frame_hold_seconds=2., fps=10):

        num_frames = len(self.sequence) + int(final_frame_hold_seconds * fps)
        save_seq = self.sequence + [self.sequence[-1]] * int(final_frame_hold_seconds * fps)

        # Create the animation callback function
        def animate_frame(frame):
            positions = save_seq[frame][1]
            score = save_seq[frame][0]
            self.update_positions(positions, None, score, draw=False)
            return self.circles + self.texts

        # Make a temporary figure to save the GIF
        temp_fig, temp_ax = self.fig, self.ax
        self.fig, self.ax = plt.subplots(figsize=(10, 8))
        self.setup_plot()

        # Create animation object
        anim = animation.FuncAnimation(
            self.fig, animate_frame, frames=num_frames,
            interval=(num_frames*1000)/fps, blit=False, repeat=True
        )

        anim.save(filename=filename, writer='pillow', fps=fps)

        # Restore original figure and axis
        self.fig, self.ax = temp_fig, temp_ax
        self.setup_plot()

    def save_final_state(self, filename="final_state.png"):
        
        # Save the current figure and axis state so we can return to it
        temp_fig, temp_ax = self.fig, self.ax

        # Make a temporary figure to save the final state
        self.fig, self.ax = plt.subplots(figsize=(10, 8))
        self.setup_plot()

        # Draw the current positions on the temporary figure
        self.update_positions(self.current_positions, self.current_indices, score=self.sequence[-1][0], draw=False)
        self.fig.savefig(filename)

        # Restore original figure and axis
        self.fig, self.ax = temp_fig, temp_ax
        self.setup_plot()
