#!/bin/bash
ffmpeg -framerate 40 -i "%05d.ppm" -vf "format=yuv420p,scale=720:720,vflip" -c:v libx264 clip.mp4
