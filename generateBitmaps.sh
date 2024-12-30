#!/bin/bash
if ! command -v convert &> /dev/null; then
    echo "Could not find ImageMagick, aborting..."
    exit 1
fi

if [ ! -d ./resources/assets ]; then
    mkdir ./resources/assets
fi

# LOW RES
for image in ./resources/hires/*.png; do
    filename=$(basename "$image")
    name="${filename%.*}"
    outputFile="./resources/assets/$name.bmp"
    convert "$image" -channel alpha -threshold 20% +channel -background '#ff33ff' -alpha remove -filter Point -resize 50% "BMP3:$outputFile"
done

# HI RES
for image in ./resources/hires/*.png; do
    filename=$(basename "$image")
    name="${filename%.*}"
    outputFile="./resources/assets/$name-144.bmp"
    convert "$image" -channel alpha -threshold 20% +channel -background '#ff33ff' -alpha remove "BMP3:$outputFile"
done
