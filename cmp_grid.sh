#!/bin/bash

fire="11N_54669"
# file="probability_215_2023-08-03.tif"
# file="007_000001_215_arrival.tif"
file="020_000030_215_.tif"

docker run -it -v /mnt/data/data/tmp/:/home/out \
    ghcr.io/osgeo/gdal:ubuntu-full-latest gdalcompare.py -skip_binary \
    /home/out/${fire}/${file} \
    /home/out/${fire}.orig/${file}
