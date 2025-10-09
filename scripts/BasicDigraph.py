# basic_digraph_pg.py
import numpy as np
import pyqtgraph as pg
import networkx as nx

class BasicDigraph:
    def __init__(self, edges):
        self.widget = pg.PlotWidget()
        self.widget.setBackground('w')
        self.widget.hideAxis('bottom')
        self.widget.hideAxis('left')
        self.graph = nx.Graph()
        self.graph.add_edges_from(edges)
        self.item = pg.GraphItem()
        self.widget.addItem(self.item)
        self.displayed = False
        # Auto-display when created
        self.display()

    def display(self):
        if self.displayed:
            return
        pos = nx.nx_agraph.graphviz_layout(self.graph, prog='dot')
        nodes = list(self.graph.nodes())

        # map node ids to 0..N-1 for GraphItem
        index = {n:i for i,n in enumerate(nodes)}
        edges_mapped = np.array([(index[u], index[v]) for (u,v) in self.graph.edges()], dtype=int)
        pts = np.array([pos[n] for n in nodes])

        self.item.setData(pos=pts, adj=edges_mapped,
                          size=10, symbolBrush=(100,150,255), pxMode=True)
        self.displayed = True

    def save(self, filename="basic_digraph.png"):
        exporter = pg.exporters.ImageExporter(self.widget.plotItem)
        exporter.parameters()['width'] = 1000
        exporter.export(filename)
