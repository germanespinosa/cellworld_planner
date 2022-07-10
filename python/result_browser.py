#!/usr/bin/python
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
from matplotlib.figure import Figure
from moviepy.editor import *


reward = Reward(step_cost=1, capture_cost=100)


class Example(QMainWindow):

    def show_step(self, frames: list = None):
        prey_state = self.current_episode[self.current_frame].prey_state
        prey_cell = self.world.cells[prey_state.cell_id]

        predator_state = self.current_episode[self.current_frame].predator_state
        predator_cell = self.world.cells[predator_state.cell_id]
        predator_destination_cell = self.world.cells[predator_state.destination_id]
        upper_limit = 0

        if prey_state.belief_state:
            upper_limit = max(prey_state.belief_state)

        if upper_limit > 0:
            cmap = plt.cm.Reds([x / upper_limit for x in prey_state.belief_state])

        for cell in self.world.cells:
            outline = None
            color = "white"
            if upper_limit > 0 and self.view_belief.isChecked():
                color = cmap[cell.id]
            if prey_cell.id == cell.id and self.view_prey.isChecked():
                color = "green"
                if prey_state.capture:
                    outline = "red"
            if predator_cell.id == cell.id and self.view_predator.isChecked():
                color = "blue"
                if prey_state.capture:
                    outline = "red"
            if self.world.cells[cell.id].occluded:
                color = "black"
            self.display.cell(cell=cell, color=color, outline_color=outline)

        if self.view_predator_destination.isChecked():
            self.predator_destination_arrow = self.display.arrow(beginning=predator_cell.location, ending=predator_destination_cell.location, color="blue", existing_arrow=self.predator_destination_arrow)
            self.predator_destination_arrow.set_visible(True)
        else:
            if self.predator_destination_arrow:
                self.predator_destination_arrow.set_visible(False)

        if self.view_prey_plan.isChecked():
            prev = prey_cell
            [arrow.set_visible(False) for arrow in self.plan_arrows if arrow is not None]
            for i, plan_step_cell_id in enumerate(prey_state.plan):
                next = self.world.cells[plan_step_cell_id]
                self.plan_arrows[i] = self.display.arrow(beginning=prev.location, ending=next.location, color="green", existing_arrow=self.plan_arrows[i])
                self.plan_arrows[i].set_visible(True)
                prev = next



        self.display.update()
        self.render.draw()

        if frames is not None:
            image_from_plot = np.frombuffer(self.render.tostring_rgb(), dtype='uint8').reshape(self.render.get_width_height()[::-1] + (3,))
            frames.append(image_from_plot)

    def play_pause(self, e):
        self.tick_paused = not self.tick_paused
        if self.tick_paused:
            self.playAct.setText("&Play")
        else:
            self.playAct.setText("&Pause")

    def save_video(self):
        self.tick_paused = True
        video_file, codec = QFileDialog.getSaveFileName(self, 'Save video', '.', "libx265 (*.mp4);;h264 (*.mp4);;mpeg4 (*.mp4);;libx264rgb (*.mp4);;png (*.avi);;rawvideo (*.avi);;png (*.avi);;libvorbis (*.ogv);;libvpx (*.webm);;qtrle (*.mov)")
        self.current_frame = 0
        frames = []
        print("Processing video")
        for i in range(len(self.current_episode)):
            print("Frame", i, "from", len(self.current_episode))
            self.current_frame = i
            self.show_step(frames=frames)
        clips = [ImageClip(m).set_duration(self.tick_interval) for m in frames]
        fps = 30
        concat_clip = concatenate_videoclips(clips)
        codec = codec.split(" ")[0];
        concat_clip.write_videofile(video_file, fps=fps, codec=codec)

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

        self.view_prey_plan = QAction('Show prey plan', self, checkable=True)
        self.view_prey_plan.setChecked(False)
        self.view_prey_plan.setStatusTip('Show prey plan')

        self.view_predator = QAction('Show predator', self, checkable=True)
        self.view_predator.setChecked(True)
        self.view_predator.setStatusTip('Show predator trajectory')

        self.view_predator_destination = QAction('Show predator destination', self, checkable=True)
        self.view_predator_destination.setChecked(False)
        self.view_predator_destination.setStatusTip('Show predator destination')


        self.view_belief = QAction('View belief state', self, checkable=True)
        self.view_belief.setChecked(True)
        self.view_belief.setStatusTip('Show belief state')

        viewMenu = menubar.addMenu('&View')

        viewMenu.addAction(self.view_prey)
        viewMenu.addAction(self.view_predator)
        viewMenu.addAction(self.view_prey_plan)
        viewMenu.addAction(self.view_predator_destination)
        viewMenu.addAction(self.view_belief)

        self.playAct = QAction('&Play', self)
        self.playAct.setShortcut('SPACE')
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

    def tick_speed_up(self):
        if self.tick_interval-.1 > 0:
            self.tick_interval -= .1

    def tick_speed_down(self):
        if self.tick_interval+.1 < 2:
            self.tick_interval += .1

    def tick_control(self):
        t = Timer(.116)
        while self.tick_active:
            while t:
                pass
            t.reset()
            if not self.tick_paused:
                self.tick()

    def __init__(self):
        super().__init__()

        self.world = World.get_from_parameters_names("hexagonal", "canonical")
        self.display = Display(self.world, fig_size=(6, 5))
        self.predator_destination_arrow = None
        self.plan_arrows = [None for x in range(100)]

        self.tick_counter = 0
        self.tick_active = True
        self.tick_paused = True
        self.tick_interval = .2
        self.tick_thread = Thread(target=self.tick_control)
        self.tick_thread.start()

        self.init_menu()
        self.statusBar()
        self.setGeometry(100, 100, 1300, 750)
        self.setWindowTitle("Result Browser")

        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        self.m_w11 = QWidget()
        self.m_w12 = QWidget()
        self.m_w13 = QWidget()

        self.r_w11 = QWidget()
        self.r_w21 = QWidget()
        self.r_w31 = QWidget()
        self.r_w41 = QWidget()

        lay = QGridLayout(central_widget)
        lay.addWidget(self.m_w11, 0, 0)
        lay.addWidget(self.m_w12, 0, 1)
        lay.addWidget(self.m_w13, 0, 2)
        lay.setColumnStretch(1, 1)

        replay_lay = QGridLayout(self.m_w12)
        replay_lay.addWidget(self.r_w11, 0, 0)
        replay_lay.addWidget(self.r_w21, 1, 0)
        replay_lay.addWidget(self.r_w31, 2, 0)
        replay_lay.addWidget(self.r_w41, 3, 0)

        lay = QVBoxLayout(self.m_w11)

        self.list_gadget = QListWidget()
        lay.addWidget(self.list_gadget)

        self.list_gadget.currentItemChanged.connect(self.episode_selected)

        lay = QVBoxLayout(self.m_w12)
        self.render = FigureCanvasQTAgg(self.display.fig)
        lay.addWidget(self.render)

        lay = QVBoxLayout(self.m_w13)

        self.stats = QTextEdit()
        lay.addWidget(self.stats)

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
        self.tick_paused = False

    def open_results(self):
        file_name = QFileDialog.getOpenFileName(self, 'Open file', '.', "Simulation results (*.json)")[0]
        self.simulation = Simulation.load_from_file(file_name)
        self.world.set_occlusions(Cell_group_builder.get_from_name(world_name="hexagonal." + self.simulation.world_info.occlusions, name="occlusions"))
        self.list_gadget.addItems(["{:03d}".format(x) for x in range(len(self.simulation.episodes))])
        stats = Simulation_statistics.load_from_sim_file_name(file_name)
        if stats is None:
            stats = self.simulation.get_stats(reward=reward)
        stat_text = stats.format("Survival_rate: {survival_rate:.4f}\nCapture_rate: {capture_rate:.4f}\nPursue_rate: {pursue_rate:.4f}\nBelief_state_entropy: {belief_state_entropy:.4f}\nLength: {length:.4f}\nDistance: {distance:.4f}\nVisited_cells: {visited_cells:.4f}\nValue: {value:.4f}\nDecision difficulty: {decision_difficulty:.4f}")
        self.stats.setText(stat_text)


def main():
    app = QApplication(sys.argv)
    ex = Example()
    sys.exit(app.exec())


if __name__ == '__main__':
    main()
