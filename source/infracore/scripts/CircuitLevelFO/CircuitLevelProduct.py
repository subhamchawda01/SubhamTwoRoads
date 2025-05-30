#!/usr/bin/python
# - * - coding : utf - 8 - * -

import datetime as dt
from zipfile import ZipFile
import subprocess
import os
import calendar
import sys
import time

DATADIR = '/spare/local/MDSlogs/GENERIC/'
LD_PRELOAD = '/home/dvcinfra/important/libcrypto.so.1.1'
MDS_LOGGER = '/home/pengine/prod/live_execs/mds_circuit_alert_read'
BHAV_DIR = '/spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files/'
LOT_SIZE_DIR = '/spare/local/tradeinfo/NSE_Files/Lotsizes/' 
MDS_INPUT_FILE = '/tmp/mds_fast_reader_input_file_circuit'
MDS_OUTPUT_FILE = '/tmp/mds_fast_out_circuit'
os.environ['LD_PRELOAD'] = LD_PRELOAD

def BashExec(command):
    try:
        return subprocess.check_output(['bash', '-c',
                command]).decode('UTF-8').strip()
    except:
        return 0


def GetPreviousWorkingDay(date_):
    global YYYYMMDD
    command = '/home/pengine/prod/live_execs/update_date ' + date_ \
        + ' P A'
    YYYYMMDD = BashExec(command)
    command = \
        '/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE ' \
        + YYYYMMDD + ' T'
    is_holiday_ = BashExec(command)
    while is_holiday_ == '1':
        command = '/home/pengine/prod/live_execs/update_date ' \
            + YYYYMMDD + ' P A'
        YYYYMMDD = BashExec(command)
        command = \
            '/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE ' \
            + YYYYMMDD + ' T'
        is_holiday_ = BashExec(command)


def Exec_mds_fast_logger(pre_market_start, pre_market_end):
    command = MDS_LOGGER + ' ' + 'GENERIC'  + ' \"' \
                      + MDS_INPUT_FILE + '\" ' \
            + ' | less  > ' + MDS_OUTPUT_FILE
    BashExec(command)

def Dump_data_output(date_,sym_close_dic):
    file_read_mds_out = open(MDS_OUTPUT_FILE ,'r')
    print ('\nSymbol\t\t\t\t\tTradePrice\t\t\t\tClosingPrice\t\t\t\t Movement\n')
    for line in file_read_mds_out:
        words = line.split(' ')
        if len(words) != 5:
            print "count does not match no trade format"
            continue
        trade_price = words[2]
        c_price = words[1]
        mvt = float(words[3])
        sym = words[0].split('_')[1]
        if mvt > 7.5 or mvt < -7.5:
            print ( sym + '\t\t\t\t\t' + trade_price + '\t\t\t\t\t' + c_price + '\t\t\t\t\t' + str(mvt))
    print ("\n")

def main():
    if len(sys.argv) < 2:
        date_ = dt.datetime.today().strftime('%Y%m%d')
    else:
        date_ = str(sys.argv[1])
    GetPreviousWorkingDay(date_)
    PYYYY = YYYYMMDD[0:4]
    PMM = YYYYMMDD[4:6]
    PDD = YYYYMMDD[6:8]
    CYYYY = date_[0:4]
    CMM = date_[4:6]
    CDD = date_[6:8]
    month_name = calendar.month_abbr[int(PMM)].upper()
    pre_market_start_date = dt.datetime(int(CYYYY), int(CMM), int(CDD), 3, 30)
    pre_market_end_date = dt.datetime(int(CYYYY), int(CMM), int(CDD), 3, 45)
    pre_market_start = (time.mktime(pre_market_start_date.timetuple()))
    pre_market_end = (time.mktime(pre_market_end_date.timetuple()))
    Lot_File = open(LOT_SIZE_DIR + "fo_mktlots_" + date_ + ".csv", 'r')
    fo_products = []
    sym_close_dic = {}
    command='/home/pengine/prod/live_execs/combined_user_msg --dump_mds_files --only_ors_files 0'
    BashExec(command)
    time.sleep( 10 )
    for line in Lot_File:
        sym = line.split(',')[1].rstrip()
        if sym == 'SYMBOL' or sym == 'BANKNIFTY' \
            or sym == 'NIFTY' or sym == 'NIFTYIT':
            continue
        fo_products.append(sym)
    closing_price = '/tmp/cm_closing_price_' + PDD + PMM + PYYYY
    if os.path.exists(BHAV_DIR + 'cm' + PDD + month_name + PYYYY
                      + 'bhav.csv') == False:
        print 'CM Bhav File Missing for ' + date_ + '!..'
        command = 'echo "" | mailx -s "Contract File missing For"' + PDD + month_name + ' -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in'
        BashExec(command)
        exit(0)
    command = 'awk -F\',\' \'$2 == "EQ" {print $1","$6}\' ' + BHAV_DIR \
        + 'cm' + PDD + month_name + PYYYY + 'bhav.csv >' + closing_price
    BashExec(command)
    file_closing_price = open(closing_price, 'r')
    file_input_mds = open(MDS_INPUT_FILE,'w')
    for line in file_closing_price:
        words = line.split(',')
        sym = words[0]
        sym_close_dic[sym] = float(words[1])
        if sym not in fo_products:
          continue
        FILE = DATADIR + 'NSE_' + sym + '_' + date_
        if os.path.exists(FILE) == False:
            print 'Skipping. File Not exist!!  ' + FILE
            continue
        file_input_mds.write( FILE + ' ' + words[1] )
    file_input_mds.close()
    file_closing_price.close()
    #Run MDS_LOGGER
    Exec_mds_fast_logger(pre_market_start,pre_market_end)
    #DUmp data to Output file
    BashExec ("sort -rk4 -n " + MDS_OUTPUT_FILE + " > /tmp/fsorted")
    BashExec ("cp /tmp/fsorted " + MDS_OUTPUT_FILE)
    Dump_data_output(date_,sym_close_dic)

if __name__ == '__main__':
    main()
