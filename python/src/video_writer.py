from moviepy.video.io.ffmpeg_writer import *
from matplotlib import figure
import numpy as np
from threading import Thread

class VideoWriter:
    def __init__(self,
                 filename,
                 size,
                 fps,
                 codec="libx264",
                 audiofile=None,
                 preset="medium",
                 bitrate=None,
                 withmask=False,
                 logfile=None,
                 threads=None,
                 ffmpeg_params=None):
        self.writer = FFMPEG_VideoWriter(
            filename,
            size,
            fps,
            codec=codec,
            preset=preset,
            bitrate=bitrate,
            logfile=logfile,
            audiofile=audiofile,
            threads=threads,
            withmask=withmask,
            ffmpeg_params=ffmpeg_params)
        self.cols, self.rows = size
        self.fps = fps
        self.ready = True


    def __write_buffer__(self, buffer, repeat=None, duration=None):
        npa = np.frombuffer(buffer, dtype=np.uint8).reshape(self.rows, self.cols, 3)
        self.write(npa, repeat=repeat, duration=duration)
        self.ready = True


    def write_figure(self, fig: figure, repeat=None, duration=None):
        buffer = fig.canvas.tostring_rgb()
        while not self.ready:
            pass
        self.ready = False
        Thread(target=self.__write_buffer__, args=(buffer, repeat, duration)).start()
        self.__write_buffer__(buffer, repeat, duration)

    def write(self, frame, repeat=0, duration=None):
        if duration:
            repeat = int(duration * self.fps)
        if repeat == 0:
            repeat = 1
        for i in range(repeat):
            self.writer.write_frame(frame)

    def close(self):
        self.writer.close()
