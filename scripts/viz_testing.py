
import time
import matplotlib.pyplot as plt
import matplotlib.animation as animation

from AnimatedGrid import AnimatedGrid
from BasicDigraph import BasicDigraph


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
    
    # Add edges (nodes are created automatically)
    edges = [(0, 2), (1, 2), (2, 3), (3, 4), (3, 5)]
    graph = BasicDigraph(edges)
    graph.display()
    graph.save()
    print("Graph window closed!")

if __name__ == "__main__":
    # test_animated_grid()
    # test_color_map()
    test_graph()
    # print("Working in it!")
