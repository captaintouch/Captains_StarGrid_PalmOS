#!/bin/bash

while getopts "o:" opt; do
  case $opt in
    o) OUTPUT_FILE="$OPTARG" ;;
    *) echo "Usage: $0 [-o output.png] HEXTILE_SIZE START_COLOR END_COLOR" >&2; exit 1 ;;
  esac
done
shift $((OPTIND - 1))

if [ "$#" -ne 3 ]; then
  echo "Usage: $0 [-o output.png] HEXTILE_SIZE START_COLOR END_COLOR" >&2
  exit 1
fi

HEXTILE_SIZE=$1
START_COLOR=$2
#END_COLOR=$3
END_COLOR=$2
OUTPUT_FILE=${OUTPUT_FILE:-hex_tile.png}

TILE_SIZE=$HEXTILE_SIZE
SEGMENT_SIZE=$((TILE_SIZE / 4))
WIDTH=$TILE_SIZE
HEIGHT=$TILE_SIZE

GRADIENT=gradient.png
MASK=mask.png

convert -size ${WIDTH}x${HEIGHT} gradient:"$START_COLOR"-"$END_COLOR" "$GRADIENT"

convert -size ${WIDTH}x${HEIGHT} xc:black -fill white \
  -draw "polygon \
    $((WIDTH / 2)),0 \
    0,$SEGMENT_SIZE \
    0,$((TILE_SIZE - SEGMENT_SIZE)) \
    $((WIDTH / 2)),$TILE_SIZE \
    $WIDTH,$((TILE_SIZE - SEGMENT_SIZE)) \
    $WIDTH,$SEGMENT_SIZE" \
  -monochrome "$MASK"

convert "$GRADIENT" "$MASK" -alpha Off -compose CopyOpacity -composite "$OUTPUT_FILE"

rm "$GRADIENT" "$MASK"

