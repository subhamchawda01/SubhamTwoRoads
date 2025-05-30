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
MDS_LOGGER = '/home/pengine/prod/live_execs/mds_fast_first_trade_read'
BHAV_DIR = '/spare/local/tradeinfo/NSE_Files/Margin_Files/Span_Files/'
OUT_FILE = '/home/dvcinfra/important/PreMarketTrades/Product_Ratio/'
LOT_SIZE_DIR = '/spare/local/tradeinfo/NSE_Files/Lotsizes/' 
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
        + ' P W'
    YYYYMMDD = BashExec(command)
    command = \
        '/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE ' \
        + YYYYMMDD + ' T'
    is_holiday_ = BashExec(command)
    while is_holiday_ == '1':
        command = '/home/pengine/prod/live_execs/update_date ' \
            + YYYYMMDD + ' P W'
        YYYYMMDD = BashExec(command)
        command = \
            '/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE ' \
            + YYYYMMDD + ' T'
        is_holiday_ = BashExec(command)
    print 'Previous working Day ' + YYYYMMDD


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
    for line in Lot_File:
        sym = line.split(',')[1].rstrip()
        if sym == 'SYMBOL' or sym == 'BANKNIFTY' \
            or sym == 'NIFTY' or sym == 'NIFTYIT':
            continue
        fo_products.append(sym)
    closing_price = '/tmp/cm_closing_price_' + PDD + PMM + PYYYY
    if os.path.exists(BHAV_DIR + 'cm' + PDD + month_name + PYYYY
                      + 'bhav.csv') == False:
        print 'CM Bhav File Missing for ' + date_ + '!Exiting..'
        sys.exit(0)
    command = 'awk -F\',\' \'$2 == "EQ" {print $1","$6}\' ' + BHAV_DIR \
        + 'cm' + PDD + month_name + PYYYY + 'bhav.csv >' + closing_price
    BashExec(command)
    file_closing_price = open(closing_price, 'r')
    if not os.path.exists(OUT_FILE):
        os.makedirs(OUT_FILE)
    file_output = open(OUT_FILE + date_, 'w')
    for line in file_closing_price:
        sym = line.split(',')[0].rstrip('\n')
        close_price = float(line.split(',')[1].rstrip('\n'))
        if sym not in fo_products:
          continue
        FILE = DATADIR + 'NSE_' + sym + '_' + date_
        if os.path.exists(FILE) == False:
            print 'Skipping. File Not exist!!  ' + FILE
            continue
        command = MDS_LOGGER + ' ' + 'GENERIC'  + ' \"' \
            + FILE + '\" ' + str(pre_market_start) + ' ' + str(pre_market_end) \
            + ' | less | grep -m 1 "TradePrice:" | cut -d\':\' -f2 | awk \'{$1=$1};1\''
        print command
        trade_price = BashExec(command)
        if trade_price == 0 or  trade_price == '':
            print 'No trade Found For ' + sym
            continue
        ratio_trade_closing = int(trade_price) / close_price
        ratio_trade_closing = ratio_trade_closing - 100
        file_output.write(sym + ' ' + str(ratio_trade_closing) + ' '
                           + str(int(trade_price) / 100) + ' '
                          + str(close_price) + '\n')
    file_closing_price.close()
    file_output.close()


if __name__ == '__main__':
    main()
