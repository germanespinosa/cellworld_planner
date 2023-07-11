from .video_writer import VideoWriter
from cellworld import *
from PyQt6.QtWidgets import *
from PyQt6.QtCore import *
from .simulation import *
from threading import Thread
matplotlib.use('Qt5Agg')
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg


class Episode_replay_window(QMainWindow):
    def __init__(self, simulation: Simulation, episode_number:int ):
        self.simulation = simulation
        self.episode_number = episode_number
        super().__init__()
        self.replay_gadget = Episode_replay()
        self.replay_gadget.set_occlusions(Cell_group_builder.get_from_name(self.simulation.world_info.world_configuration, self.simulation.world_info.occlusions, "occlusions"))
        self.setCentralWidget(self.replay_gadget)
        self.show()


class Episode_replay(QWidget):

    def __init__(self):
        super().__init__()
        self.world = World.get_from_parameters_names("hexagonal", "canonical")
        self.display = Display(self.world, fig_size=(10, 10))
        self.predator_destination_arrow = None
        self.plan_arrows = [None for x in range(100)]
        self.tick_counter = 0
        self.tick_active = True
        self.tick_paused = True
        self.tick_interval = .2
        self.tick_thread = Thread(target=self.tick_control)
        self.tick_thread.start()

        self.view_belief = True
        self.view_predator = True
        self.view_prey = True
        self.view_predator_destination = False
        self.view_prey_plan = False
        self.stop_at_capture = True


        self.setWindowTitle("Episode replay")
        self.tick_interval = .116
        self.init_layout()
        self.current_episode = None
        self.current_episode_index = 0
        self.prey_trajectory = None
        self.predator_trajectory = None
        self.current_frame = 0
        self.episode_progress_event = None
        self.show()

    @staticmethod
    def show_episode(simulation: Simulation, episode_number: int):
        Episode_replay_window(simulation, episode_number)

    def set_world(self, world: World):
        self.world = world
        self.display = Display(self.world, fig_size=(10, 10))
        self.episode_render = FigureCanvasQTAgg(self.display.fig)
        for i in reversed(range(self.central_layout.count())):
            self.central_layout.itemAt(i).widget().setParent(None)
        self.central_layout.addWidget(self.episode_render, 0, 0, Qt.AlignmentFlag.AlignTop)
        self.episode_render.draw()

    def set_episode(self, episode):
        self.current_episode = episode
        print("Episode selected")
        self.tick_paused = False
        self.current_frame = 0

    def show_step(self, video_writer: VideoWriter = None):
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
            # if upper_limit > 0 and self.view_belief:
            #     color = cmap[cell.id]
            if cell.id == 330:
                color = "orange"
            if prey_cell.id == cell.id and self.view_prey:
                color = "blue"
                if prey_state.capture:
                    outline = "red"
            if predator_cell.id == cell.id and self.view_predator:
                color = "red"
                if prey_state.capture:
                    outline = "red"
            if self.world.cells[cell.id].occluded:
                color = "black"
            self.display.cell(cell=cell, color=color, outline_color=outline)

        if self.view_predator_destination:
            self.predator_destination_arrow = self.display.arrow(beginning=predator_cell.location, ending=predator_destination_cell.location, color="blue", existing_arrow=self.predator_destination_arrow)
            self.predator_destination_arrow.set_visible(True)
        else:
            if self.predator_destination_arrow:
                self.predator_destination_arrow.set_visible(False)

        if self.view_prey_plan:
            prev = prey_cell
            [arrow.set_visible(False) for arrow in self.plan_arrows if arrow is not None]
            for i, plan_step_cell_id in enumerate(prey_state.plan):
                next = self.world.cells[plan_step_cell_id]
                self.plan_arrows[i] = self.display.arrow(beginning=prev.location, ending=next.location, color="green", existing_arrow=self.plan_arrows[i])
                self.plan_arrows[i].set_visible(True)
                prev = next

        self.display.update()
        self.episode_render.draw()

        if video_writer is not None:
            video_writer.write_figure(self.display.fig, duration=self.tick_interval)

    def play_pause(self, e):
        self.tick_paused = not self.tick_paused
        if self.tick_paused:
            self.playAct.setText("&Play")
        else:
            self.playAct.setText("&Pause")

    def save_video(self, video_file: str, codec: str, fps: int = 30):
        self.tick_paused = True
        self.current_frame = 0
        frames = []
        print("Processing video")
        video_writer = VideoWriter(video_file, self.display.fig.canvas.get_width_height(), fps)
        for i in range(len(self.current_episode)):
            print("Step", i, "of", len(self.current_episode))
            self.current_frame = i
            self.show_step(video_writer=video_writer)
            if  self.stop_at_capture and self.current_episode[self.current_frame].prey_state.capture:
                break
        video_writer.close()

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
        from time import sleep
        while self.tick_active:
            sleep(self.tick_interval)
            if not self.tick_paused:
                self.tick()

    def init_layout(self):
        self.central_layout = QGridLayout(self)
        self.episode_render = FigureCanvasQTAgg(self.display.fig)
        self.central_layout.addWidget(self.episode_render, 0, 0, Qt.AlignmentFlag.AlignTop)
        self.central_layout.setColumnMinimumWidth(0, 600)
        self.central_layout.setRowMinimumHeight(0, 550)
        self.episode_controls = QWidget()
        self.central_layout.addWidget(self.episode_controls, 1, 0, Qt.AlignmentFlag.AlignCenter)
        self.episode_controls_layout = QHBoxLayout(self.episode_controls)
        self.episode_controls.setSizePolicy(QSizePolicy.Policy.Fixed,
                                            QSizePolicy.Policy.Fixed)
        self.prev = QPushButton()
        self.prev.setStyleSheet("border-image: url(UI/images/prev.png); height: 30px; width: 30px")
        self.prev.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Fixed)
        self.episode_controls_layout.addWidget(self.prev)
        self.pause = QPushButton()
        self.pause.setStyleSheet("border-image: url(UI/images/play.png); height: 30px; width: 30px")
        self.episode_controls_layout.addWidget(self.pause)
        self.play = QPushButton()
        self.play.setStyleSheet("border-image: url(UI/images/pause.png); height: 30px; width: 30px")
        self.episode_controls_layout.addWidget(self.play)
        self.next = QPushButton()
        self.next.setStyleSheet("border-image: url(UI/images/next.png); height: 30px; width: 30px")
        self.episode_controls_layout.addWidget(self.next)

