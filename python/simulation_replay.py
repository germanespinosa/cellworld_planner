#!/usr/bin/python
from src.episode_replay import *
from src import *
from moviepy.editor import *
from PyQt6.QtGui import *
from cellworld import *
from PyQt6.QtWidgets import *
from PyQt6.QtCore import *

class Simulation_replay(QMainWindow):

    def __init__(self, simulation: Simulation = None, simulation_statistics: Simulation_statistics = None, folder: str = ""):
        super().__init__()
        self.world = None
        self.world_statistics = None
        self.simulation_statistics = None
        self.occlusions = None
        self.tick_paused = False
        self.init_menu()
        self.statusBar()
        self.setGeometry(0, 0, 1100, 600)
        self.setWindowTitle("Simulation replay")

        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        self.left_pane = QWidget()
        self.central_pane = QWidget()
        self.right_pane = QWidget()

        lay = QGridLayout(central_widget)
        lay.addWidget(self.left_pane, 0, 0)
        lay.addWidget(self.central_pane, 0, 1)
        lay.addWidget(self.right_pane, 0, 2)
        lay.setColumnStretch(1, 1)

        lay = QVBoxLayout(self.left_pane)

        self.list_gadget = QListWidget()
        lay.addWidget(self.list_gadget)
        self.list_gadget.currentItemChanged.connect(self.episode_selected)
        lay = QVBoxLayout(self.central_pane)
        self.episode_replay = Episode_replay()
        lay.addWidget(self.episode_replay)

        lay = QVBoxLayout(self.right_pane)

        self.stats = QTextEdit()
        lay.addWidget(self.stats)

        self.simulation = None
        self.current_episode = None
        self.current_episode_index = None
        self.prey_trajectory = None
        self.predator_trajectory = None
        self.current_frame = 0
        if (simulation):
            self.load_simulation(simulation, simulation_statistics, folder)
        self.show()

    def episode_selected(self, i: QListWidgetItem):
        if i is None:
            return
        self.current_episode_index = int(i.text().split(" ")[0])
        self.episode_replay.set_episode(self.simulation.episodes[self.current_episode_index])
        self.current_frame = 0
        self.tick_paused = False

    def open_results(self):
        file_name = QFileDialog.getOpenFileName(self, 'Open file', '.', "Simulation results (*.json)")[0]
        self.load_results(file_name)

    def load_results(self, file_name):
        simulation = Simulation.load_from_file(file_name)
        stats = Simulation_statistics.load_from_sim_file_name(file_name)
        self.load_simulation(simulation, stats)

    def load_items(self):
        self.list_gadget.clear()
        episode_list = []
        for x in range(len(self.simulation.episodes)):
            if self.simulation_statistics.episode_stats[x].survival_rate == 1:
                if self.view_successful.isChecked():
                    episode_list.append("{:03d} success".format(x))
            else:
                if self.simulation_statistics.episode_stats[x].capture_rate > 0:
                    if self.view_failed.isChecked():
                        episode_list.append("{:03d} failed".format(x))
                else:
                    if self.view_timed_out.isChecked():
                        episode_list.append("{:03d} timed_out".format(x))



        self.list_gadget.addItems(episode_list)

    def load_simulation(self, simulation: Simulation, simulation_statistics: Simulation_statistics, folder: str = ""):
        self.simulation = simulation
        self.simulation_statistics = simulation_statistics
        #self.list_gadget.addItems(["{:03d}".format(x) for x in range(len(self.simulation.episodes))])
        stat_text = self.simulation_statistics.format("Survival_rate: {survival_rate:.4f}\nCapture_rate: {capture_rate:.4f}\nPursue_rate: {pursue_rate:.4f}\nBelief_state_entropy: {belief_state_entropy:.4f}\nLength: {length:.4f}\nDistance: {distance:.4f}\nVisited_cells: {visited_cells:.4f}\nValue: {value:.4f}\nDecision difficulty: {decision_difficulty:.4f}")
        self.stats.setText(stat_text)
        self.world = World.get_from_parameters_names(self.simulation.world_info.world_configuration,self.simulation.world_info.world_implementation)
        self.load_items()
        if folder:
            self.world_statistics = World_statistics.load(folder + "/world_statistics")
            self.occlusions = Cell_group_builder.load(folder + "/occlusions")
        else:
            self.world_statistics = get_resource("world_statistics", self.simulation.world_info.world_configuration, self.simulation.world_info.occlusions)
            self.occlusions = Cell_group_builder.get_from_name(self.simulation.world_info.world_configuration, self.simulation.world_info.occlusions, "occlusions")
        self.world.set_occlusions(self.occlusions)
        self.episode_replay.set_world(self.world)


    @staticmethod
    def get_simulation(file_name) -> tuple:
        sim = Simulation.load_from_file(file_name)
        stats = Simulation_statistics.load_from_sim_file_name(file_name)
        folder = None
        if "random_world" in sim.world_info.occlusions:
            folder = "/".join(file_name.split("/")[:-1])
        return sim, stats, folder

    def save_video(self):
        self.tick_paused = True
        video_file, codec = QFileDialog.getSaveFileName(self, 'Save video', '.', "libx265 (*.mp4);;h264 (*.mp4);;mpeg4 (*.mp4);;libx264rgb (*.mp4);;png (*.avi);;rawvideo (*.avi);;png (*.avi);;libvorbis (*.ogv);;libvpx (*.webm);;qtrle (*.mov)")
        self.current_frame = 0
        fps = 30
        codec = codec.split(" ")[0];
        self.episode_replay.save_video(video_file, codec, fps)

    def init_menu(self):
        exitAct = QAction('&Exit', self)
        exitAct.setShortcut('Ctrl+Q')
        exitAct.setStatusTip('Exit application')
        exitAct.triggered.connect(QApplication.instance().quit)

        openAct = QAction('&Open...', self)
        openAct.setShortcut('Ctrl+O')
        openAct.setStatusTip('Open results')
        openAct.triggered.connect(self.open_results)

        saveAct = QAction('&Save video...', self)
        saveAct.setShortcut('Ctrl+S')
        saveAct.setStatusTip('Save episode video file')
        saveAct.triggered.connect(self.save_video)

        menubar = self.menuBar()
        fileMenu = menubar.addMenu('&File')

        fileMenu.addAction(openAct)
        fileMenu.addSeparator()
        fileMenu.addAction(saveAct)
        fileMenu.addAction(exitAct)

        self.view_successful = QAction('Show successful episodes', self, checkable=True)
        self.view_successful.setChecked(True)
        self.view_successful.setStatusTip('Show successful episodes')
        self.view_successful.triggered.connect(self.load_items)

        self.view_failed = QAction('Show failed episodes', self, checkable=True)
        self.view_failed.setChecked(True)
        self.view_failed.setStatusTip('Show failed episodes')
        self.view_failed.triggered.connect(self.load_items)

        self.view_timed_out = QAction('Show timed out episodes', self, checkable=True)
        self.view_timed_out.setChecked(True)
        self.view_timed_out.setStatusTip('Show failed episodes')
        self.view_timed_out.triggered.connect(self.load_items)

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
        viewMenu.addAction(self.view_successful)
        viewMenu.addAction(self.view_failed)
        viewMenu.addAction(self.view_timed_out)
        fileMenu.addSeparator()
        viewMenu.addAction(self.view_prey)
        viewMenu.addAction(self.view_predator)
        viewMenu.addAction(self.view_prey_plan)
        viewMenu.addAction(self.view_predator_destination)
        viewMenu.addAction(self.view_belief)

def main():
    app = QApplication(sys.argv)
    if len(sys.argv) == 2:
        sim, stats, folder = Simulation_replay.get_simulation(sys.argv[1])
        ex = Simulation_replay(sim, stats, folder)
    else:
        ex = Simulation_replay()
    sys.exit(app.exec())


if __name__ == '__main__':
    main()