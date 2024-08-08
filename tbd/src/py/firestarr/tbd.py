import datetime
import math
import os
import re
import shutil
import time
import timeit

import pandas as pd
import psutil
from common import (
    CONFIG,
    DIR_DATA,
    DIR_SIMS,
    DIR_TBD,
    DIR_TMP,
    FLAG_IGNORE_PERIM_OUTPUTS,
    FMT_FILE_SECOND,
    SECONDS_PER_HOUR,
    WANT_DATES,
    ensure_dir,
    force_remove,
    get_stack,
    is_newer_than,
    listdir_sorted,
    locks_for,
    logging,
    run_process,
    try_remove,
)
from gis import (
    Rasterize,
    find_best_raster,
    gdf_from_file,
    project_raster,
    save_geojson,
    save_point_file,
)
from redundancy import call_safe

from azurebatch import (
    add_simulation_task,
    check_successful,
    find_tasks_running,
    get_batch_client,
    have_batch_config,
    is_running_on_azure,
    list_nodes,
    make_or_get_job,
    make_or_get_simulation_task,
    schedule_job_tasks,
)

# set to "" if want intensity grids
NO_INTENSITY = "--no-intensity"
# NO_INTENSITY = ""

TMP_SUFFIX = "__tmp__"

_RUN_FIRESTARR = None
_FIND_RUNNING = None
JOB_ID = None
IS_USING_BATCH = None


def run_firestarr_local(dir_fire):
    stdout, stderr = None, None
    try:
        stdout, stderr = call_safe(run_process, ["./sim.sh"], dir_fire)
    except KeyboardInterrupt as ex:
        raise ex
    except Exception as ex:
        # if sim failed we want to keep track of what happened
        def save_logs(*args, **kwargs):
            if stdout:
                with open(os.path.join(dir_fire, "stdout.log"), "w") as f_log:
                    f_log.write(stdout)
            if stderr:
                with open(os.path.join(dir_fire, "stderr.log"), "w") as f_log:
                    f_log.write(stderr)

        call_safe(save_logs)

        logging.error(f"Failed running {dir_fire}")
        raise ex


def find_running_local(dir_fire):
    processes = []
    for p in psutil.process_iter(attrs=["pid", "name", "cwd"]):
        try:
            if p.name() == "tbd" and psutil.pid_exists(p.pid):
                cwd = p.cwd()
                if cwd is not None and dir_fire in cwd:
                    processes.append(cwd)
        except Exception as ex:
            # HACK: a bunch of different error types can happen if process is no longer running
            continue
    return processes


def run_firestarr_batch(dir_fire, wait=True):
    add_simulation_task(JOB_ID, dir_fire, wait=wait)


def find_running_batch(dir_fire):
    while True:
        try:
            return find_tasks_running(JOB_ID, dir_fire)
        except KeyboardInterrupt as ex:
            raise ex
        except Exception as ex:
            pass


def get_simulation_task(dir_fire):
    return make_or_get_simulation_task(JOB_ID, dir_fire)


def schedule_tasks(tasks):
    schedule_job_tasks(JOB_ID, tasks)


def get_nodes():
    return list_nodes()


def assign_firestarr_batch(dir_fire, force_local=None, force_batch=None):
    global _RUN_FIRESTARR
    global _FIND_RUNNING
    global JOB_ID
    if force_local is None:
        force_local = CONFIG.get("FORCE_LOCAL_TASKS", False)
    if force_batch is None:
        force_batch = CONFIG.get("FORCE_BATCH_TASKS", False)
    if force_local:
        logging.warning("Forcing local tasks")
    if force_batch:
        logging.warning("Forcing batch tasks")
    if force_local and force_batch:
        raise RuntimeError("Can't set both of FORCE_LOCAL_TASKS and FORCE_BATCH_TASKS")
    with locks_for(os.path.join(DIR_DATA, "assign_batch_client")):
        if not force_local and have_batch_config():
            if not is_running_on_azure():
                logging.warning("Not running on azure but using batch")
            logging.info("Running using batch tasks")
            _RUN_FIRESTARR = run_firestarr_batch
            job_id = None
            if dir_fire.startswith(DIR_SIMS):
                job_id = dir_fire.replace(DIR_SIMS, "").strip("/")
                job_id = job_id[: job_id.index("/")]
            JOB_ID = make_or_get_job(job_id=job_id)
            _FIND_RUNNING = find_running_batch
            return True
        if force_batch:
            raise RuntimeError("Forcing batch mode but no config set")
        logging.info("Running using local tasks")
        _RUN_FIRESTARR = run_firestarr_local
        _FIND_RUNNING = find_running_local
        return False


