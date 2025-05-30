#!/usr/bin/env python

import sys
import os
import json
from datetime import datetime
import shutil

sys.path.append(os.path.expanduser('~/basetrade/'))

from grid.client.api import GridClient
from walkforward.definitions import execs
from walkforward.utils.process_pduration import get_duration_in_algospace
from pylib.indstats_db_access_manager import *
from walkforward.utils.run_exec import exec_function

GRID_URL = "http://10.1.4.15:5000"

# gets the uniques gsm id date +%N


def get_unique_gsm_id():
    return exec_function("date +%N")[0].strip()

# make the major ilist with all the indicators


def make_major_ilist(shc, basepx, futpx, main_ilist, product_prod_indc_n_indc_id, product):
    if os.path.exists(main_ilist):
        return 0
    else:
        ILIST_ = open(main_ilist, "w")
        ILIST_.write(get_ilist_header(shc, basepx, futpx))
        indicator_count = 0
        for prod_indc_id_n_ind in product_prod_indc_n_indc_id[product]:
            prod_indc_indicator = prod_indc_id_n_ind.split("^")
            ind = prod_indc_indicator[1]
            indicator = "INDICATOR 1.00 " + ind + "\n"
            ILIST_.write(indicator)
            indicator_count += 1
        ILIST_.close()
        return 0

def get_indicator_shortcode_map(product, product_prod_indc_n_indc_id, work_dir, intermediate_files_):
    indicators = []
    indicator_to_prod_indc_id = {}
    for prod_indc_id_n_ind in product_prod_indc_n_indc_id[product]:
        prod_indc_indicator = prod_indc_id_n_ind.split("^")
        prod_indc_id = prod_indc_indicator[0]
        ind = prod_indc_indicator[1]
        indicators.append(ind)
        indicator_to_prod_indc_id["INDICATOR 1.00 " + ind] = prod_indc_id

    temp_ilist = work_dir + "/temp_ilist"
    with open(temp_ilist, "w") as fout:
        fout.write("\n".join(indicators))

    ## Any date would do. Using today's date.
    date_today = datetime.strftime(datetime.now(),"%Y%m%d")
    cmd_collect_shortcodes = " ".join([execs.execs().collect_shortcodes_from_model, temp_ilist, date_today, "2"])
    shc_for_inds = exec_function(cmd_collect_shortcodes)[0].strip().splitlines()
    ## delete temp_ilist file
    intermediate_files_.append(temp_ilist)

    shc_uniq = []  ## list of all distinct shortcodes across all inds
    ind_shcs = {}  ## Indicator to shortcodes map
    for i in range(len(shc_for_inds)):
        shcs = shc_for_inds[i].strip().split()
        ind_shcs[indicators[i]] = shcs
        shc_uniq.extend(shcs)
    shc_uniq = list(set(shc_uniq))
    return ind_shcs, shc_uniq, indicator_to_prod_indc_id

# make the ilist for date by checking for holiday for each indicator and prod_indc_ids to skip based on recompute flag
# and returns the map if indicator to prod_indc_id and the list of indicators_written in the order they are written


def make_ilist_for_date(shc, basepx, futpx, date, shc_uniq, ind_shcs, product_prod_indc_n_indc_id, prod_indc_id_to_skip,
                        holiday_shcs_ilistfile, ilistfile_dates, ilistfile_prefix, product):
    shc_with_holiday = []
    holiday_manager_exec = execs.execs().holiday_manager
    for shortcode in shc_uniq:
        cmd_for_holiday = [holiday_manager_exec, "PRODUCT", shortcode, date, "T"]
        cmd_for_holiday = " ".join(cmd_for_holiday)
        is_holiday = int(exec_function(cmd_for_holiday)[0].strip())
        if is_holiday == 1:
            shc_with_holiday.append(shortcode)

    holiday_shcs_str = "^".join(sorted(shc_with_holiday))

    if holiday_shcs_str in holiday_shcs_ilistfile.keys():
        if holiday_shcs_ilistfile[holiday_shcs_str] not in ilistfile_dates.keys():
            print("Something went wrong. holiday_str already seen but ilist file does not in ilistfile_dates dict.")
        ilistfile_dates[holiday_shcs_ilistfile[holiday_shcs_str]].append(date)
        return

    indicator_written = []

    for prod_indc_id_n_ind in product_prod_indc_n_indc_id[product]:
        prod_indc_indicator = prod_indc_id_n_ind.split("^")
        prod_indc_id = prod_indc_indicator[0]
        if prod_indc_id in prod_indc_id_to_skip:
            continue
        ind = prod_indc_indicator[1]
        shcs = ind_shcs[ind]
        to_continue = 0
        for shc in shcs:
            if shc in shc_with_holiday:
                to_continue = 1
        if to_continue:
            continue
        ind_str = "INDICATOR 1.00 "+ind
        indicator_written.append(ind_str)

    if len(indicator_written)  != 0:
        ilist_file_name = ilistfile_prefix + "_holiday_shcs_" + holiday_shcs_str
        ilist = open(ilist_file_name, "w")
        ilist.write(get_ilist_header(shc, basepx, futpx))
        ilist.write("\n".join(indicator_written)+"\n")
        ilist.write("INDICATOREND")
        ilist.close()
        intermediate_files_.append(ilist_file_name)

        holiday_shcs_ilistfile[holiday_shcs_str] = ilist_file_name
        ilistfile_dates[ilist_file_name] = [date]


