#!/bin/bash
ffmpeg -framerate 40 -i "%05d.ppm" -vf "scale=720:720,vflip" -c:v libx264 -vf format=yuv420p clip.mp4
