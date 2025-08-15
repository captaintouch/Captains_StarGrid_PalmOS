#!/bin/bash

rcpOutputFile="resources/graphicResources.rcp"
hOutputFile="sauce/graphicResources.h"
assetsPath="resources/assets/"
fileSuffix=".bmp"
fileHiResSuffix="-144.bmp"
fileBWSuffix="_bw.bmp"
hires=0
if [ "$1" == "--hires" ]; then
    hires=1
    echo "Hires resources enabled"
fi

if [ -f $rcpOutputFile ]; then
    rm $rcpOutputFile
fi

echo "#ifndef GRAPHICRESOURCES_H_" > $hOutputFile
echo "#define GRAPHICRESOURCES_H_" >> $hOutputFile

while IFS=';' read -r identifier name
do
    # RCP FILE GENERATION
    echo "BITMAPFAMILY ID $identifier" >> $rcpOutputFile
    echo "BEGIN" >> $rcpOutputFile
    hiresFile=$assetsPath$name$fileHiResSuffix
    bwFile=$assetsPath$name$fileBWSuffix
    if [ $hires == 1 ] && [ -f $hiresFile ]; then
        echo "	BITMAP \"$assetsPath$name$fileHiResSuffix\" BPP 8 TRANSPARENTINDEX 4 DENSITY 144 COMPRESS"  >> $rcpOutputFile
    else
        echo "	BITMAP \"$assetsPath$name$fileSuffix\" BPP 8 TRANSPARENTINDEX 4 COMPRESS"  >> $rcpOutputFile
	if [ -f $bwFile ]; then
		echo "	BITMAP \"$assetsPath$name$fileBWSuffix\" BPP 4 TRANSPARENTINDEX 4 COMPRESS"  >> $rcpOutputFile
	fi
    fi
    echo "END"  >> $rcpOutputFile

    # H FILE GENERATION
    echo "#define GFX_RES_${name^^} $identifier" >> $hOutputFile

done < "resources/graphicResources.map"

# Find total number of frames for animation sets:
results=$(awk -F';' '{print $2}' "resources/graphicResources.map" | grep _ | awk '{gsub("_", ";"); print}' | sort -u | awk -F";" '{if($2 > max[$1]) max[$1] = $2} END {for (i in max) print i ";" max[i]}')
while IFS=';' read -r name maxCount
do
	((maxCount++))
	echo "#define GFX_FRAMECOUNT_${name^^} $maxCount" >> $hOutputFile
done <<< "$results"

echo "#endif" >> $hOutputFile