def check_firestarr_batch(dir_fire):
    with locks_for(os.path.join(DIR_DATA, "check_batch_client")):
        if _RUN_FIRESTARR is None:
            assign_firestarr_batch(dir_fire)
    return _RUN_FIRESTARR(dir_fire)


def check_firestarr_running(dir_fire):
    with locks_for(os.path.join(DIR_DATA, "check_batch_client")):
        if _FIND_RUNNING is None:
            assign_firestarr_batch(dir_fire)
    return _FIND_RUNNING(dir_fire)


def run_firestarr(dir_fire):
    # FIX: this should definitely not be returning clock time if it's supposed to be simulation time
    # run generated command for parsing data
    t0 = timeit.default_timer()
    # expect everything to be in sim.sh
    (_RUN_FIRESTARR or check_firestarr_batch)(dir_fire)
    t1 = timeit.default_timer()
    sim_time = t1 - t0
    return sim_time


def find_running(dir_fire):
    return (_FIND_RUNNING or check_firestarr_running)(dir_fire)


def check_running(dir_fire):
    processes = find_running(dir_fire)
    return 0 < len(processes)


def finish_job():
    if IS_USING_BATCH is None:
        logging.error("Didn't use batch, but trying to finish job")
    else:
        if check_successful(JOB_ID):
            get_batch_client().job.terminate(JOB_ID)
        else:
            logging.error(f"Finishing incomplete job {JOB_ID}")


def get_simulation_file(dir_fire):
    fire_name = os.path.basename(dir_fire)
    return os.path.join(dir_fire, f"firestarr_{fire_name}.geojson")


def find_outputs(dir_fire):
    files = [x for x in listdir_sorted(dir_fire)]
    # FIX: include perimeter file
    files_tiff = [x for x in files if x.endswith(".tif")]
    probs_all = [x for x in files_tiff if "probability" in x]
    files_prob = [os.path.join(dir_fire, x) for x in probs_all if x.startswith("probability")]
    files_interim = [os.path.join(dir_fire, x) for x in probs_all if x.startswith("interim")]
    files_perim = [os.path.join(dir_fire, x) for x in files_tiff if os.path.basename(dir_fire) in x]
    return files_prob, files_interim, files_perim


