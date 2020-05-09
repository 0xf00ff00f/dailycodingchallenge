#!/bin/bash
ffmpeg -framerate 25 -i "%05d.ppm" -vf "scale=400:400,vflip" clip.gif
# ffmpeg -framerate 40 -i "%05d.ppm" -vf "scale=400:400,vflip" clip.gif
# ffmpeg -framerate 40 -i "%05d.ppm" -vf "vflip" clip.gif
