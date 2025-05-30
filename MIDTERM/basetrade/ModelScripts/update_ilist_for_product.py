#!/usr/bin/env python

# \file ModelScripts/generate_indicator_stats_2+pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551

import sys
import os
import argparse

sys.path.append(os.path.expanduser('~/basetrade/'))

from pylib.indstats_db_access_manager import *


# write ilist for indicators list


def write_ilist(ilist_file_name, shc, indicators_list):
    ILIST = open(ilist_file_name, "w")
    ILIST.write("MODELINIT DEPBASE " + shc + " OfflineMixMMS OfflineMixMMS\n" "MODELMATH LINEAR CHANGE\nINDICATORSTART\n")

    for indicator in indicators_list:
        ILIST.write("INDICATOR 1.00 " + indicator + "\n")

    ILIST.write("INDICATOREND")
    ILIST.close()

# parse the indicators from a list to a list and return the list


def get_indicators_from_ilist(ilist_file_name):
    ILIST = open(ilist_file_name, "r")
    lines = ILIST.read().splitlines()

    indicators_list = []
    for line in lines:
        indx = line.find("#")
        if indx == 0:
            continue
        if indx != -1:
            line = line[:indx]
        words_list = line.strip().split()
        if words_list[0] == "INDICATOR":
            indicators_list.append(" ".join(words_list[2:]))
    if len(indicators_list) > 1000:
        print("Max indicators allowed = 1000, please be kind to us")
        sys.exit(1)
    return indicators_list

# indicators to delete are those already present in the DB but not in the ilist, and vice versa for indicators to insert


def get_indicators_to_delete_and_insert(indicator_ids_already_present, indicator_ids_new):
    to_delete = []
    to_insert = []

    for indicator_id_old in indicator_ids_already_present:
        if indicator_id_old not in indicator_ids_new:
            to_delete.append(indicator_id_old)

    for indicator_id in indicator_ids_new:
        if indicator_id not in indicator_ids_already_present:
            to_insert.append(indicator_id)

    return to_insert, to_delete


# create the parser
parser = argparse.ArgumentParser()
parser.add_argument('-c', dest='configpath', help="indStats configfile path", type=str, required=True)

args = parser.parse_args()

configpath = args.configpath
shc = tp = pred_dur = ilist = start_time = end_time = mode = None
basepx = "OfflineMixMMS"
futpx= "OfflineMixMMS"
filter_name = "fsg.5"
pred_algo = "na_e3"

current_instruction = None
try:
    with open(configpath, 'r') as cfile:
        for line in cfile:
            if not line.strip():
                current_instruction = None
                continue
            line = line.strip()
            if line[0] == '#':
                continue
            if current_instruction is None:
                current_instruction = line
            else:
                if current_instruction == "SHORTCODE":
                    shc = line
                if current_instruction == "TIMEPERIOD":
                    tp = line
                if current_instruction == "PRED_DURATION":
                    pred_dur = line
                if current_instruction == "PRED_ALGO":
                    pred_algo = line
                if current_instruction == "INDICATOR_LIST_FILENAME":
                    ilist = line
                if current_instruction == "START_TIME":
                    start_time = line
                if current_instruction == "END_TIME":
                    end_time = line
                if current_instruction == "MODE":
                    mode = line
                if current_instruction == "BASEPX":
                    basepx = line
                if current_instruction == "FUTPX":
                    futpx = line
                if current_instruction == "FILTER_NAME":
                    filter_name = line
except:
    print(configpath + " not readable")
    raise ValueError(configpath + " not readable.")

if shc is None or mode is None:
    print("shortcode and mode required to proceed further.")
    sys.exit()

# Create and instance of IndStatsDBAcessManager to query the database
obj = IndStatsDBAcessManager()
obj.open_conn()

shc_id = obj.get_shc_id(shc)

# if mode is fetch it just dumps all the indicators for a product to the ilist file name provided
if mode == "FETCH":
    if ilist is None or tp is None or pred_dur is None or start_time is None or end_time is None:
        print("For mode " + mode +" ilist, tp, pred_dur, start_time, end_time are required")
        sys.exit()
    # get the ids for each, it would be required for further querying
    tp_id = str(obj.get_tp_id(tp))
    predalgo_id = str(obj.get_predalgo_id(pred_algo))
    filter_id = str(obj.get_filter_id(filter_name))
    basepx_id = str(obj.get_price_id(basepx))
    futpx_id = str(obj.get_price_id(futpx))

    start_end_time = start_time + "-" + end_time
    indicators_list = obj.get_indicators_for_product(
        shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id, start_end_time)
    write_ilist(ilist, shc, indicators_list)

# if mode is FETCH_TP it will print all the timeperiod for the shortcode
elif mode == "FETCH_TP":
    tp_list = obj.get_tp_for_shc_id(shc_id)
    if tp_list == []:
        print("No Time period Found")
    else:
        for tp in tp_list:
            print(tp + "\n")

# if mode is FETCH_for_shc it just dumps all the indicators for all the products for a shortcode in separate files
elif mode == "FETCH_for_shc":
    if ilist == None:
        print("Ilist required for mode " + mode)
        sys.exit()
    product_list = obj.get_product_for_shc_id(shc_id)
    for product in product_list:
        shc_id = (product[0])
        tp_id = (product[1])
        pred_dur = (product[2])
        predalgo_id = (product[3])
        filter_id = (product[4])
        basepx_id = (product[5])
        futpx_id = (product[6])

        start_end_time_list = obj.get_start_end_times_for_product(shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id,
                                                                  futpx_id)
        for start_end_time in start_end_time_list:
            indicators_list = obj.get_indicators_for_product(shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id,
                                                             futpx_id, start_end_time)
            ilist_file = ilist + "_" + obj.get_tp(tp_id) + "_" + pred_dur + "_" + obj.get_predalgo(predalgo_id) + "_" + \
                obj.get_filter(filter_id) + "_" + obj.get_price(basepx_id) + \
                "_" + obj.get_price(futpx_id) + "_" + start_end_time

            write_ilist(ilist_file, shc, indicators_list)

