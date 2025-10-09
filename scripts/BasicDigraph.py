
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
        self.fig, self.ax = (figure, ax) if figure and ax else plt.subplots(figsize=(10, 8))
        self.displayed = False

    def display(self):

        if self.displayed:
            return
        
        pos = nx.nx_agraph.graphviz_layout(self.graph, prog='dot')
        
        # Draw the graph on the specified axis
        nx.draw_networkx_nodes(self.graph, pos, 
                            ax=self.ax,  # Specify the axis
                            node_color='lightblue', 
                            node_size=1000, 
                            alpha=0.9)

        nx.draw_networkx_labels(self.graph, pos, 
                            ax=self.ax,  # Specify the axis
                            font_size=16, 
                            font_weight='bold')

        nx.draw_networkx_edges(self.graph, pos, 
                            ax=self.ax,  # Specify the axis
                            edge_color='gray', 
                            arrows=True, 
                            arrowsize=20, 
                            arrowstyle='->', 
                            width=2)
        
        # Add title and clean up the plot
        self.ax.set_title("Basic Directed Graph\n0→2, 1→2, 2→3, 3→4, 3→5", fontsize=14, pad=20)
        self.ax.axis('off')  # Remove axes

        self.displayed = True

    def save(self, filename="basic_digraph.png"):
        """
        Save the graph to a file.
        """
        
        temp_fig = plt.figure(figsize=(10, 8))
        temp_ax = temp_fig.add_subplot(1, 1, 1)

        pos = nx.nx_agraph.graphviz_layout(self.graph, prog='dot')
        
        nx.draw_networkx_nodes(self.graph, pos, 
                            ax=temp_ax,  # Specify the axis
                            node_color='lightblue', 
                            node_size=1000, 
                            alpha=0.9)

        nx.draw_networkx_labels(self.graph, pos, 
                            ax=temp_ax,  # Specify the axis
                            font_size=16, 
                            font_weight='bold')

        nx.draw_networkx_edges(self.graph, pos, 
                            ax=temp_ax,  # Specify the axis
                            edge_color='gray', 
                            arrows=True, 
                            arrowsize=20, 
                            arrowstyle='->', 
                            width=2)

        temp_ax.set_title("Basic Directed Graph\n0→2, 1→2, 2→3, 3→4, 3→5", fontsize=14, pad=20)
        temp_ax.axis('off')

        temp_fig.savefig(filename, bbox_inches=temp_ax.get_tightbbox(temp_fig.canvas.get_renderer()).transformed(temp_fig.dpi_scale_trans.inverted()))
