#!/usr/bin/python
from inspect import stack
from turtle import width
import numpy as np
import matplotlib
from time import sleep
from cellworld import *
import sys
from PyQt6.QtWidgets import *
from PyQt6.QtCore import *
from PyQt6.QtGui import *
from src import *
from threading import Thread
matplotlib.use('Qt5Agg')
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg
from moviepy.editor import *
from matplotlib import cm
from matplotlib.backend_bases import MouseButton
from os.path import basename
import pyqtgraph as pg
import time

# TODO
# Kill play_pause thread after detail view close
# Add point on graph as steps advance
# Add step counter

# Check two files at upload (exp and stats)


class EpBrowser(QMainWindow):

    def __init__(self, parent=None):
        super(EpBrowser, self).__init__(parent)
        self.central_widget = QWidget()
        self.setGeometry(0, 0, 1000, 700)
        self.setCentralWidget(self.central_widget)
        self.central_lay = QGridLayout(self.central_widget)

        self.setWindowTitle("Episode browser")

    def initialize(self, sim_cont):
        '''
        Show heatmaps of parent's (sim container class) episode containers
        One horizontal scroll area
        '''
        self.scroll_area = QScrollArea(self.central_widget)             # Scroll Area which contains the widgets
        self.scroll_widget = QWidget()                                  # Widget that contains the collection of Vertical Box
        self.scroll_grid = QGridLayout(self.scroll_area)                # The Horizontal Box that contains the Horizontal Boxes of world figures
        self.scroll_widget.setLayout(self.scroll_grid)

        #Scroll Area Properties
        self.scroll_area.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOn)
        self.scroll_area.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOn)
        self.scroll_area.setWidgetResizable(True)
        self.scroll_area.setWidget(self.scroll_widget)


        # Add to layout here
        pos = [0, 0]
        for epc in sim_cont.loaded_eps:
            self.scroll_grid.addWidget(epc, pos[0], pos[1])
            pos[1] += 1

        self.central_lay.addWidget(self.scroll_area)
        self.show()

    def closeEvent(self, event):
        # Define close events
        print('calling close ep browser')
        # while self.central_lay.count():
        #     child = self.central_lay.takeAt(0)
        #     if child.widget():
        #         #child.widget().deleteLater()
        #         self.central_lay.removeWidget(child)
        for i in reversed(range(self.central_lay.count())):
            item = self.central_lay.itemAt(i)
            # remove the item from layout
            self.central_lay.removeItem(item)


class DetailView(QMainWindow):

    # Call show to reveal window
    # Should have value functions and step through of pred and prey
    # Needs play stop ff rw buttons

    def __init__(self, parent=None, stacked_disp=None, pred_vals=None, prey_vals=None):
        #super(DetailView, self).__init__(parent)
        super().__init__(parent)
        self.central_widget = QWidget()
        #self.setGeometry(0, 0, 500, 500)
        self.setCentralWidget(self.central_widget)
        self.central_lay = QGridLayout(self.central_widget)
        self.stacked_disp = stacked_disp
        
        # Make the step map see the detail view (self) so it can call the plot point update
        self.stacked_disp.map.detail_view = self

        self.central_lay.addWidget(self.stacked_disp, 0, 0)
        self.plot_widget = pg.PlotWidget()
        self.central_lay.addWidget(self.plot_widget, 0, 1)
        self.plot_widget.setBackground('w')
        
        pen1 = pg.mkPen(color=(255, 0, 0), width=3)
        pen2 = pg.mkPen(color=(0, 0, 255), width=3)

        # Use ScatterPlotItem to draw points
        self.scatterItem = pg.ScatterPlotItem(
            size=15, 
            pen=pg.mkPen(None), 
            brush=pg.mkBrush(255, 255, 0),
            hoverable=True,
            hoverBrush=pg.mkBrush(0, 255, 255)
        )
        self.plot_widget.addItem(self.scatterItem)
        
        if pred_vals:
            self.pred_x = [v[0] for v in pred_vals]
            self.pred_y = [v[1] for v in pred_vals]
            self.plot_widget.plot(self.pred_x, self.pred_y, pen=pen2)

        if prey_vals:
            self.prey_x = [v[0] for v in prey_vals]
            self.prey_y = [v[1] for v in prey_vals]
            self.plot_widget.plot(self.prey_x, self.prey_y, pen=pen1)

        self.plot_step()

        self.setWindowTitle("Episode Details")

    def plot_step(self, step_idx=0):
        '''
        Plot a marker point at a given x value
        '''
        
        self.scatterItem.clear()
        x = [step_idx]
        y = [self.prey_y[step_idx]]
        self.scatterItem.setPoints(list(x), list(y))
        



