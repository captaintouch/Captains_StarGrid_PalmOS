#!/bin/bash

rcpOutputFile="resources/graphicResources.rcp"
hOutputFile="sauce/graphicResources.h"
assetsPath="resources/assets/"
fileSuffix=".bmp"
fileHiResSuffix="-144.bmp"
hires=0
if [ "$1" == "--hires" ]; then
    hires=1
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
    if [ $hires == 1 ] && [ -f $hiresFile ]; then
        echo "	BITMAP \"$assetsPath$name$fileHiResSuffix\" BPP 8 TRANSPARENTINDEX 4 DENSITY 144 COMPRESS"  >> $rcpOutputFile
    else
        echo "	BITMAP \"$assetsPath$name$fileSuffix\" BPP 8 TRANSPARENTINDEX 4 COMPRESS"  >> $rcpOutputFile
        echo "	BITMAP \"$assetsPath$name$fileSuffix\" BPP 4 TRANSPARENTINDEX 4 COMPRESS"  >> $rcpOutputFile
    fi
    echo "END"  >> $rcpOutputFile

    # H FILE GENERATION
    echo "#define GFX_RES_${name^^} $identifier" >> $hOutputFile

done < "resources/graphicResources.map"

echo "#endif" >> $hOutputFile
