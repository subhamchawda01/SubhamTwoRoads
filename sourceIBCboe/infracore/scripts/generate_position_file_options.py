import argparse
import sys
import pandas as pd
import datetime as dt
from dateutil.relativedelta import *
import subprocess
import os
import math


'''
 Here we always pick fut0 lotsize
'''
def get_lot_size(filename_):
    lot_size_df = pd.read_csv(filename_, delimiter=',', names=None, dtype=object);
    lot_size_df = lot_size_df.iloc[:, [1,2]];
    lot_size_df.columns = ['Stock', 'Lotsize']
    lot_size_df['Stock'] = lot_size_df['Stock'].str.strip();
    lot_size_df['Lotsize'] = lot_size_df['Lotsize'].str.strip();
    lot_size_df.set_index('Stock', inplace=True)
    return lot_size_df;

def get_position_limits(row,base_num_spreads_factor,physical_factor,banned_products,physical_settlement_products):
    if row['Stock'] in banned_products:
        base_num_spreads = 0
    elif row['Stock'] in physical_settlement_products:
        base_num_spreads = physical_factor*base_num_spreads_factor
    else:
        base_num_spreads = base_num_spreads_factor
    
    max_long_pos = base_num_spreads
    max_short_pos = base_num_spreads
    prefix = "NSE_" + row['Stock']

    max_long_pos = max_long_pos
    max_short_pos = max_short_pos
    max_long_exposure = 2*max_long_pos
    max_short_exposure = 2*max_short_pos

    max_long_pos_fut = max_long_pos*4
    max_short_pos_fut = max_short_pos*4
    max_long_exposure_fut = 2*max_long_pos_fut
    max_short_exposure_fut = 2*max_short_pos_fut
    
    pos_limit_string = prefix + "_FUT0_MAXLONGPOS = " + str(int(max_long_pos_fut)) + "*ONE_LOT_SIZE" +  "\n" + \
                       prefix + "_FUT0_MAXSHORTPOS = " + str(int(max_short_pos_fut)) + "*ONE_LOT_SIZE" + "\n" + \
                       prefix + "_FUT0_MAXLONGEXPOSURE = " + str(int(max_long_exposure_fut)) + "*ONE_LOT_SIZE" + "\n" + \
                       prefix + "_FUT0_MAXSHORTEXPOSURE = " + str(int(max_short_exposure_fut)) + "*ONE_LOT_SIZE" + "\n"

    for opt in ["C0","P0"]:
        for stk in ["A","O1","O2","O3"]:
            prefix = "NSE_" + row['Stock'] + "_" + opt + "_" + stk
            pos_limit_string += prefix + "_MAXLONGPOS = " + str(int(max_long_pos)) + "*ONE_LOT_SIZE" + "\n" + \
                                prefix + "_MAXSHORTPOS = " + str(int(max_short_pos)) + "*ONE_LOT_SIZE" + "\n" + \
                                prefix + "_MAXLONGEXPOSURE = " + str(int(max_long_exposure)) + "*ONE_LOT_SIZE" + "\n" + \
                                prefix + "_MAXSHORTEXPOSURE = " + str(int(max_short_exposure)) + "*ONE_LOT_SIZE" + "\n"



    return pos_limit_string

base_num_spreads_factor_ = 1

parser = argparse.ArgumentParser()
parser.add_argument('date', help='Date for which position file needs to be generated')
parser.add_argument('--base_num_spreads_factor', help='Factor for default number of spreads allowed')
parser.add_argument('--physical_factor', help='Factor for physical derivatives')

product_list_file = "/home/dvctrader/usarraf/list_products"
expiry_file = '/spare/local/files/ExpiryDates/monthly'

args = parser.parse_args()

if args.date:
    date = args.date
else:
    sys.exit('Please provide date')

if args.base_num_spreads_factor:
    base_num_spreads_factor_ = float(args.base_num_spreads_factor)

if args.physical_factor:
    physical_factor_ = float(args.physical_factor)


next_working_date_ = subprocess.Popen(["/home/pengine/prod/live_execs/update_date" , date, "N" ,"W"], stdout=subprocess.PIPE).communicate()[0]
next_working_date_ = next_working_date_.rstrip().decode("utf-8") 

is_holiday = subprocess.Popen(["/home/pengine/prod/live_execs/holiday_manager","EXCHANGE","NSE",next_working_date_,"T"], stdout=subprocess.PIPE).communicate()[0]
is_holiday = is_holiday.rstrip().decode("utf-8")

while (is_holiday == "1") :
    next_working_date_ = subprocess.Popen(["/home/pengine/prod/live_execs/update_date" , next_working_date_, "N" ,"W"], stdout=subprocess.PIPE).communicate()[0]
    next_working_date_ = next_working_date_.rstrip().decode("utf-8")
    is_holiday = subprocess.Popen(["/home/pengine/prod/live_execs/holiday_manager","EXCHANGE","NSE",next_working_date_,"T"], stdout=subprocess.PIPE).communicate()[0]
    is_holiday = is_holiday.rstrip().decode("utf-8")


expiry_date = subprocess.check_output(['bash','-c','cat %s | awk \'$1 > %s\' | head -n1'%(expiry_file, date)]).decode('utf-8').rstrip();

lot_size_filename_ = '/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_' + expiry_date[4:6] + expiry_date[2:4] + '.csv'
output_position_file_ = "/home/dvctrader/usarraf/PositionLimits.Options." + date + ".txt"
banned_prod_file_ = "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/fo_secban_" + next_working_date_ + ".csv"
physical_settlement_prod_file_ = "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderPhysicalSettlement/fo_securities_under_physical_settlement.csv"
banned_products = []
physical_settlement_products = []

with open(product_list_file) as f:
    products = f.read().splitlines()

if os.path.exists(banned_prod_file_):
    with open(banned_prod_file_) as f:
         banned_products = f.read().splitlines()

if os.path.exists(physical_settlement_prod_file_):
    with open(physical_settlement_prod_file_) as f:
        physical_settlement_products = f.read().splitlines()

pos_df = pd.DataFrame(columns=['Stock', 'Pos'])
pos_df['Stock'] = pos_df['Stock'].str.replace('~','&')

for x in products:
     if x not in pos_df['Stock'].tolist():
         pos_df = pos_df.append({'Stock':x, 'Pos':0}, ignore_index=True)

pos_df.set_index('Stock', inplace=True)
lot_size_df = get_lot_size(lot_size_filename_);

pos_df = pos_df.merge(lot_size_df, left_index=True, right_index=True, how='inner')
pos_df = pos_df[pos_df['Lotsize'] !='']
list_pos = [item for x in pos_df.reset_index().apply(get_position_limits, args=(base_num_spreads_factor_,physical_factor_,banned_products,physical_settlement_products),axis=1) for item in (x.strip().split('\n'))]
pos_limit_df_ = pd.DataFrame([x.split(' ') for x in list_pos], columns=['Field', 'Sign', 'Value'])
pos_limit_df_.loc[[i in banned_products for i in pos_limit_df_.Field.str.split('_').str[1]], 'Value'] = 0

risk_check_ = []
risk_check_.insert(0, {'Field': 'GROSS_EXPOSURE_LIMIT', 'Sign': '=', 'Value': '80'})
risk_check_.insert(0, {'Field': 'TOTAL_PORTFOLIO_STOPLOSS', 'Sign': '=', 'Value': '100000'})

pos_limit_df_ = pd.concat([pd.DataFrame(risk_check_), pos_limit_df_], ignore_index=True)
pos_limit_df_.to_csv(output_position_file_, header=None, index=None, sep=' ')

