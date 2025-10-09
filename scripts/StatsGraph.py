
"""
Ideas for graphs:
* Temperature versus time
* Score versus time
* Best score versus time
* Delta_score versus time
* Acceptance rate versus time
* Acceptance rate versus temperature
* Histogram of scores visited so far

Items I'll need
* Temperature - buffered
* Timestep - buffered
* Score - buffered
* Best score - buffered
* Delta score - buffered

* Number of accepted moves - per_update
* Grid state - per_update
"""

import matplotlib.pyplot as plt
from matplotlib.widgets import RadioButtons

class StatsGraph:
    """
    Animated grid visualization for simulated annealing progress.
    """

    plot_types = ["Temperature", "Score", "Best Score", "Delta Score", "Acceptance (%)"]
    timestamps = []
    temperatures = []
    scores = []
    best_scores = []
    delta_scores = []
    acceptance_rates = []

    plot_bboxes = {
        1: (0.275, 0., 0.825, 1.0),
        2: (0.375, 0., 0.855, 1.0),
        3: (0.555, 0., 0.830, 1.0),
    }

    tab_bboxes = {
        1: (-0.135, 0.2, 0.325, 0.5),
        2: (-0.1, 0.25, 0.35, 0.5),
        3: (-0.015, 0.25, 0.445, 0.5),
    }

    def __init__(self, title="Simulated Annealing Stats", figure=None, ax=None, figsize=(10, 8)):
        self.fig, self.ax = (figure, ax) if (figure and ax) else plt.subplots(figsize=figsize)
        self.title = title
        self.num_plots = round(figure.get_figwidth() / figsize[0])

        self.setup_axes()
        self.setup_plots()
        
    def setup_axes(self):

        self.plot_bbox, self.tab_bbox = self.get_bboxes(self.ax, self.plot_bboxes[self.num_plots], self.tab_bboxes[self.num_plots])

        self.pages = {}
        for k, lab in enumerate(self.plot_types):
            ax = self.fig.add_axes(self.plot_bbox, label=lab)
            ax.set_visible(k == 0)
            ax.set_zorder(self.ax.get_zorder() + 1)  # put the new axes on top of the old
            self.pages[lab] = ax

        self.ax.set_visible(False)  # hide the original axes

        tab_ax = self.fig.add_axes(self.tab_bbox)  # top-left strip
        tab_ax.set_zorder(1000)
        self.radio = RadioButtons(tab_ax, list(self.pages.keys()), active=0, label_props={'fontsize': [12, 12, 12]}, radio_props={'s': 50})

        # Callback: show only selected axes
        def on_tab(label):
            for name, ax in self.pages.items():
                ax.set_visible(name == label)
            self.fig.canvas.draw_idle()

        self.radio.on_clicked(on_tab)

    def setup_plots(self):
        
        self.lines = {}
        colors = {
            "Temperature": "#FF5733",
            "Score": "#2980B9",
            "Best Score": "#27AE60",
            "Delta Score": "#8E44AD",
            "Acceptance (%)": "#F1C40F"
        }
        
        for plot_type, ax in self.pages.items():
            line = ax.plot([], [], label=plot_type, color=colors.get(plot_type, 'blue'), linewidth=2, marker='o', markersize=4, alpha=0.85)
            ax.set_title(self.title, fontsize=14, fontweight='bold', color='#333333')
            ax.set_xlabel("Time", fontsize=12)
            ax.set_facecolor("#f7f7fa")
            ax.grid(True, which='both', linestyle='--', linewidth=0.7, alpha=0.6, color='#bbbbbb')
            ax.legend(frameon=True, loc='upper left', fontsize=10, fancybox=True, framealpha=0.8)
            self.lines[plot_type] = line[-1] # Store the Line2D object

    def get_bboxes(self, ax, plot_diff=(0.375, 0., 0.855, 1.0), tab_diff=(-0.1, 0.25, 0.35, 0.5)):
        """Get the bounding boxes for the plot and tab axes."""
        host = ax.get_position()
        dx, dy, plot_width, plot_height = plot_diff
        plot_bbox = [
            host.x0 + dx * host.width,
            host.y0 + dy * host.height,
            plot_width * host.width,
            plot_height * host.height
        ]
        tab_bbox = [
            host.x0 + tab_diff[0] * host.width,
            host.y0 + tab_diff[1] * host.height,
            tab_diff[2] * host.width,
            host.y0 + tab_diff[3] * host.height
        ]
        return plot_bbox, tab_bbox


    def update_stats(self, time_stamps, temperatures, scores, best_scores, delta_scores, acceptance_rates):
        self.timestamps.extend(time_stamps)
        self.temperatures.extend(temperatures)
        self.scores.extend(scores)
        self.best_scores.extend(best_scores)
        self.delta_scores.extend(delta_scores)
        self.acceptance_rates.extend(acceptance_rates)
        data = [self.temperatures, self.scores, self.best_scores, self.delta_scores, self.acceptance_rates]

        for line, new_data, ax in zip(self.lines.values(), data, self.pages.values()):
            line.set_xdata(self.timestamps)
            line.set_ydata(new_data)
            ax.relim()
            ax.autoscale_view()

    def live_update_mode(self):
        plt.ion()  # Turn on interactive mode

    def close(self):
        plt.ioff()
        plt.close(self.fig)

    def save_figs(self, filenames=[]):
        
        if len(filenames) != len(self.pages.keys()):
            raise ValueError("Number of filenames must match number of plots.")

        for (lab, filename) in zip(self.pages, filenames):
            if lab in self.lines:
                temp_fig, temp_ax = plt.figure(figsize=(8, 6)), plt.subplot(111)
                line = self.lines[lab]
                temp_ax.plot(line.get_xdata(), line.get_ydata(), label=lab, color='blue')
                temp_ax.set_title(self.title)
                temp_ax.set_xlabel("Time")
                temp_ax.set_ylabel(lab)
                temp_ax.legend()
                temp_fig.savefig(filename)
                plt.close(temp_fig)
