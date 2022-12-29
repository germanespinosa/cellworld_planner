import sys
from json_cpp import *
from cellworld import *
from src.video_writer import VideoWriter
from matplotlib import pyplot as plt
from matplotlib import gridspec
import cv2

fig = plt.figure(figsize=(6.3, 8))

spec = gridspec.GridSpec(ncols=1, nrows=2,
                         wspace=0,
                         hspace=0.01, height_ratios=[3, 1])

map_ax = fig.add_subplot(spec[0])
plot_ax = fig.add_subplot(spec[1])

w = World.get_from_parameters_names("hexagonal", "canonical", "21_05")

episode_video = None
fps = None
if len(sys.argv) > 2:
    episode_video = cv2.VideoCapture(sys.argv[2])
    fps = episode_video.get(cv2.CAP_PROP_FPS)
    if not episode_video.isOpened():
        print("can't open video file: ", sys.argv[2])
        episode_video = None

class Step_additional_info(JsonObject):
    def __init__(self):
        self.frame = 0
        self.time_stamp = 0.0
        self.belief_state = JsonList(list_type=float)
        self.mouse_visibility = JsonList(list_type=bool)
        self.mouse_exposure = JsonList(list_type=bool)
        self.weighted_visibility = 0.0
        self.weighted_exposure = 0.0
        self.itor = 0.0
        self.weighted_itor = 0.0
        self.robot_vertices = Location_list()
        self.robot_theta = 0.0
        self.mouse_head = Location()
        self.mouse_body = Location()
        self.mouse_theta = 0.0
        self.mouse_speed = 0.0
        self.mouse_acceleration = 0.0
        self.los = True
        JsonObject.__init__(self)


episode_number = int(sys.argv[1])
additional_data = JsonList(list_type=Step_additional_info)
additional_data.load_from_file("../results/episode_" + str(episode_number))

speed_filter_value = .9
acceleration_filter_value = .999
mouse_speed = 0
mouse_acceleration = 0
prev_speed = 0
a = False
l = False
peek_count = 0
bs_spread = []
free_cells = len(w.cells)

speed = [s.mouse_speed for s in additional_data]
avg_speed = [sum(speed[i-5 if i > -5 else 0:i+5 if i+5 < len(speed) else len(speed)-1])/((i+5 if i+5 < len(speed) else len(speed)-1) - (i-5 if i > -5 else 0)) for i in range(len(speed))]
avg_acceleration = [avg_speed[i] - avg_speed[i-1 if i > 0 else 0] for i in range(len(avg_speed))]

frame_numbers = [s.frame for s in additional_data]
witor = [s.weighted_itor if s.weighted_itor >= 0 else 0 for s in additional_data]
itor = [s.itor for s in additional_data]
time_stamps = [s.time_stamp for s in additional_data]
bs_spread = [sum([0 if h == 0 else 1 for h in s.belief_state]) / free_cells for s in additional_data]
los = [i for i, s in enumerate(additional_data) if s.los]


def get_peeks(values, time_stamps, threshold=5, period=.25):
    peek = []
    for i, v in enumerate(values):
        ii = i
        c = 0
        tt = time_stamps[i] + period
        while ii < len(values) and time_stamps[ii] <= tt:
            if values[ii] > .5:#more information is obtained than provided
                c += 1
            ii += 1
        if c >= threshold:
            peek.append(i)
    return peek


peeks = get_peeks(witor, time_stamps)

def heat_map(display, values, value_range: tuple = None):
    if value_range:
        minv, maxv = value_range
    else:
        minv, maxv = min(values), max(values)

    if minv == maxv:
        return

    adjusted_values = [(v-minv)/(maxv-minv) for v in values]
    for cell_id, color in enumerate(adjusted_values):
        cell = display.world.cells[cell_id]
        if display.world.cells[cell_id].occluded:
            display.ax.add_patch(
                RegularPolygon(
                    (cell.location.x, cell.location.y),
                    display.world.configuration.cell_shape.sides,
                    display.cells_size,
                    facecolor='black',
                    orientation=display.cells_theta,
                    linewidth=1)
            )
        else:
            display.ax.add_patch(
                RegularPolygon(
                    (cell.location.x, cell.location.y),
                    display.world.configuration.cell_shape.sides,
                    display.cells_size,
                    facecolor='r',
                    orientation=display.cells_theta,
                    linewidth=1,
                    alpha=color * .8)
            )


def get_peaks(values, band_ratio=.025):
    minimas = []
    maximas = []
    if len(values) > 0:
        lp = values[0]
        band_size = int(len(values) * band_ratio)
        last_peak_type = 0
        for i, v in enumerate(values):
            band_start = 0 if i < band_size else i - band_size
            band_finish = len(values)-1 if i >= len(values)-band_size else i + band_size
            band_min = min(values[band_start:band_finish])
            band_max = max(values[band_start:band_finish])
            if band_min == v and v < lp:
                if last_peak_type == -1:
                    minimas[-1] = i
                else:
                    minimas.append(i)
                last_peak_type = -1
                lp = v

            if band_max == v and v > lp:
                if last_peak_type == 1:
                    maximas[-1] = i
                else:
                    maximas.append(i)
                last_peak_type = 1
                lp = v
    return minimas, maximas


minimas, maximas = get_peaks(avg_speed)

lf = 0
for s in additional_data:
    if s.mouse_body.dist(Location(0,.5))>=.1:
        break
    for f in range(lf+1, s.frame):
        plot_ax.axvline(x=f, color='lightgray', zorder=-1)

for m in peeks:
    plot_ax.axvline(x=frame_numbers[m], color='lightgreen', zorder=-1)

plot_ax.plot(frame_numbers, avg_speed, zorder=10)
plot_ax.plot(frame_numbers, bs_spread, zorder=9)

plot_ax.scatter([frame_numbers[m] for m in minimas], [avg_speed[m] for m in minimas], marker="x", zorder=15)
plot_ax.scatter([frame_numbers[m] for m in maximas], [avg_speed[m] for m in maximas], marker="x", zorder=15)

plot_ax.scatter([frame_numbers[l] for l in los], [avg_speed[l] for l in los], s=3, color='red', zorder=11)

progress = plot_ax.axvline(x=0, color='black')

video = VideoWriter("../results/episode_" + str(episode_number) + ".mp4", fig.canvas.get_width_height(), fps)
last_frame = additional_data[0].frame
ret, video_frame = episode_video.read()
video_frame_margin = 0.045
from matplotlib.patches import Arc

for s in additional_data:
    first = True
    for f in range(last_frame, s.frame):
        ret, video_frame = episode_video.read()
        cropped_frame = video_frame[-980:, :, :]
        d = Display(w, fig=fig, ax=map_ax, animated=True)
        map_ax.imshow(cropped_frame, extent=[0, 1, video_frame_margin, 1 - video_frame_margin])
        a = Arc((0, .5), .1, .1, 0, 300, 60, color='purple', lw=1)
        map_ax.add_patch(a)

        heat_map(d, values=s.belief_state)
        if first:
            d.circle(location=s.mouse_head, color="b", radius=.003)
            d.arrow(beginning=s.mouse_head, theta=s.mouse_theta, color="g", dist=.05, head_width=.01, alpha=.5)
        progress.set_xdata(f)
        d.update()
        video.write_figure(fig, repeat=0)
        map_ax.cla()
        first = False
    last_frame = s.frame

video.close()
