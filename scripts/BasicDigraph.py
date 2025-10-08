
import networkx as nx
import matplotlib.pyplot as plt

class BasicDigraph:
    def __init__(self, edges, figure=None, ax=None):
        """
        Initialize the graph with a list of edges.
        Each edge should be a tuple (node1, node2).
        """
        self.graph = nx.Graph()
        self.graph.add_edges_from(edges)
        self.fig, self.ax = figure, ax if figure and ax else plt.subplots(figsize=(10, 8))
        self.displayed = False

    def display(self):

        if self.displayed:
            return
        
        pos = nx.nx_agraph.graphviz_layout(self.graph, prog='dot')
        
        # Draw the graph
        nx.draw_networkx_nodes(self.graph, pos, 
                            node_color='lightblue', 
                            node_size=1000, 
                            alpha=0.9)

        nx.draw_networkx_labels(self.graph, pos, 
                            font_size=16, 
                            font_weight='bold')

        nx.draw_networkx_edges(self.graph, pos, 
                            edge_color='gray', 
                            arrows=True, 
                            arrowsize=20, 
                            arrowstyle='->', 
                            width=2)
        
        # Add title and clean up the plot
        self.ax.set_title("Basic Directed Graph\n0→2, 1→2, 2→3, 3→4, 3→5", fontsize=14, pad=20)
        self.ax.axis('off')  # Remove axes

        # Show the graph and keep it open
        self.fig.tight_layout()

        self.displayed = True

    def save(self, filename="basic_digraph.png"):
        """
        Save the graph to a file.
        """
        
        pos = nx.nx_agraph.graphviz_layout(self.graph, prog='dot')
        
        nx.draw_networkx_nodes(self.graph, pos, 
                            node_color='lightblue', 
                            node_size=1000, 
                            alpha=0.9)

        nx.draw_networkx_labels(self.graph, pos, 
                            font_size=16, 
                            font_weight='bold')

        nx.draw_networkx_edges(self.graph, pos, 
                            edge_color='gray', 
                            arrows=True, 
                            arrowsize=20, 
                            arrowstyle='->', 
                            width=2)

        self.ax.set_title("Basic Directed Graph\n0→2, 1→2, 2→3, 3→4, 3→5", fontsize=14, pad=20)
        self.ax.axis('off')

        self.fig.tight_layout()
        self.fig.savefig(filename, bbox_inches=self.ax.get_tightbbox(self.fig.canvas.get_renderer()).transformed(self.fig.dpi_scale_trans.inverted()))