class TableView(QTableWidget):
    def __init__(self, data, parent=None):
        QTableWidget.__init__(self, parent)
        self.data = data
        self.parent = parent
        self.setData()
        self.resizeColumnsToContents()
        self.resizeRowsToContents()
 
    def setData(self): 
        #horHeaders = []
        self.setRowCount(len(self.data.keys()))
        self.setColumnCount(2)
        row_idx = 0
        for key, val in self.data.items():
            #print(f'Setting {key}: {val} in table...')
            self.setItem(row_idx, 0, QTableWidgetItem(str(key)))
            self.setItem(row_idx, 1, QTableWidgetItem(str(val)))
            row_idx += 1
        #self.setHorizontalHeaderLabels(horHeaders)

    def mousePressEvent(self, QMouseEvent):
        if QMouseEvent.button() == Qt.MouseButton.LeftButton:
            self.parent.mousePressEvent(QMouseEvent)
            print('Mouse press in table view')


class StackedDisplay(QWidget):
    '''
    Represents a stacked display of statistics and one (or more) world plots such as heatmaps.
    May contain parent for
    '''

    def __init__(self, parent=None, stats_dictionary=None, map=None):
        super().__init__(parent)
        self.layout = QGridLayout()
        self.stats_dictionary = stats_dictionary
        self.stats_table = TableView(stats_dictionary, self)
        self.map = map
        self.layout.addWidget(self.map, 0, 0, 1, 2)
        if isinstance(self.map, StepMap):
            self.add_step_map_buttons()
        self.layout.addWidget(self.stats_table, 2, 0, 1, 2)
        self.setLayout(self.layout)
        self.setGeometry(0, 0, 500, 500)
        self.ispaused = True
    
    def mousePressEvent(self, QMouseEvent):
        if QMouseEvent.button() == Qt.MouseButton.LeftButton:
            self.parent.mousePressEvent(QMouseEvent)
            print('Mouse pressed in stacked display!')

    def add_step_map_buttons(self):
        self.play_button=QPushButton()
        self.play_icon = self.style().standardIcon(getattr(QStyle.StandardPixmap, 'SP_MediaPlay'))
        self.pause_icon = self.style().standardIcon(getattr(QStyle.StandardPixmap, 'SP_MediaPause'))
        self.play_button.setIcon(self.play_icon)
        self.play_button.clicked.connect(self.on_play_pause)
        self.layout.addWidget(self.play_button, 1, 0, 1, 1)

        # Reset button
        self.stop_button=QPushButton()
        self.stop_icon = self.style().standardIcon(getattr(QStyle.StandardPixmap, 'SP_MediaStop'))
        self.stop_button.setIcon(self.stop_icon)
        self.stop_button.clicked.connect(self.on_stop)
        self.layout.addWidget(self.stop_button, 1, 1, 1, 1)

    def on_play_pause(self):
        if isinstance(self.map, StepMap):
            self.map.ispaused=not self.map.ispaused
            if self.map.ispaused:
                self.play_button.setIcon(self.play_icon)
            else:
                self.play_button.setIcon(self.pause_icon)

            # If the detail_view attribute is set update the scatter plot
            if hasattr(self, 'detail_view'):
                pass

    def on_stop(self):
        if isinstance(self.map, StepMap):
            self.map.current_frame=0 # Reset frame
            self.play_button.setIcon(self.play_icon) # Reset play/pause button
            self.map.show_step()
            self.map.ispaused=True # Stop stepping
            


