#!/bin/bash

# docker volume create fireguard_data
# docker network create fireguard-network

pushd docker/fireguard
# doesn't build anything until it runs since just using base images
docker compose up -d
# FIX:
# docker exec -it fireguard-db /bin/bash
# copy and paste WeatherSHIELD/db/postgre.sql for now
popd

pushd docker/gui
# doesn't build anything until it runs since just using base images
docker compose up -d
popd

# docker run --name fireguard-postgis  --ip 172.18.0.2 -v fireguard_data:/var/lib/postgresql --network fireguard-network -e POSTGRES_PASSWORD="p455w0rd!" -d postgis/postgis
# docker run --name fireguard-cli -it --rm -v fireguard_data:/var/lib/postgresql --network fireguard-network thinkwhere/gdal-python python

# docker exec -it fireguard_db_1 psql -U docker -h localhost -d gis -p 5432
# docker exec -it fireguard-db psql -U docker -h localhost -d gis -p 5432

# # build from main directory
# docker build --tag fireguard-python -f docker/python/Dockerfile . && docker run -it --rm --name fireguard-cli -v C:\FireGUARD\data:/FireGUARD/data --network fireguard-network fireguard-python /bin/bash

# docker run -it --rm -v C:\FireGUARD\data:/FireGUARD/data --network fireguard-network fireguard-python /bin/bash

# # build php docker
# docker stop wxshield-gui & docker build --tag wxshield -f docker/gui/Dockerfile . && docker run -d --rm --ip 172.18.0.5 -p 8080:80 --network fireguard-network --name wxshield-gui wxshield
