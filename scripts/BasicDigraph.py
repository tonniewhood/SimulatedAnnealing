# basic_digraph_pg.py
import numpy as np
import pyqtgraph as pg
import pyqtgraph.exporters
import networkx as nx

from PyQt5.QtCore import QPointF
from PyQt5.QtGui import QColor, QFont, QPen, QBrush, QPolygonF, QPainter
from pyqtgraph import GraphItem

class BasicDigraph:

    # class DirectedGraphItem(GraphItem):
    #     def __init__(self, **kw):
    #         super().__init__(**kw)
    #         self.edge_pen = QPen(QColor(0, 0, 0))
    #         self.arrow_brush = QBrush(QColor(0, 0, 0))
    #         self.line_width = 1
    #         self.arrow_width = 3
    #         self.arrow_height = 10

    #     def paint(self, painter, *args):
    #         super().paint(painter, *args)

    #         if self.pos is None or len(self.pos) == 0:
    #             return

    #         if not hasattr(self, 'adjacency') or self.adjacency is None or len(self.adjacency) == 0:
    #             return

    #         # Enable antialiasing for smooth lines
    #         painter.setRenderHint(painter.Antialiasing, True)
    #         painter.setPen(self.edge_pen)
    #         painter.setBrush(self.arrow_brush)

    #         view_box = self.getViewBox()
    #         view_x, view_y = (1.0, 1.0) if view_box is None else view_box.viewPixelSize()

    #         for edge in self.adjacency:
    #             i, j = edge
    #             # Make sure we have valid indices
    #             if i >= len(self.pos) or j >= len(self.pos):
    #                 print(f"Invalid edge indices: {i}, {j}")
    #                 continue
    #             # NetworkX positions are (x, y), pyqtgraph expects same
    #             p1 = QPointF(self.pos[i][0], self.pos[i][1])
    #             p2 = QPointF(self.pos[j][0], self.pos[j][1])

    #             # Get the line vector from p1 to p2
    #             vec = np.array([p2.x() - p1.x(), p2.y() - p1.y()])
    #             distance = np.linalg.norm(vec)
    #             if distance <= 1e-8:
    #                 continue

    #             unit_vec = vec / distance
    #             scale_factor = np.hypot(view_x * unit_vec[0], view_y * unit_vec[1])

    #             scaled_radius = (self.node_radius if hasattr(self, 'node_radius') else 20) * scale_factor
    #             scaled_line_w = self.line_width * scale_factor
    #             scaled_arrow_w = self.arrow_width * scale_factor
    #             scaled_arrow_h = self.arrow_height * scale_factor

    #             # Don't draw if nodes are too close
    #             min_distance = scaled_radius * 2.5  # Minimum distance based on actual node size
    #             if distance <= min_distance:
    #                 continue

    #             # Adjust distance to stop at node edge using actual node radius
    #             adjusted_distance = distance - scaled_radius + scaled_arrow_h # Small gap for clarity

    #             # Calculate angle (theta) from p1 to p2
    #             theta = np.arctan2(vec[1], vec[0])
                
    #             # Rotation matrix
    #             cos_theta = np.cos(theta)
    #             sin_theta = np.sin(theta)
                
    #             # Define arrow shape in local coordinates (pointing right)
    #             # The arrow points in the positive x direction initially
    #             arrow_points = np.array([
    #                 [0, -scaled_line_w/2],                           # Start of line, bottom
    #                 [adjusted_distance - scaled_arrow_h, -scaled_line_w/2],  # Near end, bottom
    #                 [adjusted_distance - scaled_arrow_h, -scaled_arrow_w],   # Arrow base, bottom
    #                 [adjusted_distance, 0],                            # Arrow tip
    #                 [adjusted_distance - scaled_arrow_h, scaled_arrow_w],    # Arrow base, top
    #                 [adjusted_distance - scaled_arrow_h, scaled_line_w/2],   # Near end, top
    #                 [0, scaled_line_w/2]                             # Start of line, top
    #             ])

    #             # Apply rotation and translation
    #             rotated_points = np.zeros_like(arrow_points)
    #             for i, point in enumerate(arrow_points):
    #                 x, y = point
    #                 rotated_points[i] = [
    #                     cos_theta * x - sin_theta * y + p1.x(),
    #                     sin_theta * x + cos_theta * y + p1.y()
    #                 ]
                
    #             # Create and draw the arrow polygon using the rotated points
    #             arrow = QPolygonF([QPointF(float(x), float(y)) for x, y in rotated_points])
    #             painter.drawPolygon(arrow)

    #     def setData(self, **kwds):
    #         # Capture the size parameter to calculate proper node radius
    #         if 'size' in kwds:
    #             self.node_size = kwds['size']
            
    #         # Call parent setData first
    #         super().setData(**kwds)

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
        self.graph = nx.DiGraph()
        self.graph.add_edges_from(edges)
        
        # Calculate adaptive sizes based on number of nodes
        self.num_nodes = len(self.graph.nodes())
        self.vertex_size = self._calculate_vertex_size()
        self.title_font_size = self._calculate_title_font_size()
        self.label_font_size = self._calculate_label_font_size()

        pos = nx.nx_agraph.graphviz_layout(self.graph, prog='dot')
        nodes = list(self.graph.nodes())

        # map node ids to 0..N-1 for GraphItem
        index = {n:i for i,n in enumerate(nodes)}
        edges_mapped = np.array([(index[u], index[v]) for (u,v) in self.graph.edges()], dtype=int)
        pts = np.array([pos[n] for n in nodes])

        label_font = QFont("Arial", self.label_font_size, QFont.Bold)
        labels = [str(n) for n in nodes]
        for (pos, label) in zip(pts, labels):
            text_item = pg.TextItem(text=label, color="w", anchor=(0.5, 0.5))
            text_item.setFont(label_font)
            text_item.setPos(pos[0], pos[1])
            text_item.setZValue(10)  # Ensure label is drawn in front
            self.widget.addItem(text_item)

        # Create the DirectedGraphItem and set its data
        self.item = pg.GraphItem()
        self.item.setData(
            pos=pts,
            adj=edges_mapped,
            size=self.vertex_size,
            symbolBrush=(100,150,255),
            pxMode=True
        )
        self.widget.plotItem.setTitle("Basic Directed Graph", color='k', size=f'{self.title_font_size}pt')
        self.widget.addItem(self.item)

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


import sys
from PyQt5 import QtWidgets, QtCore
from PyQt5.QtWidgets import QApplication

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
    graph.display()
    hbox.addWidget(graph.widget)

    win.show()

    while win.isVisible():
        app.processEvents()

    graph.save("basic_digraph.png")
