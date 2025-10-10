# animated_grid_pg.py
import numpy as np
import tempfile
import os
from PyQt5 import QtWidgets
import pyqtgraph as pg
import imageio
from pyqtgraph.exporters import ImageExporter
import sys
import time

class AnimatedGrid:
    def __init__(self, NROWS, NCOLS, title="Simulated Annealing Progress", use_color_map=False, num_nodes=1, size=(650, 600)):
        self.NROWS, self.NCOLS = NROWS, NCOLS
        self.title = title
        self.use_color_map = use_color_map
        self.num_nodes = num_nodes
        self.size = size
        
        # Calculate adaptive sizes
        self.vertex_size = self._calculate_vertex_size()
        self.title_font_size = self._calculate_title_font_size()
        self.label_font_size = self._calculate_label_font_size()

        self.widget = pg.GraphicsLayoutWidget()
        if size:
            self.widget.setFixedSize(size[0], size[1])
        self.widget.setBackground('white')
        self.plot = self.widget.addPlot()
        self.plot.setTitle(self.title, color='k', size=f'{self.title_font_size}pt')
        self.plot.setAspectLocked(True)
        
        # Set range to show grid with proper centering and border
        self.plot.setXRange(0.0, NCOLS, padding=0.1)
        self.plot.setYRange(0.0, NROWS, padding=0.1)
        self.plot.invertY(True)
        self.plot.hideAxis('bottom')
        self.plot.hideAxis('left')

        # Add grid border
        border = QtWidgets.QGraphicsRectItem(0.0, 0.0, NCOLS, NROWS)
        border.setPen(pg.mkPen(color='black', width=3))
        border.setBrush(pg.mkBrush(None))
        self.plot.addItem(border)

        # Light gray checkerboard background
        bg = pg.ImageItem(self._checkerboard(NROWS, NCOLS), levels=(0,255))
        bg.setZValue(-10)
        self.plot.addItem(bg)

        # single scatter for all nodes with black borders (adaptive size)
        self.scatter = pg.ScatterPlotItem(size=self.vertex_size, pen=pg.mkPen('black', width=2),
                                          brush=None if use_color_map else pg.mkBrush(70,130,180))
        self.plot.addItem(self.scatter)

        # pre-allocate label TextItems if you still want indices
        self.labels = []
        self.sequence = []
        self.label_positions = []
        self.scores = []

    def _calculate_vertex_size(self):
        """Calculate vertex size based on grid dimensions and widget size"""
        # Base size on the smaller of widget dimensions and grid cell size
        min_widget_dim = min(self.size[0], self.size[1]) - 100  # Account for margins/title
        grid_cell_size = min_widget_dim / max(self.NROWS, self.NCOLS)
        
        # Vertex should be 60-80% of cell size, with reasonable bounds
        vertex_size = max(8, min(50, grid_cell_size * 0.7))
        return int(vertex_size)
    
    def _calculate_title_font_size(self):
        """Calculate title font size based on title length and widget width"""
        title_length = len(self.title)
        widget_width = self.size[0]
        
        # Longer titles get smaller fonts, scale with widget width
        base_size = widget_width / 30  # Base scaling factor
        length_factor = max(0.6, 1.0 - (title_length - 20) * 0.02)  # Reduce for long titles
        
        font_size = max(12, min(24, base_size * length_factor))
        return int(font_size)
    
    def _calculate_label_font_size(self):
        """Calculate label font size based on vertex size"""
        # Label font should be proportional to vertex size
        return max(8, min(16, self.vertex_size // 3))

    def _checkerboard(self, rows, cols):
        # Create checkerboard pattern with light gray (RGB) instead of black
        a = np.indices((rows, cols)).sum(axis=0) % 2
        img = np.zeros((rows, cols), dtype=np.uint8)
        white = 255
        gray = 220  # light gray
        img[a == 0] = white
        img[a == 1] = gray
        return img

    def _plasma_color(self, index, total_nodes):
        """Generate a color from matplotlib's plasma colormap"""
        if total_nodes <= 1:
            return pg.mkBrush(240, 249, 33)  # Plasma yellow for single node
        
        # Normalize index to 0-1 range
        t = index / (total_nodes - 1)
        
        # Plasma colormap approximation: dark purple -> magenta -> orange -> yellow
        if t < 0.25:
            # Dark purple to magenta
            s = t * 4
            r = int(13 + (179 - 13) * s)
            g = int(8 + (10 - 8) * s)  
            b = int(135 + (140 - 135) * s)
        elif t < 0.5:
            # Magenta to red/orange
            s = (t - 0.25) * 4
            r = int(179 + (234 - 179) * s)
            g = int(10 + (85 - 10) * s)
            b = int(140 + (39 - 140) * s)
        elif t < 0.75:
            # Red/orange to orange/yellow
            s = (t - 0.5) * 4
            r = int(234 + (253 - 234) * s)
            g = int(85 + (190 - 85) * s)
            b = int(39 + (31 - 39) * s)
        else:
            # Orange/yellow to softer yellow
            s = (t - 0.75) * 4
            r = int(253 + (235 - 253) * s)   
            g = int(190 + (235 - 190) * s)   
            b = int(31 + (60 - 31) * s)      
        
        return pg.mkBrush(r, g, b)

    def update_positions(self, new_positions, score=None):
        if not new_positions:
            return
        pts = np.asarray(new_positions, dtype=float)  # [(r,c),...]
        self.sequence.append(pts)
        # Center the vertices in cells by adding 0.5 offset to both x and y
        xy = np.c_[pts[:,1] + 0.5, pts[:,0] + 0.5]

        brushes = None
        if self.use_color_map:
            # Plasma colormap gradient colors
            brushes = [self._plasma_color(i, self.num_nodes) for i in range(len(xy))]

        self.scatter.setData(x=xy[:,0], y=xy[:,1], brush=brushes)

        if score is not None:
            self.plot.setTitle(f"{self.title} - Score: {score:.2f}")
        
        self.scores.append(score if score is not None else float('nan'))

        # Force a repaint to ensure updates are visible
        self.widget.repaint()

        # simple label pool with larger font and black outline
        while len(self.labels) < len(xy):
            t_outline = pg.TextItem("", anchor=(0.5,0.5), color='w')
            t_outline.setFont(pg.QtGui.QFont("Arial", self.label_font_size, pg.QtGui.QFont.Bold))
            t_outline.setZValue(20)
            self.plot.addItem(t_outline)
            self.labels.append(t_outline)
        for i,(x,y) in enumerate(xy):
            self.labels[i].setText(str(i))
            self.labels[i].setPos(x,y)

        self.label_positions = xy

    def live_update_mode(self):
        pass  # not needed with Qt; Viz.show/keep_alive drives the loop

    def close(self):
        self.widget.close()

    def save_gif(self, filename="animation.gif", final_frame_hold_seconds=2., fps=10):
        if not self.sequence:
            print("No frames to save.")
            return

        images = []
        exporter = ImageExporter(self.plot)
        exporter.parameters()['width'] = 400  # adjust as needed

        # Save each frame in the sequence
        for frame in self.sequence:
            # Update scatter to this frame - center vertices in cells
            xy = np.c_[frame[:,1] + 0.5, frame[:,0] + 0.5]
            brushes = None

            if self.use_color_map:
                brushes = [self._plasma_color(i, self.num_nodes) for i in range(len(xy))]
                self.scatter.setData(x=xy[:,0], y=xy[:,1], brush=brushes)
                for i,(x,y) in enumerate(xy):
                    self.labels[i].setText(str(i))
                    self.labels[i].setPos(x,y)
                self.plot.setTitle(f"{self.title} - Score: {self.scores[len(images)]:.2f}" if len(self.scores) > len(images) else self.title)

            QtWidgets.QApplication.processEvents()
            # Export to temporary file and read back as array
            with tempfile.NamedTemporaryFile(suffix='.png', delete=False) as tmp:
                temp_path = tmp.name
            exporter.export(temp_path)
            images.append(imageio.imread(temp_path))
            os.unlink(temp_path)  # Clean up temp file

        # Hold the final frame
        for _ in range(int(final_frame_hold_seconds * fps)):
            images.append(images[-1])

        imageio.mimsave(filename, images, fps=fps)


    def save_final_state(self, filename="final_state.png"):
        exporter = pg.exporters.ImageExporter(self.plot)
        exporter.parameters()['width'] = 400  # or whatever size you want
        exporter.export(filename)


if __name__ == "__main__":

    app = QtWidgets.QApplication(sys.argv)
    grid = AnimatedGrid(5, 5, use_color_map=True, num_nodes=5)
    grid.widget.show()

    positions = []
    # Fill diagonal
    for i in range(5):
        pos = [(j, j) for j in range(i + 1)]
        grid.update_positions(pos)
        QtWidgets.QApplication.processEvents()
        time.sleep(0.3)
        positions.append(pos)

    # Move right and wrap around
    for step in range(1, 6):
        pos = [((j), (j + step) % 5) for j in range(5)]
        grid.update_positions(pos)
        QtWidgets.QApplication.processEvents()
        time.sleep(0.3)
        positions.append(pos)

    sys.exit(app.exec_())