def copy_fire_outputs(dir_fire, dir_output, changed):
    # simulation was done or is now, but outputs don't exist
    logging.debug(f"Collecting outputs from {dir_fire}")
    fire_name = os.path.basename(dir_fire)
    files_prob, files_interim, files_perim = find_outputs(dir_fire)
    # HACK: keep track of changed files so we can make sure we copy new things until we can simplify this
    files_changed = {}
    extent = None
    dir_region = ensure_dir(os.path.join(dir_output, "initial"))
    suffix = ""
    is_interim = False
    if files_interim and files_prob:
        # check timestamp and remove if older than interim
        if len(files_interim) != len(files_prob) or any(
            [is_newer_than(files_interim[i], files_prob[i]) for i in range(len(files_interim))]
        ):
            logging.warning(f"Ignoring {files_prob} because {files_interim} is newer")
            # NOTE: is there any reason to not delete these?
            # HACK: make a point of marking these as changed
            for file_old in files_prob:
                files_changed[file_old] = True
                # NOTE: could be outputting currently in another thread so don't delete
                # try_remove(file_old)
            files_prob = []
            for f in files_prob:
                files_changed[f] = True
    if files_interim and not files_prob:
        logging.debug(f"Using interim rasters for {dir_fire}")
        dir_tmp_fire = ensure_dir(os.path.join(DIR_TMP, os.path.basename(dir_output), "interim", fire_name))
        force_remove(dir_tmp_fire)
        call_safe(shutil.copytree, dir_fire, dir_tmp_fire, dirs_exist_ok=True)
        # double check that outputs weren't created while copying
        probs_tmp, interim_tmp, files_perim = find_outputs(dir_tmp_fire)
        # HACK: since we already avoided files_prob if they were out of date then never worry about probs_tmp
        for f_interim in interim_tmp:
            f_tmp = f_interim.replace("interim_", "")
            shutil.move(f_interim, f_tmp)
        probs_tmp, interim_tmp, files_perim = find_outputs(dir_tmp_fire)
        if interim_tmp:
            raise RuntimeError("Expected files to be renamed")
        files_prob = probs_tmp
        for f in files_prob:
            files_changed[f] = True
        # # force copying because not sure when interim is from
        # changed = True
        suffix = TMP_SUFFIX
        is_interim = True
    files_project = {}
    if files_prob:
        for prob in files_prob:
            logging.debug(f"Adding raster to final outputs: {prob}")
            # want to put each probability raster into right date so we can combine them
            d = prob[(prob.rindex("_") + 1) : prob.rindex(".tif")].replace("-", "")
            # FIX: want all of these to be output at the size of the largest?
            # FIX: still doesn't show whole area that was simulated
            file_out = os.path.join(dir_region, d, f"{fire_name}{suffix}.tif")
            file_out_interim = os.path.join(dir_region, d, f"{fire_name}{TMP_SUFFIX}.tif")
            if file_out != file_out_interim:
                # remove interim if we have final
                force_remove(file_out_interim)
                # FIX: which one?
                files_changed[file_out] = True
            files_changed[prob] = True
            files_project[prob] = file_out
    if not FLAG_IGNORE_PERIM_OUTPUTS:
        if len(files_perim) > 0:
            file_out = os.path.join(dir_region, "perim", f"{fire_name}{suffix}.tif")
            files_project[files_perim[0]] = file_out
    extent = None
    for file_src, file_out in files_project.items():
        changed = changed or file_src in files_changed or file_out in files_changed
        if changed or is_newer_than(file_src, file_out):
            logging.debug(f"Adding raster to final outputs: {file_src}")
            # if writing over file then get rid of it
            force_remove(file_out)
            # using previous extent is limiting later days
            if "perim" not in file_src:
                extent = None
            extent = project_raster(
                file_src,
                file_out,
                outputBounds=extent,
                # HACK: if nodata is none then 0's should just show up as 0?
                nodata=None,
            )
            if extent is None:
                raise RuntimeError(f"Fire {dir_fire} has invalid output file {file_src}")
            # if file didn't exist then it's changed now
            changed = True
    return changed, is_interim, files_project


def parse_sim_time(file_log):
    # try parsing log for simulation time
    sim_time = None
    try:
        if os.path.isfile(file_log):
            # if log says it ran then don't run it
            # HACK: just use tail instead of looping or seeking ourselves
            stdout, stderr = run_process(["tail", "-1", file_log], "/appl/tbd")
            if stdout:
                line = stdout.strip().split("\n")[-1]
                g = re.match(".*Total simulation time was (.*) seconds", line)
                if g and g.groups():
                    sim_time = int(g.groups()[0])
    except KeyboardInterrupt as ex:
        raise ex
    except Exception:
        pass
    return sim_time