# we store the output of timed_data_to_corr_record to a file and then read it to get the map of indicator to correlation and indicator to tail_correaltion


def parse_corr_file(corrs_log_file, indicators_written_list):
    indicators_to_corr = {}
    indicators_to_tail_corr = {}
    f = open(corrs_log_file, "r")
    lines_vec = f.read().splitlines()
    for line in lines_vec:
        words_vec = line.strip().split()
        if float(words_vec[0][5:]) < 0.2 and words_vec[0][:5] == "tail_":
            for i in range(0, len(indicators_written_list)):
                indicators_to_corr[indicators_written_list[i]] = words_vec[i + 3]
        else:
            for i in range(0, len(indicators_written_list)):
                indicators_to_tail_corr[indicators_written_list[i]] = words_vec[i + 3]

    return indicators_to_corr, indicators_to_tail_corr


# returns the ilist header
def get_ilist_header(shc, dep_base_pricetype_, dep_pred_pricetype_):
    ilist_header_ = "MODELINIT DEPBASE " + shc + " " + dep_base_pricetype_ + \
        " " + dep_pred_pricetype_ + "\nMODELMATH LINEAR CHANGE\nINDICATORSTART\n"
    return ilist_header_

# gets the min_price_increment


def get_min_price_increment(shc, date):
    cmd_min_price_increment = [execs.execs().get_min_price_increment, shc, date]
    cmd_min_price_increment = " ".join(cmd_min_price_increment)

    min_price_increment_ = exec_function(cmd_min_price_increment)[0].strip()
    return min_price_increment_

## run_datagen
def run_datagen(ilist, start_time, end_time, dates):

    grid_datagen_json = {'job': "generate_data", 'ilist': ilist, 'msecs': '4000', 'l1events': 'c3', 'trades': '0',
                         'eco_mode': '0', 'start_time': start_time,
                         'end_time': end_time, 'dates': [], 'stats_args': None}
    for date in dates:
        grid_datagen_json['dates'].append(str(date))

    print(grid_datagen_json)
    work_dir = os.path.dirname(ilist)

    temp_json_file = os.path.join(work_dir, "temp_json_file")
    file_handle = open(temp_json_file, 'w')
    file_handle.write(json.dumps(grid_datagen_json, sort_keys=True, separators=(',', ':'), indent=2))
    file_handle.close()

    grid_client = GridClient(server_url=GRID_URL, username=os.getenv("GRID_USERNAME"),
                             password=os.getenv("GRID_PASSWORD"), grid_artifacts_dir=work_dir)
    print("Submitting datagen job. Please wait for a while...")
    output_directory = grid_client.submit_job(json.dumps(grid_datagen_json))
    return output_directory

# executes timed_data_to_corr_record,


