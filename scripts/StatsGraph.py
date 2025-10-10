# stats_graph_pg.py
import numpy as np
from PyQt5 import QtWidgets
import pyqtgraph as pg
import pyqtgraph.exporters as pg_exp
import sys
import time
from PyQt5 import QtWidgets, QtCore

WINDOW_WIDTH = 100000

class StatsGraph:
    plot_types = ["Temperature", "Score", "Best Score", "Delta Score", "Acceptance (%)"]
    
    # Define colors for each plot type
    plot_colors = {
        "Temperature": '#FF6B6B',      # Red
        "Score": '#4ECDC4',            # Teal  
        "Best Score": '#45B7D1',       # Blue
        "Delta Score": '#96CEB4',      # Green
        "Acceptance (%)": '#FECA57'    # Yellow/Orange
    }

    def __init__(self, title="Simulated Annealing Stats", size=(600, 600)):
        self.title = title
        self.size = size
        
        # Calculate adaptive font sizes
        self.title_font_size = self._calculate_title_font_size()
        
        self.widget = QtWidgets.QTabWidget()
        if size:
            self.widget.setFixedSize(size[0], size[1])
        self.curves = {}
        self.x = []  # timestamps
        self.series = {k: [] for k in self.plot_types}

        # Style the tab widget
        self.widget.setStyleSheet("""
            QTabWidget::pane {
                border: 2px solid #C0C0C0;
                background-color: white;
            }
            QTabBar::tab {
                background-color: #E0E0E0;
                padding: 8px 16px;
                margin: 2px;
            }
            QTabBar::tab:selected {
                background-color: #4ECDC4;
                color: white;
                font-weight: bold;
            }
        """)

        for lab in self.plot_types:
            w = pg.PlotWidget()
            
            # Set white background with better styling
            w.setBackground('white')
            
            # Enhanced grid styling with subtle colors
            w.showGrid(x=True, y=True, alpha=0.3)
            w.getPlotItem().getAxis('left').setPen(pg.mkPen(color='#404040', width=1))
            w.getPlotItem().getAxis('bottom').setPen(pg.mkPen(color='#404040', width=1))
            
            # Set grid colors to be more subtle
            w.getPlotItem().getAxis('left').setGrid(170)  # Light gray grid
            w.getPlotItem().getAxis('bottom').setGrid(170)
            
            # Set solid black title with adaptive formatting
            w.setTitle(f"{lab}", color='black', size=f'{self.title_font_size}pt', bold=True)
            
            # Add margins around the plot
            w.getPlotItem().setContentsMargins(15, 15, 15, 15)
            
            # Style the axes labels with bigger, bold font
            w.setLabel('left', 'Value', color='black', size='13pt', bold=True)
            w.setLabel('bottom', 'Time (s)', color='black', size='13pt', bold=True)
            
            # Create curve with color and better pen
            color = self.plot_colors[lab]
            pen = pg.mkPen(color=color, width=3, style=pg.QtCore.Qt.SolidLine) # Semi-transparent black
            
            curve = w.plot([], [], pen=pen)
            
            # Add some visual improvements
            w.enableAutoRange('xy', True)
            w.setMouseEnabled(x=True, y=True)  # Allow zooming/panning
            w.showButtons()  # Show auto-scale buttons
            
            # Set axis tick formatting for better readability
            w.getPlotItem().getAxis('left').setStyle(tickTextOffset=8)
            w.getPlotItem().getAxis('bottom').setStyle(tickTextOffset=8)
            
            self.widget.addTab(w, lab)
            self.curves[lab] = curve

    def _calculate_title_font_size(self):
        """Calculate title font size based on title length and widget width"""
        title_length = len(self.title)
        widget_width = self.size[0]
        
        # Base size scales with widget width, adjust for title length
        base_size = widget_width / 35
        length_factor = max(0.7, 1.0 - (title_length - 15) * 0.015)
        
        font_size = max(10, min(18, base_size * length_factor))
        return int(font_size)

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
            
            windowed_x = self.x[-WINDOW_WIDTH:]
            windowed_y = self.series[lab][-WINDOW_WIDTH:]
            self.curves[lab].setData(windowed_x, windowed_y)

        # Force widget repaint to ensure updates are visible
        self.widget.repaint()
    
    def save_figs(self, filenames):
        """Save each plot tab as a separate image file"""
        if len(filenames) != len(self.plot_types):
            print(f"Warning: Expected {len(self.plot_types)} filenames, got {len(filenames)}")
            return

        # Update each plot to ensure latest data is shown
        for lab, vals in self.series.items():
            self.curves[lab].setData(self.x, vals)

        # Force widget repaint to ensure updates are visible
        self.widget.repaint()

        for i, (plot_type, filename) in enumerate(zip(self.plot_types, filenames)):
            # Get the plot widget for this tab
            plot_widget = self.widget.widget(i)
            if plot_widget:
                exporter = pg_exp.ImageExporter(plot_widget.plotItem)
                # Higher resolution export with better quality
                exporter.parameters()['width'] = 1200
                exporter.parameters()['height'] = 900
                try:
                    exporter.parameters()['antialias'] = True
                except KeyError:
                    pass  # Parameter not available in this version
                exporter.export(filename)
                print(f"Saved {plot_type} plot as '{filename}'")


if __name__ == "__main__":
    

    app = QtWidgets.QApplication(sys.argv)
    stats_graph = StatsGraph()

    main_window = QtWidgets.QMainWindow()
    main_window.setCentralWidget(stats_graph.widget)
    main_window.resize(900, 700)
    main_window.show()

    start_time = time.time()
    duration = 10  # seconds
    interval = 50  # ms

    def animate():
        t = time.time() - start_time
        if t > duration:
            timer.stop()
            return
        # Generate new data point
        x = t
        stats_graph.update_stats(
            [x],
            [np.sqrt(x)],
            [x**2],
            [1/x if x != 0 else 0],
            [np.sin(x)],
            [np.tanh(x - 5)]
        )

    timer = QtCore.QTimer()
    timer.timeout.connect(animate)
    timer.start(interval)

    exit_code = app.exec_()
    stats_graph.save_figs([
        "a.png",
        "b.png",
        "c.png",
        "d.png",
        "e.png",
    ])

    sys.exit(exit_code)