#!/bin/sh
openscad --camera=-44.29,-45.99,-42.40,16,29,8.4,27 ./logo.scad -o tmp.png
convert tmp.png -fuzz 10% -transparent white logo512.png
rm -f tmp.png

convert logo512.png -resize 256x256 logo256.png
convert logo512.png -resize 128x128 logo128.png
convert logo512.png -resize 64x64 logo64.icon
convert logo512.png -resize 32x32 logo32.icon