class StepMap(FigureCanvasQTAgg):

    def __init__(self, world, episode, parent=None, title=""):

        self.current_frame=0
        self.ispaused=True
        self.pp_thread = Thread(target=self.playback)

        self.predator_destination_arrow=None
        self.world=world
        self.episode=episode

        self.display = Display(world)
        self.parent=parent
        super().__init__(self.display.fig)
        self.display.fig.suptitle(title, fontsize=15)
        self.setFixedSize(300, 300)

    def playback(self):
        #print('in play pause')
        self.show_step()
        while True:
            if not self.ispaused and (self.current_frame<len(self.episode)):
                #print('showing step')
                self.show_step()
                time.sleep(0.3)
            elif self.current_frame == len(self.episode):
                self.current_frame=0
                self.show_step()
                self.ispaused=True

    def show_step(self):

        if hasattr(self, 'detail_view'):
            self.detail_view.plot_step(self.current_frame)

        prey_state = self.episode[self.current_frame].prey_state
        prey_cell = self.world.cells[prey_state.cell_id]

        predator_state = self.episode[self.current_frame].predator_state
        predator_cell = self.world.cells[predator_state.cell_id]
        predator_destination_cell = self.world.cells[predator_state.destination_id]

        if prey_state.belief_state:
            upper_limit = max(prey_state.belief_state)

        if upper_limit > 0:
            cmap = plt.cm.Reds([x / upper_limit for x in prey_state.belief_state])

        for cell in self.world.cells:
            color = "white"
            if upper_limit > 0:
                color = cmap[cell.id]
            if prey_cell.id == cell.id:
                color = "green"
            if predator_cell.id == cell.id:
                color = "blue"
            if self.world.cells[cell.id].occluded:
                color = "black"
            self.display.cell(cell=cell, color=color)

        if True: #self.view_predator_destination.isChecked():
            self.predator_destination_arrow = self.display.arrow(beginning=predator_cell.location, ending=predator_destination_cell.location, color="blue", existing_arrow=self.predator_destination_arrow)
            self.predator_destination_arrow.set_visible(True)
        else:
            if self.predator_destination_arrow:
                self.predator_destination_arrow.set_visible(False)

        self.current_frame += 1
        self.display.update()
        self.draw()

        # if True: #self.view_prey_plan.isChecked():
        #     prev = prey_cell
        #     [arrow.set_visible(False) for arrow in self.plan_arrows if arrow is not None]
        #     for i, plan_step_cell_id in enumerate(prey_state.plan):
        #         next = self.world.cells[plan_step_cell_id]
        #         self.plan_arrows[i] = self.display.arrow(beginning=prev.location, ending=next.location, color="green", existing_arrow=self.plan_arrows[i])
        #         self.plan_arrows[i].set_visible(True)
        #         prev = next

        


class HeatMap(FigureCanvasQTAgg):

    def __init__(self, pred_counters, prey_counters, world, parent=None, zero_color="white", title=""):

        d = Display(world)
        self.parent = parent
        super().__init__(d.fig)
        d.fig.suptitle(title, fontsize=15)
        
        # get the max number of counts in a cell (pred and prey included)
        count_max = np.max(np.array(pred_counters) + np.array(prey_counters))
        assert(len(pred_counters) == len(prey_counters))
        cmap = lambda p1, p2 : ((1 - p1/count_max), 1, 1 - (p2/count_max))

        for i, _ in enumerate(pred_counters):
            if not world.cells[i].occluded:
                if pred_counters[i] + prey_counters[i] > 0:
                    d.cell(cell_id=i, color=cmap(pred_counters[i], prey_counters[i]))
                else:
                    d.cell(cell_id=i, color=zero_color)

        self.setFixedSize(300, 300)
    
    def mousePressEvent(self, QMouseEvent):
        if QMouseEvent.button() == Qt.MouseButton.LeftButton:
            self.parent.mousePressEvent(QMouseEvent)
            print('mouse press in hm')


