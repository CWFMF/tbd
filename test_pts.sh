#!/bin/bash
set -e
# docker compose exec tbd_dev_svc /bin/bash -c './test_hull.sh > hull_testing.log 2>&1'
sed -i "s/Settings::setSavePoints(false);/Settings::setSavePoints(true);/g" tbd/src/cpp/Test.cpp
docker compose exec tbd_dev_svc /bin/bash -c './test_hull.sh'
python pts_to_shp.py
# mkdir -p /mnt/d/firestarr_pts
# rsync -avHP --delete ../data/test_output/ /mnt/d/firestarr_pts/
sed -i "s/Settings::setSavePoints(true);/Settings::setSavePoints(false);/g" tbd/src/cpp/Test.cpp
