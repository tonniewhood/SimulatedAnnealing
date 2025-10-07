
import networkx as nx
import matplotlib.pyplot as plt

class BasicDigraph:
    def __init__(self, edges):
        """
        Initialize the graph with a list of edges.
        Each edge should be a tuple (node1, node2).
        """
        self.graph = nx.Graph()
        self.graph.add_edges_from(edges)

    def display(self):
        """
        Display the graph using matplotlib.
        """
        # Create the plot
        plt.figure(figsize=(10, 8))
        
        # Position nodes using pygraphviz layouts (much better for automatic layout)
        # Try different layout algorithms:
        
        # Option 1: 'dot' - Hierarchical layout (best for directed graphs)
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
        plt.title("Basic Directed Graph\n0→2, 1→2, 2→3, 3→4, 3→5", fontsize=14, pad=20)
        plt.axis('off')  # Remove axes
        
        # Show the graph and keep it open
        plt.tight_layout()
        plt.show(block=True)

    def save(self, filename="basic_digraph.png"):
        """
        Save the graph to a file.
        """
        plt.figure(figsize=(10, 8))
        
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
        
        plt.title("Basic Directed Graph\n0→2, 1→2, 2→3, 3→4, 3→5", fontsize=14, pad=20)
        plt.axis('off')
        
        plt.tight_layout()
        plt.savefig(filename)
        print(f"Graph saved as '{filename}'")
        plt.close()