class SimulationContainter(HeatMap):

    def __init__(self, filename, stats_filename, parent=None):
        self.parent = parent
        self.filename = filename
        self.stats_filename = stats_filename
        self.simulation = Simulation.load_from_file(self.filename)

        # CURRENT ISSUE, 
        if np.all([ep == [] for ep in self.simulation.episodes]):
            raise Exception(f'Simulation episodes appear to be empty.\nEpisodes={self.simulation.episodes}')

        print('Building world ...')
        self.sim_occlusions = Cell_group_builder.get_from_name(world_name="hexagonal." + self.simulation.world_info.occlusions, name="occlusions")
        print('Finished building world')
        self.world = World.get_from_parameters_names("hexagonal", "canonical")
        self.world.set_occlusions(self.sim_occlusions)
        
        #stats = self.simulation.get_stats(reward=Reward()) # Old way -> get stats from exp file
        stats = Simulation_statistics.load_from_sim_file_name(self.filename)

        self.loaded_eps = [EpisodeContainer(ep, self.sim_occlusions, stats=stats_, parent=self.parent) for ep, stats_ in zip(self.simulation.episodes, stats.episode_stats)]
        exclude_stats = [attr for attr in dir(stats) if '__' in attr or attr in ['episode_stats', 'save', 'parse', 'load_from_url', 'load_from_file', 'load', 'format', 'copy']]
        sim_stat_props = [attr for attr in dir(stats) if attr not in exclude_stats]
        self.simulation_stats = {prop: getattr(stats, prop) for prop in sim_stat_props}

        prey_counters = [0 for x in self.world.cells]
        pred_counters = [0 for x in self.world.cells]
        for _, ep_cont in enumerate(self.loaded_eps):
            pred_c, prey_c = ep_cont.pred_counters, ep_cont.prey_counters
            pred_counters = [sum(pair) for pair in zip(pred_c, pred_counters)]
            prey_counters = [sum(pair) for pair in zip(prey_c, prey_counters)]

        super().__init__(pred_counters, prey_counters, self.world, parent=self.parent, title=basename(self.filename))


    def mousePressEvent(self, QMouseEvent):
        print('MousePress in Sim Container')
        self.parent.list_gadget.clear()
        for k, v in self.simulation_stats.items():
            self.parent.list_gadget.addItem(f'{k}: {v}')
        
        # Pop up ep browser
        self.parent.ep_browser.initialize(self)


class EpisodeContainer(StackedDisplay):
        
    def __init__(self, episode, occlusions, parent=None, stats=None, view_belief=True, view_prey=True, view_predator=True):
        self.episode = episode
        self.parent = parent
        self.stats = stats
        self.world = World.get_from_parameters_names("hexagonal", "canonical")
        self.occlusions = occlusions
        self.world.set_occlusions(occlusions)
        self.display = Display(self.world, fig_size=(5, 5))
        #self.render = FigureCanvasQTAgg(self.display.fig)
        #self.render.setFixedSize(300, 300)
        self.view_belief = view_belief
        self.view_prey = view_prey
        self.view_predator = view_predator
        self.predator_destination_arrow = None

        # For step map draw on self.display

        self.pred_counters, self.prey_counters = self.get_heatmap()
        self.hm = HeatMap(self.pred_counters, self.prey_counters, self.world, title="", parent=self.parent)
        self.ep_stats = vars(self.stats)

        self.detail_view = DetailView(
            parent=self.parent, 
            stacked_disp=StackedDisplay(self.parent, self.ep_stats,
                map = StepMap(self.world, self.episode, self, "")), pred_vals=self.pred_vals, prey_vals=self.prey_vals)
            

        super().__init__(self.parent, self.ep_stats, self.hm)

    def mousePressEvent(self, QMouseEvent):
        self.detail_view.show()
        self.detail_view.stacked_disp.map.pp_thread.start()

    def get_heatmap(self):

        prey_counters = [0 for x in self.world.cells]
        pred_counters = [0 for x in self.world.cells]

        self.prey_vals = []
        self.pred_vals = []
        
        for frame in range(len(self.episode)):

            # Prey stuff
            prey_state = self.episode[frame].prey_state
            prey_vals = prey_state.options_values
            prey_vals.append(0)
            self.prey_vals.append((frame, -1 * min(prey_vals)))
            prey_counters[prey_state.cell_id] += 1

            pred_state = self.episode[frame].predator_state
            if hasattr(pred_state, 'options_values'):
                pred_vals = pred_state.options_values
                pred_vals.append(0)
                self.pred_vals.append((frame, -1 * min(pred_vals)))
            pred_counters[pred_state.cell_id] += 1

        return pred_counters, prey_counters

    def show_step(self, frames: list = None):

        if self.paused:
            return

        prey_state = self.episode[self.current_frame].prey_state
        prey_cell = self.world.cells[prey_state.cell_id]

        predator_state = self.episode[self.current_frame].predator_state
        predator_cell = self.world.cells[predator_state.cell_id]
        predator_destination_cell = self.world.cells[predator_state.destination_id]
        upper_limit = 0

        if prey_state.belief_state:
            upper_limit = max(prey_state.belief_state)

        if upper_limit > 0:
            cmap = plt.cm.Reds([x / upper_limit for x in prey_state.belief_state])

        for cell in self.world.cells:
            color = "white"
            if upper_limit > 0 and self.view_belief:
                color = cmap[cell.id]
            if prey_cell.id == cell.id and self.view_prey:
                color = "green"
            if predator_cell.id == cell.id and self.view_predator:
                color = "blue"
            if self.world.cells[cell.id].occluded:
                color = "black"
            self.display.cell(cell=cell, color=color)

        if self.view_predator:
            self.predator_destination_arrow = self.display.arrow(beginning=predator_cell.location, ending=predator_destination_cell.location, color="blue", existing_arrow=self.predator_destination_arrow)
            self.predator_destination_arrow.set_visible(True)

        self.display.update()
        self.render.draw()

        if frames is not None:
            image_from_plot = np.frombuffer(self.render.tostring_rgb(), dtype='uint8').reshape(self.render.get_width_height()[::-1] + (3,))
            frames.append(image_from_plot)

        # Increment current frame and check if we have reached the end
        self.current_frame += 1
        if self.current_frame == len(self.episode):
            self.current_frame = 0
            self.paused = True


