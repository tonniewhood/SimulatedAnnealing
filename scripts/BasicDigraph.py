
import sys

import numpy as np
import pyqtgraph as pg
import pyqtgraph.exporters
import networkx as nx

from PyQt5 import QtWidgets
from PyQt5.QtWidgets import QApplication
from PyQt5.QtGui import QFont, QPainter

class BasicDigraph:

    size: tuple[int, int] = (600, 600)
    widget: pg.PlotWidget
    nx_graph: nx.DiGraph
    pg_graph: pg.GraphItem
    num_nodes: int
    vertex_size: int
    title_font_size: int
    label_font_size: int

    def __init__(self, edges, size=(600, 600)):
        self.size = size
        self.widget = pg.PlotWidget()
        if size:
            self.widget.setFixedSize(size[0], size[1])
        self.widget.setBackground('w')
        self.widget.hideAxis('bottom')
        self.widget.hideAxis('left')
        
        # Improve rendering quality
        self.widget.setRenderHints(QPainter.Antialiasing | QPainter.TextAntialiasing | QPainter.SmoothPixmapTransform)

        # Setup the digraph and layout
        self.nx_graph = nx.DiGraph()
        self.nx_graph.add_edges_from(edges)
        
        # Calculate adaptive sizes based on number of nodes
        self.num_nodes = len(self.nx_graph.nodes())
        self.vertex_size = self._calculate_vertex_size()
        self.title_font_size = self._calculate_title_font_size()
        self.label_font_size = self._calculate_label_font_size()

        pos = nx.spring_layout(self.nx_graph, seed=42, iterations=2000)

        nodes = list(self.nx_graph.nodes())

        # map node ids to 0..N-1 for GraphItem
        index = {n:i for i,n in enumerate(nodes)}
        edges_mapped = np.array([(index[u], index[v]) for (u,v) in self.nx_graph.edges()], dtype=int)
        pts = np.array([pos[n] for n in nodes])

        label_font = QFont("Arial", self.label_font_size, QFont.Bold)
        labels = [str(n) for n in nodes]
        for (pos, label) in zip(pts, labels):
            text_item = pg.TextItem(text=label, color="w", anchor=(0.5, 0.5))
            text_item.setFont(label_font)
            text_item.setPos(pos[0], pos[1])
            text_item.setZValue(10)  # Ensure label is drawn in front
            self.widget.addItem(text_item)

        # Create the GraphItem and set its data
        self.pg_graph = pg.GraphItem()
        self.pg_graph.setData(
            pos=pts,
            adj=edges_mapped,
            size=self.vertex_size,
            symbolBrush=(100,150,255),
            pxMode=True
        )
        self.widget.plotItem.setTitle("Basic Directed Graph", color='k', size=f'{self.title_font_size}pt')
        self.widget.addItem(self.pg_graph)

    def _calculate_vertex_size(self):
        """Calculate vertex size based on number of nodes and widget size"""
        min_widget_dim = min(self.size[0], self.size[1])
        
        # More nodes = smaller vertices, with reasonable bounds
        if self.num_nodes <= 3:
            base_size = min_widget_dim / 12
        elif self.num_nodes <= 8:
            base_size = min_widget_dim / 15
        elif self.num_nodes <= 15:
            base_size = min_widget_dim / 20
        else:
            base_size = min_widget_dim / 25
            
        return max(15, min(60, int(base_size)))
    
    def _calculate_title_font_size(self):
        """Calculate title font size based on widget size"""
        # Scale title with widget width
        return max(14, min(22, self.size[0] // 25))
    
    def _calculate_label_font_size(self):
        """Calculate label font size based on vertex size"""
        return max(8, min(14, self.vertex_size // 4))

    def save(self, filename="basic_digraph.png"):
        exporter = pyqtgraph.exporters.ImageExporter(self.widget.plotItem)
        # Higher resolution export
        exporter.parameters()['width'] = 2000
        exporter.parameters()['height'] = 1500
        # Note: antialias parameter might not be available in all versions
        try:
            exporter.parameters()['antialias'] = True
        except KeyError:
            pass  # Parameter not available in this version
        exporter.export(filename)
        print(f"Graph saved as '{filename}'")


if __name__ == "__main__":

    app = QApplication(sys.argv)
    win = QtWidgets.QMainWindow()
    win.setWindowTitle("Simulated Annealing")
    win.setGeometry(100, 100, 800, 600)
    central = QtWidgets.QWidget()
    hbox = QtWidgets.QHBoxLayout(central)
    hbox.setContentsMargins(6, 6, 6, 6)
    hbox.setSpacing(8)
    win.setCentralWidget(central)

    edges = [(0, 2), (1, 2), (2, 3), (3, 4), (3, 5)]
    graph = BasicDigraph(edges)
    hbox.addWidget(graph.widget)

    win.show()

    while win.isVisible():
        app.processEvents()

    graph.save("basic_digraph.png")