# if mode is FETCH_for_shc_tp it just dumps all the indicators for all the products for a shortcode and tp in separate files
elif mode == "FETCH_for_shc_tp":
    if ilist is None or tp is None:
        print("Both ilist and tp are required for mode " + mode)
        sys.exit()
    tp_id = obj.get_tp_id(tp)
    product_list = obj.get_products_for_shc_tp(shc_id, tp_id)
    for product in product_list:
        shc_id = (product[0])
        tp_id = (product[1])
        pred_dur = (product[2])
        predalgo_id = (product[3])
        filter_id = (product[4])
        basepx_id = (product[5])
        futpx_id = (product[6])

        start_end_time_list = obj.get_start_end_times_for_product(shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id,
                                                                  futpx_id)
        for start_end_time in start_end_time_list:
            indicators_list = obj.get_indicators_for_product(shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id,
                                                             futpx_id, start_end_time)
            ilist_file = ilist + "_" + tp + "_" + pred_dur + "_" + obj.get_predalgo(predalgo_id) + "_" + \
                obj.get_filter(filter_id) + "_" + obj.get_price(basepx_id) + \
                "_" + obj.get_price(futpx_id) + "_" + start_end_time

            write_ilist(ilist_file, shc, indicators_list)

# else if mode if update, it will update the DB to have only the indicators given in the new ilist in the DB
elif mode == "UPDATE":
    if tp is None or pred_dur is None or ilist is None or start_time is None or end_time is None:
        print("For mode " + mode + " tp, pred_dur, ilist, start_time, end_time are required")
        sys.exit()
    start_end_time = start_time + "-" + end_time
    # get the ids for each, it would be required for further querying
    tp_id = obj.get_tp_id(tp)
    predalgo_id = obj.get_predalgo_id(pred_algo)
    filter_id = obj.get_filter_id(filter_name)
    basepx_id = obj.get_price_id(basepx)
    futpx_id = obj.get_price_id(futpx)

    # if any of the product defintion is not in the DB insert it
    if shc_id == -1:
        obj.insert_value(shc, "shortcode")
        shc_id = obj.get_shc_id(shc)
    if tp_id == -1:
        obj.insert_value(tp, "timeperiod")
        tp_id = obj.get_tp_id(tp)
    if filter_id == -1:
        obj.insert_value(filter_name, "filter")
        filter_id = obj.get_filter_id(filter_name)
    if predalgo_id == -1:
        obj.insert_value(pred_algo, "predalgo")
        predalgo_id = obj.get_predalgo_id(pred_algo)
    if basepx_id == -1:
        obj.insert_value(basepx, "pricetype")
        basepx_id = obj.get_price_id(basepx)
    if futpx_id == -1:
        obj.insert_value(futpx, "pricetype")
        futpx_id = obj.get_price_id(futpx)

    # gets the prodcut for shc and tp
    products_for_shc_tp = obj.get_products_for_shc_tp(shc_id, tp_id)

    # we cannot allow more than 5 products for shc and tp, so the checks; if product is already present we let it continue
    if len(products_for_shc_tp) >= 5 and (shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id) not in products_for_shc_tp:
        print("5 pairs already present,use an existing pairs " + "\n")
        for prod in products_for_shc_tp:
            print(prod + "\n")
        sys.exit()

    # parse the ilist
    indicators_list = get_indicators_from_ilist(ilist)
    # insert the indicators to the DB
    obj.insert_indicators_multiple(indicators_list)
    # get the indicator ids for the indicators
    indicator_ids_list = obj.get_indicator_id_multiple(indicators_list)

    # get the indicator ids already present for the product
    indicators_already_present_for_product = obj.get_indicators_for_product(
        shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id, start_end_time)
    indicator_ids_already_present_for_product = obj.get_indicator_id_multiple(indicators_already_present_for_product)

    # get the indicator ids to insert and delete
    indicator_ids_to_insert, indicator_ids_to_delete = get_indicators_to_delete_and_insert(
        indicator_ids_already_present_for_product, indicator_ids_list)

    prodindicators_already_present_for_shc_tp = obj.get_num_of_prodindicators_for_shc_tp(shc_id, tp_id)
    # if number of indicators is greater than 1000 we exit
    number_of_indicators = prodindicators_already_present_for_shc_tp - \
        len(indicator_ids_to_delete) + len(indicator_ids_to_insert)
    if number_of_indicators > 3000:
        print("number of indicators cannot be greater than 3000 for  shc and timeperiod")
        sys.exit()

    # delete and insert the indicator ids
    if indicator_ids_to_delete != []:
        obj.delete_prod_indicators_multiple(shc_id, tp_id, pred_dur, predalgo_id,
                                            filter_id, basepx_id, futpx_id, indicator_ids_to_delete)
    if indicator_ids_to_insert != []:
        obj.insert_prod_indicators_multiple(shc_id, tp_id, pred_dur, predalgo_id,
                                            filter_id, basepx_id, futpx_id, indicator_ids_to_insert)

    # get the prod_indc_ids for product and indicators and insert the datagen timings for them
    prod_indc_ids_list = obj.get_prod_indc_ids_for_product_and_indicator(
        shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id, indicator_ids_list)
    obj.insert_timings_for_prod_indc_id_multiple(prod_indc_ids_list, start_end_time)

# close the connection
obj.close_conn()
