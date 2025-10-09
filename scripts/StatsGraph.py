# stats_graph_pg.py
import numpy as np
from PyQt5 import QtWidgets
import pyqtgraph as pg

class StatsGraph:
    plot_types = ["Temperature", "Score", "Best Score", "Delta Score", "Acceptance (%)"]

    def __init__(self, title="Simulated Annealing Stats"):
        self.title = title
        self.widget = QtWidgets.QTabWidget()
        self.curves = {}
        self.x = []  # timestamps
        self.series = {k: [] for k in self.plot_types}

        for lab in self.plot_types:
            w = pg.PlotWidget()
            w.setBackground('w')
            w.showGrid(x=True, y=True, alpha=0.3)
            w.setTitle(self.title)
            curve = w.plot([], [], pen=pg.mkPen(width=2))
            self.widget.addTab(w, lab)
            self.curves[lab] = curve
            # nice auto-range behavior
            w.enableAutoRange('xy', True)

    def update_stats(self, time_stamps, temperatures, scores, best_scores, delta_scores, acceptance_rates):
        # append
        self.x.extend(time_stamps)
        data = {
            "Temperature": temperatures,
            "Score": scores,
            "Best Score": best_scores,
            "Delta Score": delta_scores,
            "Acceptance (%)": acceptance_rates,
        }
        for lab, vals in data.items():
            self.series[lab].extend(vals)
            self.curves[lab].setData(self.x, self.series[lab])
        
        # Force widget repaint to ensure updates are visible
        self.widget.repaint()

    def live_update_mode(self): 
        pass

    def close(self): 
        self.widget.close()
    
    def save_figs(self, filenames):
        """Save each plot tab as a separate image file"""
        if len(filenames) != len(self.plot_types):
            print(f"Warning: Expected {len(self.plot_types)} filenames, got {len(filenames)}")
            return
        
        for i, (plot_type, filename) in enumerate(zip(self.plot_types, filenames)):
            # Get the plot widget for this tab
            plot_widget = self.widget.widget(i)
            if plot_widget:
                exporter = pg.exporters.ImageExporter(plot_widget.plotItem)
                exporter.parameters()['width'] = 800
                exporter.parameters()['height'] = 600
                exporter.export(filename)
                print(f"Saved {plot_type} plot as '{filename}'")
