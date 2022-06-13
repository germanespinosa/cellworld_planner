from time import sleep
from src import *
from cellworld import *
from matplotlib import pyplot as plt
from os import mkdir
import glob
from moviepy.editor import *
from threading import Thread
from moviepy.video.io.bindings import mplfig_to_npimage
import sys

simulation = Simulation.load_from_file(sys.argv[1])
videos_folder = None
if len(sys.argv) > 2:
    videos_folder = sys.argv[2]

if videos_folder:
    mkdir(videos_folder)
    os.chdir(videos_folder)


def make_video(frame_files_path, video_file):
    if frame_files_path == ".":
        return
    base_dir = os.path.realpath(frame_files_path)
    print("working on " + base_dir)
    fps = 20
    file_list = glob.glob(base_dir + '/*.png')  # Get all the pngs in the current directory
    clips = [ImageClip(m).set_duration(.05)
             for m in file_list]
    concat_clip = concatenate_videoclips(clips, method="compose")
    concat_clip.write_videofile(video_file, fps=fps)


w = World.get_from_parameters_names(simulation.world_info.world_configuration, simulation.world_info.world_implementation, simulation.world_info.occlusions)
d = Display(w, animated=True, fig_size=(6, 5))
prey_cell = -1
predator_cell = -1

value_function = []

for prey_history in simulation.prey_data:
    value_function.append([max(x.options_values) for x in prey_history])

t = None

for i in range(simulation.prey_data):
    clips = []
    if videos_folder:
        episode_folder = "./episode_%04d" % (i,)
        mkdir(episode_folder)
#    plt.plot(value_function[i])
    for step in episode.trajectories:
        for prey_state in simulation.prey_data[i]:
            if step.frame == prey_state.frame:
#                print(max(prey_state.options_values))
                upper_limit = max(prey_state.belief_state)
                if upper_limit > 0:
                    cmap = plt.cm.Reds([x / upper_limit for x in prey_state.belief_state])
                    for cell_id, c in enumerate(cmap):
                        if not w.cells[cell_id].occluded and cell_id != predator_cell:
                            d.cell(cell_id=cell_id, color=c)

        if step.agent_name == "prey":
            if prey_cell >= 0:
                d.cell(cell_id=prey_cell, color="white")
            prey_cell = w.cells.find(step.location)
            d.cell(cell_id=prey_cell, color="green")
        else:
            if predator_cell >= 0:
                d.cell(cell_id=predator_cell, color="white")
            predator_cell = w.cells.find(step.location)
            d.cell(cell_id=predator_cell, color="blue")
        d.update()
        if videos_folder:
            d.fig.savefig(episode_folder + "/frame_" + "%04d" % (step.frame,) + ".png")
    if videos_folder:
        if t:
            t.join()
        t = Thread(target=make_video, args=(episode_folder, episode_folder + ".mp4"))
        t.start()
    # break
