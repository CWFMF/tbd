import sys
sys.path.append('../util')
import common
import json
import logging
import sys
import pandas as pd
import math
import datetime
import shlex
import timeit
import subprocess
from osgeo import ogr
from osgeo import osr
import statistics
import fiona
from shapely.geometry import Polygon, mapping
import os
import firestarr_gis

startup = {
            'ffmc':          {'value': 85.0},
            'dmc':           {'value': 6.0},
            'dc':            {'value': 15.0},
            'precipitation': {'value': 0.0},
          }

fire_name = "FIRE"
out_dir = os.path.join("./Data", fire_name)
common.ensure_dir(out_dir)

def unnest_values(dict):
    for i in dict:
        dict[i] = dict[i]['value']
    return dict

def try_read_first(dict, key, fail_msg=None, is_fatal=False):
    result = dict[key]
    n = len(result)
    if n < 1:
        msg = "No {} provided".format(key)
        if fail_msg is not None:
            msg = "{} - {}".format(msg, fail_msg)
        if is_fatal:
            logging.fatal(msg)
            sys.exit(-1)
        logging.warning(msg)
        return None
    else:
        if len(result) > 1:
            logging.warning("{} {} provided - only the first one will be used".format(n, key))
    return result[0]

with open(sys.argv[1]) as f:
  data = json.load(f)

MSG_DEFAULT_STARTUP = 'using default startup indices'
project = data['project']
stn = try_read_first(project['stations'], 'stations', MSG_DEFAULT_STARTUP)
if stn is not None:
    stream = try_read_first(stn['station'], 'streams', MSG_DEFAULT_STARTUP)
    if stream is not None:
        startup = stream['condition']['startingCodes']
unnest_values(startup)
logging.info("Startup indices are: {}".format(startup))
ffmc = startup['ffmc']
dmc = startup['dmc']
dc = startup['dc']
apcp_0800 = startup['precipitation']

pt = None
ignition = try_read_first(project['ignitions'], 'ignitions', is_fatal=True)
ign = try_read_first(ignition['ignition']['ignitions'], 'ignitions', is_fatal=True)

perim = None
poly = ign['polygon']
if poly['units'] != 'LAT_LON':
    logging.fatal("Only lat/long coordinates are currently supported")
    sys.exit(-1)
if ign['polyType'] != 'POINT':
    logging.fatal("Only point ignition is currently supported")
    if ign['polyType'] == 'POLYGON_OUT':
        pts = poly['polygon']['points']
        pts = list(map(unnest_values, pts))
        pts = [list(map(lambda v: [v['x'], v['y']], pts))]
        lat = statistics.mean(list(map(lambda v: v[1], pts[0])))
        long = statistics.mean(list(map(lambda v: v[0], pts[0])))
        print(long)
        orig_zone = 15
        orig_long = -93
        diff = long - orig_long
        print(diff)
        ZONE_SIZE = 6
        zone_diff = round(diff / ZONE_SIZE)
        print(zone_diff)
        meridian = orig_long + (zone_diff * ZONE_SIZE)
        print(meridian)
        zone = orig_zone + zone_diff
        # print(pts)
        p = '''{"type": "Polygon",
                "coordinates": ''' + str(pts) + ''',
            }'''
        # print(p)
        g = ogr.CreateGeometryFromJson(p)
        # print(g)
        # print("Hi! I'm a %s with an Area  %s" % (g.GetGeometryName(), g.Area()))
        # print("I have inside me %s feature(s)!\n" % g.GetGeometryCount())
        # for idx, f in enumerate(g):
            # print("I'm feature n.%s and I am a %s.\t I have an Area of %s - You can get my json repr with f.ExportToJson()" % (idx, f.GetGeometryName(),f.Area()))
        source = osr.SpatialReference()
        source.ImportFromEPSG(4269)
        target = osr.SpatialReference()
        target.ImportFromEPSG(3159)
        str = target.ExportToWkt()
        str = str[:str.rindex(",AUTHORITY")] + "]"
        str = str.replace('UTM zone 15N', 'UTM zone {}N')
        str = str.replace('"central_meridian",-93', '"central_meridian",{}')
        str = str.format(zone, meridian)
        print(str)
        print(target)
        target.ImportFromWkt(str)
        transform = osr.CoordinateTransformation(source, target)
        g.Transform(transform)
        print(g)
        print("Hi! I'm a %s with an Area  %s" % (g.GetGeometryName(), g.Area()))
        print("I have inside me %s feature(s)!\n" % g.GetGeometryCount())
        for idx, f in enumerate(g):
            print("I'm feature n.%s and I am a %s.\t I have an Area of %s - You can get my json repr with f.ExportToJson()" % (idx, f.GetGeometryName(),f.Area()))
        out_name = '{}.shp'.format(fire_name)
        out_file = os.path.join(out_dir, out_name)
        driver = ogr.GetDriverByName("Esri Shapefile")
        ds = driver.CreateDataSource(out_file)
        layr1 = ds.CreateLayer('',None, ogr.wkbPolygon)
        # create the field
        layr1.CreateField(ogr.FieldDefn('id', ogr.OFTInteger))
        # Create the feature and set values
        defn = layr1.GetLayerDefn()
        feat = ogr.Feature(defn)
        feat.SetField('id', 1)
        feat.SetGeometry(g)
        layr1.CreateFeature(feat)
        # close the shapefile
        ds.Destroy()
        target.MorphToESRI()
        with open(os.path.join(out_dir, '{}.prj'.format(fire_name)), 'w') as file:
            file.write(target.ExportToWkt())
        YEAR = 2021
        perim = firestarr_gis.rasterize_perim(out_dir, out_file, YEAR, fire_name)[1]
    if perim is None:
        sys.exit(-1)
