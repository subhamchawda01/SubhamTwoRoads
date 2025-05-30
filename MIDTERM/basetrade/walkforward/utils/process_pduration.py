import subprocess
import warnings
from walkforward.definitions import execs


def get_duration_in_algospace(shortcode, duration_in_tspace, end_date, pred_algo, start_time, end_time):

    # give pred duration is in seconds, by default leaving in time space
    # timed_data has msecs from midnight and events count
    # so we pass duration in msecs for time processing algos
    # and we pass events ( converting time space to events ) for event algos

    duration_in_algo_space = 1000 * int(duration_in_tspace)

    if pred_algo == "na_e1" or pred_algo == "na_e3" or pred_algo == "na_e5" or pred_algo == "ac_e3" or pred_algo == "ac_e5" or pred_algo == "na_m4":
        avg_events_per_sec = 1.0
        cmd = [execs.execs().sample_data, shortcode, end_date, "30", start_time, end_time, "0", "L1EVPerSec"]
        process = subprocess.Popen(' '.join(cmd), shell=True,
                                   stderr=subprocess.PIPE,
                                   stdout=subprocess.PIPE)
        out, err = process.communicate()
        if out is not None:
            out = out.decode('utf-8')
        if err is not None:
            err = err.decode('utf-8')
        errcode = process.returncode

        out = float(out.strip().split(" ")[2])

        if out > 0:
            avg_events_per_sec = out
        else:
            warnings.warn("avg_sample_data has failed to return value > 0, for shortcode " + shortcode +
                          " " + end_date + " 30 " + start_time + " " + end_date + " 0 L1EVPerSec" + err)

        duration_in_algo_space = avg_events_per_sec * int(duration_in_tspace)
        print(duration_in_algo_space)
    return duration_in_algo_space


def get_pred_counters(shortcode, pred_duration, date, pred_algo, timed_file, start_time, end_time):

    # give pred duration is in seconds
    pred_counters = 1000 * int(pred_duration)

    if pred_algo == "na_t1" or pred_algo == "na_t3" or pred_algo == "na_t5" or pred_algo == "na_s4":
        pred_counters = 1000 * int(pred_duration)
    if pred_algo == "na_e1" or pred_algo == "na_e3" or pred_algo == "na_e5" or pred_algo == "ac_e3" or pred_algo == "ac_e5" or pred_algo == "na_m4":
        avg_events_per_sec = 1.0
        prev_day = calc_prev_week_day(date, 1)
        cmd = [execs.execs().sample_data, shortcode, prev_date, "30", start_time, end_time, "0", "L1EVPerSec"]
        process = subprocess.Popen(' '.join(cmd), shell=True,
                                   stderr=subprocess.PIPE,
                                   stdout=subprocess.PIPE)
        out, err = process.communicate()
        if out is not None:
            out = out.decode('utf-8')
        if err is not None:
            err = err.decode('utf-8')

        errcode = process.returncode

        out = float(out.strip().split(" ")[2])

        if out > 0:
            avg_events_per_sec = out
        else:
            cmd = [execs.execs().get_average_events_per_second, timed_file]
            process = subprocess.Popen(' '.join(cmd), shell=True,
                                       stderr=subprocess.PIPE,
                                       stdout=subprocess.PIPE)
            out, err = process.communicate()
            if out is not None:
                out = out.decode('utf-8')
            if err is not None:
                err = err.decode('utf-8')

            errcode = process.returncode

            avg_events_per_sec = float(out)

        pred_counters = avg_events_per_sec * int(pred_duration)

    return pred_counters
