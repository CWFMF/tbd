import math
import os
import re
import shutil

import geopandas as gpd
import pandas as pd

CRS_WGS84 = 4326

DIR = "../data/test_output"
DIR_SIMS = f"{DIR}/sims"
DIR_OUT = f"{DIR}/gis"

os.makedirs(DIR_OUT, exist_ok=True)


def to_gdf(df, crs=CRS_WGS84):
    geometry = df["geometry"] if "geometry" in df else gpd.points_from_xy(df["x"], df["y"], crs=crs)
    return gpd.GeoDataFrame(df, crs=crs, geometry=geometry)


STAGES = {"N": 0, "S": 1, "C": 2}


def find_groups(s):
    g = re.fullmatch("(\d*)(.)(\d)*", s).groups()
    return [int(g[0]), STAGES[g[1]], int(g[2])]


def find_id(s):
    scenario, stage, step = find_groups(s)
    return step * 10 + stage


def to_shp(file, force=False, dir_to=DIR_OUT):
    d = os.path.dirname(file)
    name = os.path.basename(d)
    save_to = f"{dir_to}/{name}.shp"
    if not (force or not os.path.exists(save_to)):
        return
    df = pd.read_csv(file)
    print(f"Converting {len(df)} points from {file}")
    # df[["scenario", "stage", "step"]] = list(df["step_id"].apply(find_groups))
    df["id"] = df["step_id"].apply(find_id)
    gdf = to_gdf(df, crs=None)
    # gdf["id"] = gdf.apply(lambda x: x["step"] * 10 + x["stage"], axis=1)
    # del gdf["scenario"]
    del gdf["step_id"]
    del gdf["x"]
    del gdf["y"]
    gdf.to_file(
        save_to,
        # schema=schema,
        layer=name,
    )


def to_gpkg(file, force=False, dir_to=DIR_OUT):
    d = os.path.dirname(file)
    name = os.path.basename(d)
    save_to = f"{dir_to}/{name}.gpkg"
    if not (force or not os.path.exists(save_to)):
        return
    df = pd.read_csv(file)
    print(f"Converting {len(df)} points from {file}")
    df[["scenario", "stage", "step"]] = list(df["step_id"].apply(find_groups))
    gdf = to_gdf(df, crs=None)
    del gdf["step_id"]
    del gdf["x"]
    del gdf["y"]
    gdf.to_file(
        save_to,
        driver="GPKG",
        layer=name,
    )
    shutil.make_archive(save_to, "zip", dir_to, os.path.basename(save_to))


def to_parquet(file, force=False, dir_to=DIR_OUT):
    d = os.path.dirname(file)
    name = os.path.basename(d)
    save_to = f"{dir_to}/{name}.parquet"
    if not (force or not os.path.exists(save_to)):
        df = gpd.read_parquet(save_to)
        print(f"Already converted {len(df)} points from {name}")
        return
    df = pd.read_csv(file)
    print(f"Converting {len(df)} points from {name}")
    df[["scenario", "stage", "step"]] = list(df["step_id"].apply(find_groups))
    # df = df.loc[(df["step"] < 2) | (df["step"] == df["step"].max())]
    df = df.loc[(df["step"] < 1)]
    gdf = to_gdf(df, crs=None)
    del gdf["step_id"]
    del gdf["x"]
    del gdf["y"]
    gdf.to_parquet(save_to, index=False)


angles = []


def add_offsets_calc_ros(theta):
    d = math.degrees(theta)
    print(d)
    angles.append(d)
    return True


def real_angle(l_b, rads):
    # return rads
    rx = 1.0
    ry = l_b
    x = math.cos(rads)
    y = math.sin(rads)
    aa = rx * rx
    bb = ry * ry
    t = aa * bb / ((x * x * bb) + (y * y * aa))
    x *= t
    y *= t
    y *= rx / ry
    return math.atan2(y, x)


# angles = []
# # head_ros
# l_b = 5.0
# theta = 0
# cur_x = 0
# step_x = 0.1
# added = True
# while added and cur_x < 1:
#     theta = abs(math.acos(cur_x) - math.radians(90))
#     added = add_offsets_calc_ros(real_angle(l_b, theta))
#     # added = add_offsets_calc_ros(theta)
#     cur_x += step_x

# if added:
#     # added = add_offsets(math.radians(90), flank_ros * math.sqrt(a_sq_sub_c_sq) / a)
#     added = add_offsets_calc_ros(math.radians(90))
#     cur_x -= step_x

# while added and cur_x > 0:
#     # theta = math.acos(1.0 - cur_x)
#     theta = math.radians(90) + math.acos(cur_x)
#     added = add_offsets_calc_ros(real_angle(l_b, theta))
#     # added = add_offsets_calc_ros(theta)
#     cur_x -= step_x

# if added:
#     add_offsets_calc_ros(math.radians(180))

# angles = [180 - x for x in angles]


to_convert = []
for root, dirs, files in os.walk(DIR_SIMS):
    for f in files:
        if "_points.txt" in f:
            to_convert.append(os.path.join(root, f))

to_convert = sorted(to_convert)
for f in to_convert:
    to_parquet(f)