def run_fire_from_folder(
    dir_fire,
    dir_output,
    verbose=False,
    prepare_only=False,
    run_only=False,
    no_wait=False,
):
    def nolog(*args, **kwargs):
        pass

    def dolog(msg, *args, **kwargs):
        logging.info(f"{dir_fire}: {msg}", *args, **kwargs)

    if prepare_only and run_only:
        raise RuntimeError("Can't prepare_only and run_only at the same time")
    log_info = dolog if verbose else nolog

    was_running = check_running(dir_fire)
    if was_running:
        log_info(f"Already running {dir_fire} - waiting for it to finish")
    while check_running(dir_fire):
        time.sleep(10)
    if was_running:
        log_info(f"Continuing after {dir_fire} finished running")

    file_sim = get_simulation_file(dir_fire)
    file_sh = os.path.join(dir_fire, "sim.sh")
    files_required = [file_sim, file_sh]
    # need directory for lock
    ensure_dir(os.path.dirname(file_sim))
    # lock before reading so if sim is running it will update file before lock ends
    with locks_for(file_sim):
        df_fire = gdf_from_file(file_sim) if os.path.isfile(file_sim) else None
        if df_fire is None:
            force_remove(files_required)
            raise RuntimeError(f"Couldn't get fire data from {file_sim}")
        if 1 != len(df_fire):
            force_remove(files_required)
            raise RuntimeError(f"Expected exactly one fire in file {file_sim}")
        data = df_fire.iloc[0]
        file_wx = os.path.join(dir_fire, data["wx"])
        files_required.append(file_wx)
        # check if completely done
        # if data.get("postprocessed", False):
        #     df_fire["changed"] = False
        #     return df_fire
        changed = False
        fire_name = data["fire_name"]
        # file_log = file_sim.replace(".geojson", ".log")
        file_log = os.path.join(dir_fire, "firestarr.log")
        df_fire["log_file"] = file_log
        sim_time = data.get("sim_time", None)
        if not sim_time:
            sim_time = parse_sim_time(file_log)
            # rely on sim_time being applied to df_fire later
            # if sim_time is not None:
            #     try:
            #         # HACK: repeat here for now
            #         df_fire["sim_time"] = sim_time
            #         if "dates_out" in df_fire.columns:
            #             del df_fire["dates_out"]
            #         save_geojson(df_fire, file_sim)
            #     except KeyboardInterrupt as ex:
            #         raise ex
            #     except Exception:
            #         pass
            # HACK: save if found sim_tim
            df_fire["sim_time"] = sim_time
            save_geojson(df_fire, file_sim)
        want_dates = WANT_DATES
        max_days = data["max_days"]
        date_offsets = [x for x in want_dates if x <= max_days]
        # HACK: rerun if not enough outputs
        outputs = listdir_sorted(dir_fire)
        probs = [x for x in outputs if x.endswith("tif") and x.startswith("probability")]
        if not sim_time or len(probs) != len(date_offsets):
            if prepare_only and os.path.isfile(file_sh):
                # save changes or else groups won't have startup indices
                save_geojson(df_fire, file_sim)
                return df_fire
            if not run_only or not os.path.isfile(file_sh):
                lat = float(data["lat"])
                lon = float(data["lon"])
                start_time = pd.to_datetime(data["start_time"])
                log_info(f"Scenario start time is: {start_time}")
                if "Point" != data.geometry.geom_type:
                    year = start_time.year
                    reference = find_best_raster(lon, year)
                    raster = os.path.join(dir_fire, "{}.tif".format(fire_name))
                    with locks_for(raster):
                        # FIX: if we never use points then the sims don't guarantee
                        # running from non-fuel for the points like normally
                        perim = Rasterize(file_sim, raster, reference)
                else:
                    perim = None
                # NOTE: save point file either way so we can see where it is
                # think this should be fine for using individual points
                save_point_file(lat, lon, dir_fire, fire_name)
                log_info("Startup coordinates are {}, {}".format(lat, lon))
                hour = start_time.hour
                minute = start_time.minute
                tz = start_time.tz.utcoffset(start_time).total_seconds() / SECONDS_PER_HOUR
                # HACK: I think there might be issues with forecasts being at
                #           the half hour?
                if math.floor(tz) != tz:
                    logging.warning("Rounding down to deal with partial hour timezone")
                    tz = math.floor(tz)
                tz = int(tz)
                log_info("Timezone offset is {}".format(tz))
                start_date = start_time.date()
                cmd = os.path.join(DIR_TBD, "tbd")
                # want format like a list with no spaces
                fmt_offsets = "[" + ",".join([str(x) for x in date_offsets]) + "]"

                def strip_dir(path):
                    p = os.path.abspath(path)
                    d = os.path.abspath(dir_fire)
                    if p.startswith(d):
                        p = p[len(d) + 1 :]
                    if 0 == len(p):
                        p = "."
                    return p

                # FIX: --log doesn't work right now
                args = " ".join(
                    [
                        f'"{strip_dir(dir_fire)}" {start_date} {lat} {lon}',
                        f"{hour:02d}:{minute:02d}",
                        NO_INTENSITY,
                        f"--ffmc {data['ffmc_old']}",
                        f"--dmc {data['dmc_old']}",
                        f"--dc {data['dc_old']}",
                        f"--apcp_prev {data['apcp_prev']}",
                        "-v",
                        # "-v",
                        # "-v",
                        f"--output_date_offsets {fmt_offsets}",
                        f"--wx {strip_dir(file_wx)}",
                        # f"--log {strip_dir(file_log)}",
                    ]
                )
                if perim is not None:
                    args = args + f" --perim {strip_dir(perim)}"
                args = args.replace("\\", "/")

                def mk_sim_sh(*a, **k):
                    with open(file_sh, "w") as f_out:
                        # HACK: use tee to pipe to file and stdout
                        # add $* at end so with can call with more args from cli
                        f_out.writelines(["#!/bin/bash\n", f"{cmd} {args} $* 2>&1 | tee -a from_tee.log\n"])

                call_safe(mk_sim_sh)
                # NOTE: needs to be octal base
                os.chmod(file_sh, 0o775)
                if prepare_only:
                    # is prepared but not run
                    return df_fire
            try:
                # HACK: return this again since it waits for the fire to finish at the start
                if check_running(dir_fire):
                    logging.info(f"Retrying for running fire {dir_fire}")
                    return run_fire_from_folder(
                        dir_fire,
                        dir_output,
                        verbose=verbose,
                        prepare_only=prepare_only,
                        run_only=run_only,
                        no_wait=no_wait,
                    )
                # if we're going to run then move old log if it exists
                if os.path.isfile(file_log):
                    filetime = os.path.getmtime(file_log)
                    filedatetime = datetime.datetime.fromtimestamp(filetime)
                    file_log_old = file_log.replace(".log", f"{filedatetime.strftime(FMT_FILE_SECOND)}.log")
                    logging.warning(f"Moving old log file from {file_log} to {file_log_old}")
                    shutil.move(file_log, file_log_old)
                try:
                    real_time = run_firestarr(dir_fire)
                    # parse from file instead of using clock time
                    sim_time = parse_sim_time(file_log)
                except FileNotFoundError as ex:
                    # HACK: work around python not seeing processes that are too fast
                    # seems to be happening when process finishes so quickly that python is still looking for it
                    #       [Errno 2] No such file or directory: '/proc/[0-9]*/cwd'
                    # parse from file instead of using clock time
                    sim_time = parse_sim_time(file_log)
                    if sim_time is None:
                        raise ex
            except KeyboardInterrupt as ex:
                raise ex
            except Exception as ex:
                logging.error(f"Couldn't run fire {dir_fire}")
                logging.error(get_stack(ex))
                # force_remove(files_required)
                # return None
                raise ex
            log_info("Took {}s to run simulations".format(sim_time))
        elif prepare_only:
            # still need to run with run_only to copy outputs
            return df_fire
        else:
            # log_info("Simulation already ran but don't have processed outputs")
            log_info("Simulation already ran")
            return df_fire
        # try:
        #     # save time if parsed or None if sim failed
        #     df_fire["sim_time"] = sim_time
        #     if "dates_out" in df_fire.columns:
        #         del df_fire["dates_out"]
        #     save_geojson(df_fire, file_sim)
        # except KeyboardInterrupt as ex:
        #     raise ex
        # except Exception:
        #     pass
        # save time if parsed or None if sim failed
        if not sim_time:
            logging.error(f"Simulation time {sim_time} is invalid for {dir_fire}")
        df_fire["sim_time"] = sim_time
        # if "dates_out" in df_fire.columns:
        #     del df_fire["dates_out"]
        save_geojson(df_fire, file_sim)
        changed, is_interim, files_project = copy_fire_outputs(dir_fire, dir_output, changed)
        df_fire["changed"] = changed
        return df_fire
