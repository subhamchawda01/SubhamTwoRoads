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
MDS_LOGGER = '/home/pengine/prod/live_execs/mds_fast_first_trade_read_new'
BHAV_DIR = '/spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files/'
OUT_FILE = '/home/dvcinfra/important/PreMarketTrades/Product_Ratio/'
LOT_SIZE_DIR = '/spare/local/tradeinfo/NSE_Files/Lotsizes/' 
MDS_INPUT_FILE = '/tmp/mds_fast_reader_input_file'
MDS_OUTPUT_FILE = '/tmp/mds_fast_out'
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
    print 'Previous working Day ' + YYYYMMDD

def Update_html():
    command = '/home/dvcinfra/important/PreMarketTrades/scripts/generate_preMarket_trades_report.sh \
            >>/tmp/Pre_Markets_Update_log 2>&1'
    BashExec(command)

def SyncProduct():
    command = '/home/dvcinfra/important/PreMarketTrades/sync_preMarket_trades.sh'
    BashExec(command)

def Exec_mds_fast_logger(pre_market_start, pre_market_end):
    command = MDS_LOGGER + ' ' + 'GENERIC'  + ' \"' \
                      + MDS_INPUT_FILE + '\" ' + str(pre_market_start) + ' ' + str(pre_market_end) \
            + ' | less  > ' + MDS_OUTPUT_FILE
    BashExec(command)

def Dump_data_output(date_,sym_close_dic):
    if not os.path.exists(OUT_FILE):
        os.makedirs(OUT_FILE)
    file_output = open(OUT_FILE + date_, 'w')
    file_read_mds_out = open(MDS_OUTPUT_FILE ,'r')
    for line in file_read_mds_out:
        words = line.split(' ')
        if len(words) != 8:
            print "count does not match no trade format"
            continue
        trade_price = words[6]
        sym = words[1].split('_')[1]
        ratio_trade_closing = int(trade_price) / sym_close_dic[sym]
        ratio_trade_closing = ratio_trade_closing - 100
        file_output.write(sym + ' ' + str(round(ratio_trade_closing,2)) + ' '
                    + str(int(trade_price) / 100) + ' '
                    + str(sym_close_dic[sym]) + '\n')
    file_output.close()
    file_read_mds_out.close()


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
            or sym == 'NIFTY' or sym == 'FINNIFTY':
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
        file_input_mds.write( FILE + '\n' )
    file_input_mds.close()
    file_closing_price.close()
    #Run MDS_LOGGER
    Exec_mds_fast_logger(pre_market_start,pre_market_end)
    #DUmp data to Output file
    Dump_data_output(date_,sym_close_dic)
    Update_html()
    #sync 
    SyncProduct()

if __name__ == '__main__':
    main()
