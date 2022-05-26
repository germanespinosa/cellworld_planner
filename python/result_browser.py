#!/usr/bin/python

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
from matplotlib.figure import Figure



class Example(QMainWindow):

    def show_step(self):
        prey_state = self.current_episode[self.current_frame].prey_state
        prey_cell_id = prey_state.cell_id
        predator_state = self.current_episode[self.current_frame].predator_state
        predator_cell_id = predator_state.cell_id
        upper_limit = 0
        if prey_state.belief_state:
            upper_limit = max(prey_state.belief_state)
        if upper_limit > 0:
            cmap = plt.cm.Reds([x / upper_limit for x in prey_state.belief_state])

        for cell in self.world.cells:
            color = "white"
            if upper_limit > 0:
                color = cmap[cell.id]
            if prey_cell_id == cell.id:
                color = "green"
            if predator_cell_id == cell.id:
                color = "blue"
            if self.world.cells[cell.id].occluded:
                color = "black"
            self.display.cell(cell=cell, color=color)

        self.display.update()
        self.render.draw()

    def play_pause(self, e):
        self.tick_paused = not self.tick_paused
        if self.tick_paused:
            self.playAct.setText("&Play")
        else:
            self.playAct.setText("&Pause")

    def init_menu(self):
        exitAct = QAction('&Exit', self)
        exitAct.setShortcut('Ctrl+Q')
        exitAct.setStatusTip('Exit application')
        exitAct.triggered.connect(QApplication.instance().quit)

        openAct = QAction('&Open', self)
        openAct.setShortcut('Ctrl+O')
        openAct.setStatusTip('Open results')
        openAct.triggered.connect(self.open_results)

        menubar = self.menuBar()
        fileMenu = menubar.addMenu('&File')

        fileMenu.addAction(openAct)
        fileMenu.addAction(exitAct)

        view_prey_trajectoryAct = QAction('&View prey', self, checkable=True)
        view_prey_trajectoryAct.setChecked(True)
        view_prey_trajectoryAct.setShortcut('Ctrl+P')
        view_prey_trajectoryAct.setStatusTip('View prey trajectory')
        #view_prey_trajectoryAct.triggered.connect(self.open_results)

        view_predator_trajectoryAct = QAction('&View predator', self, checkable=True)
        view_predator_trajectoryAct.setChecked(True)
        view_predator_trajectoryAct.setShortcut('Ctrl+R')
        view_predator_trajectoryAct.setStatusTip('View predator trajectory')
        #view_prey_trajectoryAct.triggered.connect(self.open_results)

        view_predator_trajectoryAct = QAction('&View spawn locations', self, checkable=True)
        view_predator_trajectoryAct.setChecked(True)
        view_predator_trajectoryAct.setShortcut('Ctrl+S')
        view_predator_trajectoryAct.setStatusTip('View predator trajectory')
        #view_prey_trajectoryAct.triggered.connect(self.open_results)

        viewMenu = menubar.addMenu('&View')


        viewMenu.addAction(view_prey_trajectoryAct)
        viewMenu.addAction(view_predator_trajectoryAct)

        self.playAct = QAction('&Play', self)
        self.playAct.setShortcut('Ctrl+SPACE')
        self.playAct.setStatusTip('Play/Pause episode replay')
        self.playAct.triggered.connect(self.play_pause)

        fasterAct = QAction('&Speed UP', self)
        fasterAct.setShortcut('Ctrl+UP')
        fasterAct.setStatusTip('Speed up episode replay')
        fasterAct.triggered.connect(self.tick_speed_up)

        slowerAct = QAction('&Speed DOWN', self)
        slowerAct.setShortcut('Ctrl+DOWN')
        slowerAct.setStatusTip('Speed down episode replay')
        slowerAct.triggered.connect(self.tick_speed_down)

        menubar = self.menuBar()
        replayMenu = menubar.addMenu('&Replay')

        replayMenu.addAction(self.playAct)
        replayMenu.addAction(fasterAct)
        replayMenu.addAction(slowerAct)

    def tick(self):
        self.show_step()
        self.current_frame += 1
        if self.current_frame == len(self.current_episode):
            self.current_frame = 0
            self.tick_paused = True
        self.tick_counter += 1
        print("tick", self.tick_counter)

    def tick_speed_up(self):
        if self.tick_interval-.1 > 0:
            self.tick_interval -= .1

    def tick_speed_down(self):
        if self.tick_interval+.1 < 2:
            self.tick_interval += .1

    def tick_control(self):
        while self.tick_active:
            sleep(self.tick_interval)
            if not self.tick_paused:
                self.tick()

    def __init__(self):
        super().__init__()

        self.world = World.get_from_parameters_names("hexagonal", "canonical")
        self.display = Display(self.world, fig_size=(6, 5))

        self.tick_counter = 0
        self.tick_active = True
        self.tick_paused = True
        self.tick_interval = .5
        self.tick_thread = Thread(target=self.tick_control)
        self.tick_thread.start()

        self.init_menu()
        self.statusBar()
        self.setGeometry(100, 100, 1000, 700)
        self.setWindowTitle("Result Browser")

        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        self.m_w11 = QWidget()
        self.m_w12 = QWidget()

        lay = QGridLayout(central_widget)
        lay.addWidget(self.m_w11, 0, 0)
        lay.addWidget(self.m_w12, 0, 1)
        lay.setColumnStretch(1, 1)

        lay = QVBoxLayout(self.m_w11)

        self.list_gadget = QListWidget()
        lay.addWidget(self.list_gadget)

        self.list_gadget.currentItemChanged.connect(self.episode_selected)

        lay = QVBoxLayout(self.m_w12)
        self.render = FigureCanvasQTAgg(self.display.fig)
        lay.addWidget(self.render)


        self.simulation = None
        self.current_episode = None
        self.current_episode_index = None
        self.prey_trajectory = None
        self.predator_trajectory = None
        self.current_frame = 0
        self.show()

    def episode_selected(self, i: QListWidgetItem):
        self.current_episode_index = int(i.text())
        self.current_episode = self.simulation.episodes[self.current_episode_index]
        self.current_frame = 0
        print("Episode", self.current_episode_index, "selected")

    def open_results(self):
        fname = QFileDialog.getOpenFileName(self, 'Open file', '.', "Simulation results (*.json)")[0]
        self.simulation = Simulation.load_from_file(fname)
        self.world.set_occlusions(Cell_group_builder.get_from_name(world_name="hexagonal." + self.simulation.world_info.occlusions, name="occlusions"))
        self.list_gadget.addItems(["{:03d}".format(x) for x in range(len(self.simulation.episodes))])

def main():

    app = QApplication(sys.argv)
    ex = Example()
    sys.exit(app.exec())


if __name__ == '__main__':
    main()