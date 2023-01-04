from threading import Thread
import sys
from cellworld import *
from src.video_writer import VideoWriter
from matplotlib import pyplot as plt
from matplotlib.patches import Arc
from matplotlib import gridspec, patches
import cv2

episode_video = None
fps = None
cropped_frames = []
video_ready = False

def load_video(video_reader):
    global cropped_frames, video_ready
    while video_reader.isOpened():
        ret, video_frame = video_reader.read()
        if video_frame is None:
            break
        cropped_frames.append(video_frame[-980:, :, :])
    video_ready = True


if len(sys.argv) > 2:
    episode_video = cv2.VideoCapture(sys.argv[2])
    fps = episode_video.get(cv2.CAP_PROP_FPS)
    if not episode_video.isOpened():
        print("can't open video file: ", sys.argv[2])
        episode_video = None
    else:
        Thread(target=load_video, args=(episode_video,)).start()



fig = plt.figure(figsize=(6.3, 8))

spec = gridspec.GridSpec(ncols=1, nrows=2,
                         wspace=0,
                         hspace=0.01, height_ratios=[3, 1])

map_ax = fig.add_subplot(spec[0])
plot_ax = fig.add_subplot(spec[1])

w = World.get_from_parameters_names("hexagonal", "canonical", "21_05")

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


additional_data = JsonList(list_type=Step_additional_info)
additional_data.load_from_file(sys.argv[1])

mouse_speed = 0
prev_speed = 0
a = False
l = False
peek_count = 0
bs_spread = []
free_cells = len(w.cells)

speed = [s.mouse_speed for s in additional_data]
avg_speed = [sum(speed[i-5 if i > -5 else 0:i+5 if i+5 < len(speed) else len(speed)-1])/((i+5 if i+5 < len(speed) else len(speed)-1) - (i-5 if i > -5 else 0)) for i in range(len(speed))]
frame_numbers = [s.frame for s in additional_data]
total_frames = max(frame_numbers) + 1
witor = [s.weighted_itor if s.weighted_itor >= 0 else 0 for s in additional_data]
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
            if values[ii] > .5: #more information is obtained than provided
                c += 1
            ii += 1
        if c >= threshold:
            peek.append(i)
    return peek


def heat_map(ax, world, values, value_range: tuple = None, los: bool = False):
    if value_range:
        minv, maxv = value_range
    else:
        minv, maxv = min(values), max(values)

    if minv == maxv:
        return

    [p.remove() for p in reversed(ax.patches)]

    adjusted_values = [(v-minv)/(maxv-minv) for v in values]
    for cell_id, color in enumerate(adjusted_values):
        cell = world.cells[cell_id]
        if world.cells[cell_id].occluded:
            ax.add_patch(
                RegularPolygon(
                    (cell.location.x, cell.location.y),
                    world.configuration.cell_shape.sides,
                    world.implementation.cell_transformation.size / 2,
                    facecolor='black',
                    orientation=math.radians(0 - world.implementation.cell_transformation.rotation) + math.radians(0 - world.implementation.space.transformation.rotation),
                    linewidth=1)
            )
        else:
            if los:
                continue
            if not los and color > 0:
                ax.add_patch(
                    RegularPolygon(
                        (cell.location.x, cell.location.y),
                        world.configuration.cell_shape.sides,
                        world.implementation.cell_transformation.size / 2,
                        facecolor='r',
                        orientation=math.radians(0 - world.implementation.cell_transformation.rotation) + math.radians(0 - world.implementation.space.transformation.rotation),
                        linewidth=1,
                        alpha=color * .8)
                )


def get_peaks(values, time_stamps, band_size):
    minimas = []
    maximas = []
    if len(values) > 0:
        lpm = values[0]
        last_peak_type = 0
        for i, v in enumerate(values):
            band_start = i
            while band_start > 0 and time_stamps[band_start] > time_stamps[i]-band_size/2:
                band_start -= 1
            band_finish = i
            while band_finish < len(values) - 1 and time_stamps[band_finish] < time_stamps[i]+band_size/2:
                band_finish += 1
            band_min = min(values[band_start:band_finish])
            band_max = max(values[band_start:band_finish])
            if band_min == v and v < lpm:
                if last_peak_type == -1:
                    minimas[-1] = i
                else:
                    minimas.append(i)
                last_peak_type = -1
                lpm = v

            if band_max == v and v > lpm:
                if last_peak_type == 1:
                    maximas[-1] = i
                else:
                    maximas.append(i)
                last_peak_type = 1
                lpm = v
    return minimas, maximas


peeks = get_peeks(witor, time_stamps)
minimas, maximas = get_peaks(avg_speed, time_stamps, 1.0)

lf = 0
first_valid_frame = 0
first_valid_time = 0
for s in additional_data:
    if s.mouse_body.dist(Location(0, .5)) >= .1:
        break
    first_valid_frame = s.frame
    first_valid_time = s.time_stamp

adjusted_time_stamp = [ts - first_valid_time for ts in time_stamps]

plot_ax.add_patch(patches.Rectangle((0, 0), first_valid_frame, max(avg_speed), facecolor="lightgray", zorder=-1))
frame_speed = [0 if i not in frame_numbers else avg_speed[frame_numbers.index(i)] for i in range(total_frames)]
frame_peeks = [frame_numbers[m] for m in peeks if frame_numbers[m] >= first_valid_frame]
frame_minimas = [frame_numbers[m] for m in minimas if frame_numbers[m] >= first_valid_frame]
frame_maximas = [frame_numbers[m] for m in maximas if frame_numbers[m] >= first_valid_frame]
frame_los = [frame_numbers[m] for m in los if frame_numbers[m] >= first_valid_frame]

