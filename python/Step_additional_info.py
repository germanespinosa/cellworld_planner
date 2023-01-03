from threading import Thread
import sys
from json_cpp import *
from cellworld import *
from src.video_writer import VideoWriter
from matplotlib import pyplot as plt
from matplotlib import gridspec
from matplotlib import patches
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
total_frames = max(frame_numbers) + 1
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


def get_peaks(values, band_ratio=.05):
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


peeks = get_peeks(witor, time_stamps)
minimas, maximas = get_peaks(avg_speed)

lf = 0
first_valid_frame = 0
for s in additional_data:
    if s.mouse_body.dist(Location(0, .5)) >= .1:
        break
    first_valid_frame = s.frame

plot_ax.add_patch(patches.Rectangle((0, 0), first_valid_frame, max(avg_speed), facecolor="lightgray", zorder=-1))

frame_speed = [0 if i not in frame_numbers else avg_speed[frame_numbers.index(i)] for i in range(total_frames)]
frame_peeks = [frame_numbers[m] for m in peeks if frame_numbers[m] >= first_valid_frame]
frame_minimas = [frame_numbers[m] for m in minimas if frame_numbers[m] >= first_valid_frame]
frame_maximas = [frame_numbers[m] for m in maximas if frame_numbers[m] >= first_valid_frame]
frame_los = [frame_numbers[m] for m in los if frame_numbers[m] >= first_valid_frame]
acceleration_sign = [1 if f in frame_minimas else -1 if f in frame_maximas else 0 for f in range(total_frames)]

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


cycles = [[]]
cycles_behavior = [0]
cycles_stop = [False]
cycles_peeks = [0]
for i in frame_numbers:
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
        cycles[-1].append(i)
        cycles.append([])
    cycles[-1].append(i)

print(cycles_behavior)
print(cycles_stop)

behavior = [0 for f in range(total_frames)]

predictive = 1
for i, a in enumerate(acceleration_sign):
    if i in frame_los:
        if a == 1:
            predictive = 0
    if i in frame_minimas:
        predictive = 1
    behavior[i] = predictive

frame_predictive_peeks = []
frame_reactive_peeks = []
for i in frame_peeks:
    if behavior[i] == 1:
        frame_predictive_peeks.append(i)
    else:
        frame_reactive_peeks.append(i)

frame_predictive_los = []
frame_reactive_los = []
for i in frame_los:
    if i < first_valid_frame:
        continue
    if behavior[i] == 1:
        frame_predictive_los.append(i)
    else:
        frame_reactive_los.append(i)


peeks_video = VideoWriter("../results/episode_" + str(episode_number) + "_peeks.mp4", fig.canvas.get_width_height(), fps)

for i, c in enumerate(cycles):
    if cycles_behavior[i] == 1:
        plot_ax.add_patch(patches.Rectangle((c[0], 0), c[-1], max(avg_speed), facecolor="lightgreen", zorder=-1))
    else:
        plot_ax.add_patch(patches.Rectangle((c[0], 0), c[-1], max(avg_speed), facecolor="lightblue", zorder=-1))
    plot_ax.axvline(x=c[0], color='blue')


plot_ax.plot(frame_numbers, avg_speed, zorder=10)

plot_ax.plot(frame_numbers, bs_spread, zorder=9)
# plot_ax.plot(behavior, zorder=10)
plot_ax.scatter(frame_minimas, [frame_speed[m] for m in frame_minimas], marker="x", zorder=15)
plot_ax.scatter(frame_maximas, [frame_speed[m] for m in frame_maximas], marker="x", zorder=15)
plot_ax.scatter(frame_predictive_los, [frame_speed[m] for m in frame_predictive_los], marker=".", color='red', zorder=11, alpha=1)
plot_ax.scatter(frame_reactive_los, [frame_speed[m] for m in frame_reactive_los], marker="x", color='red', zorder=11, alpha=.5)
plot_ax.scatter(frame_predictive_peeks, [frame_speed[m] for m in frame_predictive_peeks], marker=".", color='purple', zorder=11, alpha=1)
plot_ax.scatter(frame_reactive_peeks, [frame_speed[m] for m in frame_reactive_peeks], marker="x", color='purple', zorder=11, alpha=.5)

progress = plot_ax.axvline(x=0, color='black')

video = VideoWriter("../results/episode_" + str(episode_number) + ".mp4", fig.canvas.get_width_height(), fps)
last_frame = additional_data[0].frame
ret, video_frame = episode_video.read()
video_frame_margin = 0.045
from matplotlib.patches import Arc

while not video_ready:
    pass


map_ax.axes.xaxis.set_visible(False)
map_ax.axes.yaxis.set_visible(False)

# d = Display(w, fig=fig, ax=map_ax, animated=True)
arco = Arc((0, .5), .1, .1, 0, 300, 60, color='purple', lw=1)

for s in additional_data:
    first = True
    for f in range(last_frame, s.frame):
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
        progress.set_xdata(f)
        map_ax.set_xlim(xmin=0, xmax=1)
        map_ax.set_ylim(ymin=0.05, ymax=.95)
        plot_ax.set_xlim(xmin=first_valid_frame, xmax=total_frames)
        plot_ax.set_ylim(ymin=0, ymax=max(avg_speed))
        map_ax.add_patch(arco)
        fig.canvas.draw()
        video.write_figure(fig, repeat=0)
        map_ax.cla()
        first = False
        t1 = t.to_seconds()
        print(t1, f)
    last_frame = s.frame

video.close()
