#!/usr/bin/env python

import os
import subprocess
import random
from walkforward.definitions import execs

"""
Usage: /home/dvctrader/basetrade_install/bin/timed_data_to_reg_data MODELFILENAME INPUTDATAFILENAME MSECS/EVENTS_TO_PREDICT NORMALIZING_ALGO OUTPUTDATAFILENAME <FILTER>? [PRINT_TIME = 0] <fsudm_level>
143
kDatToRegCommandLineLessArgs
"""



def convert_t2d(shortcode, date, modelfile, tdata_file, pred_counters, pred_algo, rdata_file, regdata_process_filter):
    if pred_algo == "na_t3_bd":

        min_price_increment = get_min_price_increment(shortcode,date)

        rdata_command = [execs.execs().timed_data_to_reg_data_bd, tdata_file, rdata_file, pred_counters, "1000",
                         min_price_increment]
        # print (rdata_command)
        process = subprocess.Popen(' '.join(rdata_command), shell=True,
                                   stderr=subprocess.PIPE,
                                   stdout=subprocess.PIPE)
        out, err = process.communicate()
        if out is not None:
            out = out.decode('utf-8')
        if err is not None:
            err = err.decode('utf-8')

        errcode = process.returncode

        return out, err, errcode
    else:
        trade_per_sec_file = "INVALIDFILE"
        fsudm_level = 0

        if regdata_process_filter == "fv":
            trade_per_sec_file = os.path.dirname(tdata_file) + "/" + str(
                random.randint(0, 10000)) + "_" + shortcode + "_trd_per_sec"
            daily_trade_aggregator_cmd = [execs.execs().daily_trade_aggregator, shortcode, date, trade_per_sec_file]
            process = subprocess.Popen(' '.join(daily_trade_aggregator_cmd), shell=True,
                                       stderr=subprocess.PIPE,
                                       stdout=subprocess.PIPE)
            out, err = process.communicate()
            if out is not None:
                out = out.decode('utf-8')
            if err is not None:
                err = err.decode('utf-8')

            errcode = process.returncode
        elif regdata_process_filter == "fsudm1" or regdata_process_filter == "fsudm":
            fsudm_level = 1
        elif regdata_process_filter == "fsudm2":
            fsudm_level = 2
        elif regdata_process_filter == "fsudm3":
            fsudm_level = 3

        rdata_command = [execs.execs().timed_data_to_reg_data, modelfile, tdata_file, pred_counters, pred_algo,
                         rdata_file, trade_per_sec_file, "0", str(fsudm_level)]
        # print (rdata_command)
        process = subprocess.Popen(' '.join(rdata_command), shell=True,
                                   stderr=subprocess.PIPE,
                                   stdout=subprocess.PIPE)
        out, err = process.communicate()
        if out is not None:
            out = out.decode('utf-8')
        if err is not None:
            err = err.decode('utf-8')

        errcode = process.returncode

        return out, err, errcode