time_speed = [0 if i not in time_stamps else avg_speed[time_stamps.index(i)] for i in range(total_frames)]
time_peeks = [time_stamps[m] for m in peeks if time_stamps[m] >= first_valid_time]
time_minimas = [time_stamps[m] for m in minimas if time_stamps[m] >= first_valid_time]
time_maximas = [time_stamps[m] for m in maximas if time_stamps[m] >= first_valid_time]
time_los = [time_stamps[m] for m in los if time_stamps[m] >= first_valid_time]


a_s = 0
i = 0
mid_points = []
for ma in frame_maximas:
    cmi = 0
    for mi in frame_minimas:
        if mi < ma:
            cmi = mi
        else:
            break
    mid_points.append(int((cmi+ma)/2))


cycles = [[time_stamps[0]]]
cycles_behavior = [0]
cycles_stop = [False]
cycles_peeks = [0]
for n, i in enumerate(frame_numbers):
    if i < first_valid_frame:
        continue
    if cycles_behavior[-1] == 0:
        if i + 5 in frame_los:
            cycles_behavior[-1] = 2
        else:
            if i in frame_maximas:
                cycles_behavior[-1] = 1
    if i in frame_minimas:
        if cycles_peeks[-1] == 0:
            cycles_peeks[-1] == -1

        if frame_speed[i] < .3:
            cycles_stop[-1] = True

    if cycles_peeks[-1] >= 0:
        if i in frame_peeks:
            cycles_peeks[-1] += 1

    if i in mid_points:
        cycles_behavior.append(0)
        cycles_stop.append(False)
        cycles_peeks.append(False)
        cycles[-1].append(time_stamps[n])
        cycles.append([time_stamps[n]])

for i, c in enumerate(cycles):
    if cycles_behavior[i] == 1:
        if cycles_stop[i]:
            plot_ax.add_patch(patches.Rectangle((c[0], -0.05), c[-1], max(avg_speed) + .1, facecolor=(.5, 1.0, .5), zorder=-1))
        else:
            plot_ax.add_patch(patches.Rectangle((c[0], -0.05), c[-1], max(avg_speed) + .1, facecolor=(.8, 1.0, .8), zorder=-1))
    else:
        plot_ax.add_patch(patches.Rectangle((c[0], -0.05), c[-1], max(avg_speed) + .1, facecolor="lightgray", zorder=-1))
    plot_ax.axvline(x=c[0], color='blue')

plot_ax.plot(time_stamps, avg_speed, zorder=10)
plot_ax.plot(time_stamps, bs_spread, zorder=9)
plot_ax.scatter(time_minimas, [frame_speed[m] for m in frame_minimas], color='orange', marker="x", zorder=15)
plot_ax.scatter(time_maximas, [frame_speed[m] for m in frame_maximas], color='orange', marker="x", zorder=15)
plot_ax.scatter(time_los, [frame_speed[m] for m in frame_los], marker=".", color='red', zorder=11, alpha=1)
plot_ax.scatter(time_peeks, [frame_speed[m] for m in frame_peeks], marker=".", color='purple', zorder=11, alpha=1)

progress = plot_ax.axvline(x=0, color='black')

video = VideoWriter(sys.argv[3], fig.canvas.get_width_height(), fps)
last_frame = additional_data[0].frame
ret, video_frame = episode_video.read()
video_frame_margin = 0.045

while not video_ready:
    pass


map_ax.axes.xaxis.set_visible(False)
map_ax.axes.yaxis.set_visible(False)

# d = Display(w, fig=fig, ax=map_ax, animated=True)
arco = Arc((0, .5), .1, .1, 0, 300, 60, color='purple', lw=1)

last_frame = additional_data[0].frame
last_time_stamp = additional_data[0].time_stamp

for i, s in enumerate(additional_data):
    first = True
    for f in range(last_frame, s.frame):
        current_time_stamp = last_time_stamp + (s.time_stamp - last_time_stamp) / (s.frame - last_frame) * (f - last_frame)
        t = Timer()
        if f < first_valid_frame:
            continue
        map_ax.imshow(cropped_frames[f], extent=[0, 1, video_frame_margin, 1 - video_frame_margin])
        map_ax.text(.03, .9, "%05d" % (f,), color="white")
        heat_map(map_ax, w, values=s.belief_state, los=f in frame_los)
        if first:
            map_ax.add_patch(plt.Circle((s.mouse_head.x, s.mouse_head.y), .003, color="b", alpha=1))
            arrow_head = Location(0, 0).move(theta=s.mouse_theta, dist=.05)
            map_ax.arrow(s.mouse_head.x,
                         s.mouse_head.y,
                         arrow_head.x,
                         arrow_head.y,
                         color="g",
                         head_width=.01,
                         length_includes_head=True,
                         alpha=1)
        progress.set_xdata(current_time_stamp)
        map_ax.set_xlim(xmin=0, xmax=1)
        map_ax.set_ylim(ymin=0.05, ymax=.95)
        plot_ax.set_xlim(xmin=first_valid_time, xmax=max(time_stamps))
        plot_ax.set_ylim(ymin=-0.05, ymax=max(avg_speed) + .05)
        map_ax.add_patch(arco)
        fig.canvas.draw()
        video.write_figure(fig, repeat=0)
        map_ax.cla()
        first = False
        t1 = t.to_seconds()
        print("%i: %f" % (f, t1))
    last_frame = s.frame
    last_time_stamp = s.time_stamp

video.close()