def execute_cmd_for_corrs(shc, preddur_, date, predalgo_, datagen_start_hhmm_,
                          datagen_end_hhmm_, min_price_increment_, product, ilist_file_name, datagen_filename_, corr_filename_, _intermediate_files_):

    pred_counter = str(get_duration_in_algospace(shc, preddur_, date, predalgo_, datagen_start_hhmm_,
                                                 datagen_end_hhmm_))

    cmd_for_corrs_ = [execs.execs().timed_data_to_corr_record, ilist_file_name,
                      datagen_filename_, min_price_increment_, predalgo_, "1", filter_, "1", pred_counter]

    if (filter_ == "fv"):
        # do the trade volume file generation part
        trade_per_sec_file = work_dir_ + "/" + product + "_trd_per_sec"
        cmd_for_trade_per_sec_file = [execs.execs().daily_trade_aggregator, shc, date, trade_per_sec_file]
        cmd_for_trade_per_sec_file = " ".join(cmd_for_trade_per_sec_file)
        process = subprocess.Popen(cmd_for_trade_per_sec_file, shell=True,
                                   stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        out, err = process.communicate()
        errcode = process.returncode
        if errcode != 0:
            sys.stderr.write("cmd_for_trade_per_sec_file failed " + cmd_for_trade_per_sec_file + "\n\n")
            sys.exit()

        _intermediate_files_.append(trade_per_sec_file)
        cmd_for_corrs_ = cmd_for_corrs_ + [trade_per_sec_file, "0", "2", "0.1", "1.0"]

    elif (filter == "fsudm"):
        cmd_for_corrs_ = cmd_for_corrs_ + [" INVALIDFILE", "1", "2", "0.1", "1.0"]
    elif (filter == "fsudm2"):
        cmd_for_corrs_ = cmd_for_corrs_ + [" INVALIDFILE", "2", "2", "0.1", "1.0"]
    elif (filter == "fsudm3"):
        cmd_for_corrs_ = cmd_for_corrs_ + [" INVALIDFILE", "3", "2", "0.1", "1.0"]
    else:
        cmd_for_corrs_ = cmd_for_corrs_ + [" INVALIDFILE", "0", "2", "0.1", "1.0"]

    cmd_for_corrs_ = cmd_for_corrs_ + [" > ", corr_filename_]

    _intermediate_files_.append(corr_filename_)
    cmd_for_corrs_ = " ".join(cmd_for_corrs_)
    errcode = os.system(cmd_for_corrs_)
    return errcode
    # if errcode != 0:
    #     sys.stderr.write("cmd_for_corrs_ failed " + cmd_for_corrs_ + "\n\n")
    #     sys.exit()


# check for number of arguments
if len(sys.argv) < 12:
    print("USAGE: <shc> <datesfile> <datagen_start_hhmm_> <datagen_end_hhmm_> <preddur_> <predalgo_> <filter_> <basepx> <futpx> <recompute> <work_dir> \n")
    sys.exit()

# parse the arguments
shc = sys.argv[1]
datesfile = sys.argv[2]
datagen_start_hhmm_ = sys.argv[3]
datagen_end_hhmm_ = sys.argv[4]
preddur_ = sys.argv[5]
predalgo_ = sys.argv[6]
filter_ = sys.argv[7]
basepx = sys.argv[8]
futpx = sys.argv[9]
recompute = sys.argv[10]
work_dir_ = sys.argv[11]

# make the product from the arguments
product = shc + "^" + datagen_start_hhmm_ + "-" + datagen_end_hhmm_ + "^" + \
    preddur_ + "^" + predalgo_ + "^" + filter_ + "^" + basepx + "^" + futpx

# initiate the datagen arguments
# datagen_msecs_timeout_ = "4000"
# datagen_l1events_timeout_ = "c3"
# datagen_num_trades_timeout_ = "0"
# to_print_on_economic_times_ = "0"
intermediate_files_ = []


# # make ilist file name prefix
ilistfile_prefix = work_dir_ + "/ilist_" + shc + "_" + datagen_start_hhmm_ + "_" + datagen_end_hhmm_ + "_" + preddur_ +\
    "_" + predalgo_ + "_" + filter_ + "_" + basepx

#
# Create and instance of IndStatsDBAcessManager to query the database
IndStatsObject = IndStatsDBAcessManager()
IndStatsObject.open_conn()

# gets the map of product to prod_indc_id^indicator
product_prod_indc_n_indc_id = IndStatsObject.get_map_prod_prod_indc_n_indc_id_for_shc(shc)

main_ilist = work_dir_ + "/ilist_" + shc + "_" + datagen_start_hhmm_ + "_" + datagen_end_hhmm_ + "_" + preddur_ +\
    "_" + predalgo_ + "_" + filter_ + "_" + basepx + "_" + futpx

# if the ilist doesn't exist make it
if not(os.path.exists(main_ilist)):
    make_major_ilist(shc, basepx, futpx, main_ilist, product_prod_indc_n_indc_id, product)

ind_shcs, shc_uniq, indicator_to_prod_indc_id = get_indicator_shortcode_map(product, product_prod_indc_n_indc_id, work_dir_, intermediate_files_)

holiday_shcs_ilistfile = {}
ilistfile_dates = {}
with open(datesfile, "r") as fp:
    for line in fp.readlines():
        date  = line.strip()
        # # gets the prod_indc_ids to skip
        prod_indc_id_to_skip = IndStatsObject.GetProdIndcidtoSkip(shc, recompute, date)
        # # call make_ilist_for_date -> which would return dictionary with keys as ilist file name and values as list of dates corresponding to that ilist
        make_ilist_for_date(shc, basepx, futpx, date, shc_uniq, ind_shcs, product_prod_indc_n_indc_id, prod_indc_id_to_skip,
                            holiday_shcs_ilistfile, ilistfile_dates, ilistfile_prefix, product)

# # call dategen for each key, value pair in ilistfile_dates dict.
for ilistfile_name, dates in ilistfile_dates.items():
    #print("Product: ", product)
    #print(datetime.now(), "ilist_file: ", ilistfile_name, ", num of dates for datagen: ", len(dates))

    #call_datagen
    output_directory = run_datagen(ilistfile_name, datagen_start_hhmm_, datagen_end_hhmm_, dates)
    print("output directory: ", output_directory)

    ## indicator_written_list = read_from_ilistfile in ilistfile_dates
    indicators_written_list = []
    for line in open(ilistfile_name, "r").readlines():
        if line.strip().split()[0] not in ["MODELINIT", "MODELMATH", "INDICATORSTART", "INDICATOREND"]:
            indicators_written_list.append(line.strip())

    for date in dates:
        print("corr computation", date)
        datagen_filename_ = os.path.join(output_directory, "artifacts", "datagen", str(date) + ".txt")
        # run timed_data_to_corr_record
        corr_filename_ = work_dir_ + "/corr_list_" + product + "_" + date + ".txt"

        min_price_increment_ = get_min_price_increment(shc, date)

        errcode = execute_cmd_for_corrs(shc, preddur_, date, predalgo_, datagen_start_hhmm_,
                                         datagen_end_hhmm_, min_price_increment_, product, ilistfile_name,
                                         datagen_filename_, corr_filename_, intermediate_files_)
        if errcode != 0:
            sys.stderr.write("execute_cmd_for_corrs failed for date {} \n\n".format(date))
            continue

        # parse the correlation file and get the map of indicators_to_corr and indicators_to_tail_corr
        indicators_to_corr, indicators_to_tail_corr = parse_corr_file(corr_filename_, indicators_written_list)

        # keep a count of indicator_inserted
        indicator_inserted = 0

        # insert the correlations
        for indicator in list(indicators_to_corr.keys()):
            # get the prod_indc_id for the indicator
            prod_indc_id = indicator_to_prod_indc_id[indicator]

            # insert NULL if correlation or tail correlation is nan
            if indicators_to_corr[indicator] == "nan":
                indicators_to_corr[indicator] = "NULL"
            if indicators_to_tail_corr[indicator] == "nan":
                indicators_to_tail_corr[indicator] = "NULL"

            retval = IndStatsObject.InsertIndStats(
                prod_indc_id, date, indicators_to_corr[indicator], indicators_to_tail_corr[indicator])

            if retval == 1:
                indicator_inserted += 1
            else:
                sys.stderr.write("Indicator insertion failed for " + product + " " + indicator,
                                 date + " " + indicators_to_corr[indicator] +
                                 " " + indicators_to_tail_corr[indicator])

        print(("#Product " + product))
        print(("#Date " + date + " total no of Indicators " + str(
            len(product_prod_indc_n_indc_id[product])) + " # Of Indicators for date " +
               str(len(list(indicators_to_corr.keys()))) + " # Of Indicators Sucessfully Added " + str(
                    indicator_inserted) + " \n"))

# remove the files
for file_ in intermediate_files_:
    os.remove(file_)
