#!/usr/bin/python
from src import *
from moviepy.editor import *
from PyQt6.QtGui import *
from cellworld import *

class Simulation_replay(QMainWindow):

    def __init__(self, simulation: Simulation = None, simulation_statistics: Simulation_statistics = None):
        super().__init__()

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
        self.load_simulation(simulation, simulation_statistics)
        self.show()

    def episode_selected(self, i: QListWidgetItem):
        self.current_episode_index = int(i.text())
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

    def load_simulation(self, simulation: Simulation, simulation_statistics: Simulation_statistics):
        self.simulation = simulation
        self.simulation_statistics = simulation_statistics
        self.list_gadget.addItems(["{:03d}".format(x) for x in range(len(self.simulation.episodes))])
        stat_text = self.simulation_statistics.format("Survival_rate: {survival_rate:.4f}\nCapture_rate: {capture_rate:.4f}\nPursue_rate: {pursue_rate:.4f}\nBelief_state_entropy: {belief_state_entropy:.4f}\nLength: {length:.4f}\nDistance: {distance:.4f}\nVisited_cells: {visited_cells:.4f}\nValue: {value:.4f}\nDecision difficulty: {decision_difficulty:.4f}")
        self.stats.setText(stat_text)
        self.world_statistics = get_resource("world_statistics", "hexagonal", self.simulation.world_info.occlusions)
        self.episode_replay.set_occlusions(Cell_group_builder.get_from_name("hexagonal", self.simulation.world_info.occlusions, "occlusions"))

    @staticmethod
    def get_simulation(file_name) -> tuple:
        sim = Simulation.load_from_file(file_name)
        stats = Simulation_statistics.load_from_sim_file_name(file_name)
        return sim, stats

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

def main():
    app = QApplication(sys.argv)
    if len(sys.argv) == 2:
        sim, stats = Simulation_replay.get_simulation(sys.argv[1])
        ex = Simulation_replay(sim, stats)
    else:
        ex = Simulation_replay()
    sys.exit(app.exec())


if __name__ == '__main__':
    main()