else:
    pt = try_read_first(poly['polygon'], 'points', is_fatal=True)

    if pt is None:
        # should have already exited but check
        logging.fatal("Ignition point not initialized")
        sys.exit(-1)
    unnest_values(pt)
    lat = pt['y']
    long = pt['x']

logging.info("Startup coordinates are {}, {}".format(lat, long))

scenario = try_read_first(project['scenarios'], 'scenarios', is_fatal=True)['scenario']
start_time = scenario['startTime']['time']
start_time = pd.to_datetime(start_time)
logging.info("Scenario start time is: {}".format(start_time))
hour = start_time.hour
minute = start_time.minute

tz = (start_time.tz._minutes) / 60.0
if math.floor(tz) != tz:
    logging.fatal("Currently not set up to deal with partial hour timezones")
    sys.exit(-1)
tz = int(tz)
logging.info("Timezone offset is {}".format(tz))

date_offset = 0
start_date = datetime.date.today()
start_date = start_time.date()
if start_date != datetime.date.today():
    date_offset = (start_date - datetime.date.today()).days
    logging.warning("Simulation does not start today - date offset set to {}".format(date_offset))

url = r"http://wxshield:80/wxshield/getWx.php?model=geps&lat={}&long={}&dateOffset={}&tz={}&mode=daily".format(lat, long, date_offset, tz)
logging.debug(url)
try:
    csv = common.download(url).decode("utf-8")
except:
    logging.fatal("Unable to download weather")
    sys.exit(-3)
data = [x.split(',') for x in csv.splitlines()]
df = pd.DataFrame(data[1:], columns=data[0])
print(df)

# supposed to be really picky about inputs
#"Scenario,Date,APCP,TMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI";
df = df[['MEMBER', 'DAILY', 'PREC', 'TEMP', 'RH', 'WS', 'WD']]
df.columns = ['Scenario', 'Date', 'APCP', 'TMP', 'RH', 'WS', 'WD']
# for some reason scenario numbers are negative right now?
df['Scenario'] = df['Scenario'].apply(lambda x: -1 - int(x))
df['Date'] = df['Date'].apply(lambda x: x + " 13:00:00")
for col in ['FFMC', 'DMC', 'DC', 'ISI', 'BUI', 'FWI']:
    df[col] = 0
df.to_csv('wx.csv', index=False)

cmd = "./FireSTARR"
args = "{} {} {} {} {}:{:02d} -v --wx wx.csv --ffmc {} --dmc {} --dc {} --apcp_0800 {}".format(out_dir, start_date, lat, long, hour, minute, ffmc, dmc, dc, apcp_0800)
if perim is not None:
    args = args + " --perim {}".format(perim)
# run generated command for parsing data
run_what = [cmd] + shlex.split(args.replace('\\', '/'))
logging.info("Running: " + ' '.join(run_what))
t0 = timeit.default_timer()

def start_process(run_what, cwd):
    """!
    Start running a command using subprocess
    @param run_what Process to run
    @param flags Flags to run with
    @param cwd Directory to run in
    @return Running subprocess
    """
    logging.debug(run_what)
    p = subprocess.Popen(run_what,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE,
                           cwd=cwd)
    p.args = run_what
    return p


process = start_process(run_what, "/FireGUARD/FireSTARR")
stdout, stderr = process.communicate()
if process.returncode != 0:
    #HACK: seems to be the exit code for ctrl + c events loop tries to run it again before it exits without this
    if -1073741510 == process.returncode:
        sys.exit(process.returncode)
    raise Exception('Error running {} [{}]: '.format(process.args, process.returncode) + stderr + stdout)
t1 = timeit.default_timer()
logging.info("Took {}s to run simulations".format(t1 - t0))
with open(os.path.join(out_dir, "log.txt"), 'w') as log_file:
    log_file.write(stdout)

print(cmd)