class Example(QMainWindow):

    def play_pause(self):
        self.tick_paused = not self.tick_paused
        if self.tick_paused:
            self.playAct.setText("&Play")
        else:
            self.playAct.setText("&Pause")

    def save_video(self):
        self.tick_paused = True
        video_file = QFileDialog.getSaveFileName(self, 'Save video', '.', "Video file (*.mp4)")[0]
        self.current_frame = 0
        frames = []
        for i in range(len(self.current_episode)):
            self.current_frame = i
            self.show_step(frames=frames)
        clips = [ImageClip(m).set_duration(.05) for m in frames]
        fps = 30
        concat_clip = concatenate_videoclips(clips, method="compose")
        concat_clip.write_videofile(video_file, fps=fps, codec="mpeg4")

    def init_menu(self):
        exitAct = QAction('&Exit', self)
        exitAct.setShortcut('Ctrl+Q')
        exitAct.setStatusTip('Exit application')
        exitAct.triggered.connect(QApplication.instance().quit)

        openAct = QAction('&Open...', self)
        openAct.setShortcut('Ctrl+O')
        openAct.setStatusTip('Open results')
        openAct.triggered.connect(self.open_results)


        saveframeAct = QAction('&Save frame...', self)
        saveframeAct.setShortcut('Ctrl+F')
        saveframeAct.setStatusTip('Save frame to png')
        saveframeAct.triggered.connect(self.save_video)

        saveAct = QAction('&Save video...', self)
        saveAct.setShortcut('Ctrl+S')
        saveAct.setStatusTip('Save episode video file')
        saveAct.triggered.connect(self.save_video)

        menubar = self.menuBar()
        fileMenu = menubar.addMenu('&File')

        fileMenu.addAction(openAct)
        fileMenu.addAction(saveframeAct)
        fileMenu.addAction(saveAct)
        fileMenu.addSeparator()
        fileMenu.addAction(exitAct)

        self.view_prey = QAction('Show prey', self, checkable=True)
        self.view_prey.setChecked(True)
        self.view_prey.setStatusTip('Show prey trajectory')

        self.view_predator = QAction('Show predator', self, checkable=True)
        self.view_predator.setChecked(True)
        self.view_predator.setStatusTip('Show predator trajectory')

        self.view_belief = QAction('View belief state', self, checkable=True)
        self.view_belief.setChecked(True)
        self.view_belief.setStatusTip('Show belief state')

        viewMenu = menubar.addMenu('&View')


        viewMenu.addAction(self.view_prey)
        viewMenu.addAction(self.view_predator)
        viewMenu.addAction(self.view_belief)

        self.playAct = QAction('&Play', self)
        self.playAct.setShortcut('Ctrl+SPACE')
        self.playAct.setStatusTip('Play/Pause episode replay')
        self.playAct.triggered.connect(self.play_pause)

        menubar = self.menuBar()
        replayMenu = menubar.addMenu('&Replay')

        replayMenu.addAction(self.playAct)

    def __init__(self):
        super().__init__()

        # Within this class there is:
        # 1. a world class
        # 2. a display class which has a plt.fig parent
        # 3. some methods for setting up the menu and status bar
        # 4. 'central' QWidget obj -> passed to self.setCentralWidget()
        # 5. 3 other Qwidgets
        # 6. a QGridLayout on the central object to which the 3 other widgets are added
        # 7. a layout for the subsequent 3 widgets
        
        # I will add:
        # A scrollable viewer for viewing all of the episodes
        # For now add a world and render for each episode loaded in the side bar

        # List of (episode, world, display, render) tuples for each episode
        self.all_eps = [] 

        self.ep_browser = EpBrowser(parent=self)


        self.init_menu()
        self.statusBar()
        
        self.setGeometry(100, 100, 1000, 700)
        self.setWindowTitle("Result Browser V2")

        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)

        self.m_w11 = QWidget()
        
        lay = QGridLayout(self.central_widget)
        lay.addWidget(self.m_w11, 0, 0)
        self.init_scroll_area()
        lay.addWidget(self.scroll_area, 0, 1)
        lay.setColumnStretch(1, 1)

        lay = QVBoxLayout(self.m_w11)
        self.list_gadget = QListWidget()
        lay.addWidget(self.list_gadget)

        self.simulation = None
        self.current_episode = None
        self.current_episode_index = None
        self.prey_trajectory = None
        self.predator_trajectory = None
        self.current_frame = 0
        self.show()

    def init_scroll_area(self):
        self.scroll_area = QScrollArea(self.central_widget)             # Scroll Area which contains the widgets
        self.scroll_widget = QWidget()          # Widget that contains the collection of Vertical Box
        self.scroll_grid = QGridLayout()               # The Vertical Box that contains the Horizontal Boxes of world figures

        # self.world = World.get_from_parameters_names("hexagonal", "canonical")
        # self.display = Display(self.world, fig_size=(3, 3))

        # Add renders in loop here!
        # for _ in range(1,8):
        #     render_i = FigureCanvasQTAgg(self.display.fig)
        #     render_i.setFixedSize(600, 600)
        #     self.vbox.addWidget(render_i)

        self.scroll_widget.setLayout(self.scroll_grid)

        #Scroll Area Properties
        self.scroll_area.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOn)
        self.scroll_area.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.scroll_area.setWidgetResizable(True)
        self.scroll_area.setWidget(self.scroll_widget)

    def display_renders(self, renders):
        # Clear old EpContainers
        self.clearLayout(self.scroll_grid)
        pos = [0, 0]
        for render in renders:
            self.scroll_grid.addWidget(render, pos[0], pos[1])
            if pos[1] == 1:
                pos[1] = 0
                pos[0] += 1
            else:
                pos[1] += 1

    def clearLayout(self, layout):
        while layout.count():
            child = layout.takeAt(0)
            if child.widget():
                child.widget().deleteLater()

    def open_results(self):
        file_names = QFileDialog.getOpenFileNames(self, 'Open file', '.', "Simulation results (*.json)")[0]
        hms = []

        pairs = []
        for f in file_names:

            # Check we have experiment and sim
            if 'stats' not in f:
                dir = os.path.dirname(f)
                fname, ext = os.path.splitext(f)
                stats_name = os.path.join(dir, fname + '_stats' + ext)
                if False:#not os.path.exists(stats_name):
                    raise Exception(f'Expected stats file: {stats_name} to exist for experiment file {f}')
                else:
                    pairs.append((f, stats_name))
                
        for p in pairs:
            s = SimulationContainter(p[0], p[1], self)
            hms.append(s)
        
        self.display_renders(hms)


def main():
    app = QApplication(sys.argv)
    ex = Example()
    sys.exit(app.exec())


if __name__ == '__main__':
    main()
