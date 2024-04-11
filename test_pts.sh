#!/bin/bash
set -e
# docker compose exec tbd_dev_svc /bin/bash -c './test_hull.sh > hull_testing.log 2>&1'
docker compose exec tbd_dev_svc /bin/bash -c './test_hull.sh'
python pts_to_shp.py
# mkdir -p /mnt/d/firestarr_pts
# rsync -avHP --delete ../data/test_output/ /mnt/d/firestarr_pts/
