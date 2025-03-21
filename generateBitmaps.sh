#!/bin/bash
if ! command -v convert &> /dev/null; then
    echo "Could not find ImageMagick, aborting..."
    exit 1
fi

if [ ! -d ./resources/assets ]; then
    mkdir ./resources/assets
fi

if [ ! -d ./resources/hiresTMP ]; then
    mkdir ./resources/hiresTMP
fi

function rotate() {
    sourceImage=$1
    degrees=$2
    outputFile=$3

    originalWidth=$(identify -format "%w" "$sourceImage")
    originalHeight=$(identify -format "%h" "$sourceImage")
    convert "$sourceImage" -channel alpha -background transparent -rotate $degrees -gravity center -extent "${originalWidth}x${originalHeight}" "$outputFile"
}

# Add rotated images for each ship
for image in ./resources/ships/*.png; do
    filename=$(basename "$image")
    name="${filename%.*}"
    cp $image ./resources/hiresTMP/${name}_0.png
    for i in {1..7}; do
        rotate "$image" $((i * 45)) "./resources/hiresTMP/${name}_${i}.png"
    done
done

# Add other images (flags, etc)
for image in ./resources/others/*.png; do
    cp $image ./resources/hiresTMP/
done

# LOW RES
for image in ./resources/hiresTMP/*.png; do
    filename=$(basename "$image")
    name="${filename%.*}"
    outputFile="./resources/assets/$name.bmp"
    convert "$image" -channel alpha -threshold 20% +channel -background '#ff33ff' -alpha remove -filter Point -resize 50% "BMP3:$outputFile"
done

# HI RES
for image in ./resources/hiresTMP/*.png; do
    filename=$(basename "$image")
    name="${filename%.*}"
    outputFile="./resources/assets/$name-144.bmp"
    convert "$image" -channel alpha -threshold 20% +channel -background '#ff33ff' -alpha remove "BMP3:$outputFile"
done

# Add presized images (some icons)
for image in ./resources/others_noresize/*.png; do
    filename=$(basename "$image")
    name="${filename%.*}"
    outputFile="./resources/assets/$name.bmp"
    convert "$image" -channel alpha -threshold 20% +channel -background '#ff33ff' -alpha remove "BMP3:$outputFile"
done