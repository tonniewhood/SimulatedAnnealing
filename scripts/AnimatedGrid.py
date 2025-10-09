# animated_grid_pg.py
import numpy as np
import tempfile
import os
from PyQt5 import QtWidgets
import pyqtgraph as pg
import imageio
from pyqtgraph.exporters import ImageExporter

class AnimatedGrid:
    def __init__(self, NROWS, NCOLS, title="Simulated Annealing Progress", use_color_map=False, num_nodes=1):
        self.NROWS, self.NCOLS = NROWS, NCOLS
        self.title = title
        self.use_color_map = use_color_map
        self.num_nodes = num_nodes

        self.widget = pg.GraphicsLayoutWidget()
        self.plot = self.widget.addPlot()
        self.plot.setTitle(self.title)
        self.plot.setAspectLocked(True)
        self.plot.setXRange(-0.5, NCOLS - 0.5, padding=0)
        self.plot.setYRange(-0.5, NROWS - 0.5, padding=0)
        self.plot.invertY(True)
        self.plot.hideAxis('bottom')
        self.plot.hideAxis('left')

        # light checkerboard background (optional)
        bg = pg.ImageItem(self._checkerboard(NROWS, NCOLS))
        bg.setZValue(-10)
        self.plot.addItem(bg)

        # single scatter for all nodes
        self.scatter = pg.ScatterPlotItem(size=16, pen=pg.mkPen(None),
                                          brush=None if use_color_map else pg.mkBrush(70,130,180))
        self.plot.addItem(self.scatter)

        # pre-allocate label TextItems if you still want indices
        self.labels = []
        self.sequence = []

    def _checkerboard(self, rows, cols):
        a = np.indices((rows, cols)).sum(axis=0) % 2
        img = a.astype(np.float32)
        return img  # colormap handled by pyqtgraph’s LUT if you want

    def update_positions(self, new_positions, new_indices=None, score=None, draw=True):
        if not new_positions:
            return
        pts = np.asarray(new_positions, dtype=float)  # [(r,c),...]
        self.sequence.append(pts)
        # swap to (x=col, y=row)
        xy = np.c_[pts[:,1], pts[:,0]]

        brushes = None
        if self.use_color_map:
            # simple categorical colors
            brushes = [pg.intColor(i, hues=self.num_nodes) for i in range(len(xy))]

        self.scatter.setData(x=xy[:,0], y=xy[:,1], brush=brushes)

        if score is not None:
            self.plot.setTitle(f"{self.title} - Score: {score:.2f}")

        # Force a repaint to ensure updates are visible
        self.widget.repaint()

        # optional labels (avoid for maximum speed)
        if new_indices is not None:
            # simple label pool
            while len(self.labels) < len(xy):
                t = pg.TextItem("", anchor=(0.5,0.5), color='w')
                t.setZValue(20)
                self.plot.addItem(t)
                self.labels.append(t)
            for i,(x,y) in enumerate(xy):
                self.labels[i].setText(str(new_indices[i]) if i < len(new_indices) else str(i))
                self.labels[i].setPos(x,y)

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
            # Update scatter to this frame
            xy = np.c_[frame[:,1], frame[:,0]]
            brushes = None

            if self.use_color_map:
                brushes = [pg.intColor(i, hues=self.num_nodes) for i in range(len(xy))]
                self.scatter.setData(x=xy[:,0], y=xy[:,1], brush=brushes)

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
