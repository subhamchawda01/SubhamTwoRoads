import argparse
import sys
import pandas as pd
import datetime as dt
from dateutil.relativedelta import *
import subprocess
import os
import math

'''
 take fut1 lot size, for expiry day fut2
'''
def get_lot_size(filename_, exp_num):
    lot_size_df = pd.read_csv(filename_, delimiter=',', names=None, dtype=object);
    lot_size_df = lot_size_df.iloc[:, [1,exp_num + 2 ,exp_num + 3]];
    lot_size_df.columns = ['Stock', 'Lotsize0', 'Lotsize']
    lot_size_df['Stock'] = lot_size_df['Stock'].str.strip();
    lot_size_df['Lotsize'] = lot_size_df['Lotsize'].str.strip();
    lot_size_df['Lotsize0'] = lot_size_df['Lotsize0'].str.strip();
    lot_size_df.set_index('Stock', inplace=True)
    return lot_size_df;

def get_position_limits(row,base_num_spreads_factor,physical_factor,max_num_spreads_diff,banned_products,physical_settlement_products):
    if row['Stock'] in banned_products:
        base_num_spreads = 0
        max_num_spreads = 0
    else:
        if row['Stock'] in physical_settlement_products:
            base_num_spreads = math.ceil(int(base_num_spreads_factor*row['NumSpread'])*physical_factor)
            max_num_spreads = int(base_num_spreads + max_num_spreads_diff)
            #max_num_spreads = int(base_num_spreads)
        else:
            base_num_spreads = int(base_num_spreads_factor*row['NumSpread'])
            max_num_spreads = int(base_num_spreads + max_num_spreads_diff)
    
    max_long_pos = base_num_spreads
    max_short_pos = base_num_spreads
    prefix = "NSE_" + row['Stock']

    if row['Num_Lots'] < 0:
        max_long_pos = base_num_spreads-row['Num_Lots']
        max_short_pos = min(max_num_spreads+row['Num_Lots'],base_num_spreads)
    else:
        max_long_pos = min(max_num_spreads-row['Num_Lots'],base_num_spreads)
        max_short_pos = base_num_spreads + row['Num_Lots']

    max_long_pos = max_long_pos
    max_short_pos = max_short_pos
    max_long_exposure = max_long_pos + 2
    max_short_exposure = max_short_pos + 2
    pos_limit_string = prefix + "_FUT" + str(curr_exp_num + 1) + "_MAXLONGPOS = " + str(int(max_long_pos)) + "*ONE_LOT_SIZE" + "\n" + \
                       prefix + "_FUT" + str(curr_exp_num + 1) + "_MAXSHORTPOS = " + str(int(max_short_pos)) +  "*ONE_LOT_SIZE" + "\n" + \
                       prefix + "_FUT" + str(curr_exp_num + 1) + "_MAXLONGEXPOSURE = " + str(int(max_long_exposure)) + "*ONE_LOT_SIZE"  + "\n" + \
                       prefix + "_FUT" + str(curr_exp_num + 1) + "_MAXSHORTEXPOSURE = " + str(int(max_short_exposure)) +  "*ONE_LOT_SIZE" + "\n" + \
                       prefix + "_FUT" + str(curr_exp_num) + "_MAXLONGPOS = " + str(int(max_short_exposure)) +  "*ONE_LOT_SIZE"  + "\n" + \
                       prefix + "_FUT" + str(curr_exp_num) + "_MAXSHORTPOS = " + str(int(max_long_exposure)) +  "*ONE_LOT_SIZE"  +"\n" + \
                       prefix + "_FUT" + str(curr_exp_num) + "_MAXLONGEXPOSURE = " + str(int(max_short_exposure) + 1) +  "*ONE_LOT_SIZE" + "\n" + \
                       prefix + "_FUT" + str(curr_exp_num) + "_MAXSHORTEXPOSURE = " + str(int(max_long_exposure) + 1) + "*ONE_LOT_SIZE"  + "\n"
    return pos_limit_string
def get_agg_position_limits(row,base_num_spreads_factor,physical_factor,aggressive_factor,yesterday_ban_factor,max_num_spreads_diff,banned_products,yesterday_banned_products,physical_settlement_products, exp_num):
    if row['Stock'] in banned_products:
        base_num_spreads = 0
        max_num_spreads = 0
    else:
        if row['Stock'] in physical_settlement_products:
            base_num_spreads = math.ceil(int(base_num_spreads_factor*row['NumSpread'])*physical_factor)
            max_num_spreads = int(base_num_spreads + max_num_spreads_diff)
            #max_num_spreads = int(base_num_spreads)
        else:
            base_num_spreads = int(base_num_spreads_factor*row['NumSpread'])
            max_num_spreads = int(base_num_spreads + max_num_spreads_diff)
    if row['Stock'] in yesterday_banned_products:
       base_num_spreads = int(base_num_spreads*yesterday_ban_factor)
       max_num_spreads = base_num_spreads

    max_long_pos = base_num_spreads
    max_short_pos = base_num_spreads
    prefix = "NSE_" + row['Stock']

    if row['Num_Lots'] < 0:
        max_long_pos = base_num_spreads
        max_short_pos = min(max_num_spreads+row['Num_Lots'],base_num_spreads)
    else:
        max_long_pos = min(max_num_spreads-row['Num_Lots'],base_num_spreads)
        max_short_pos = base_num_spreads
    max_long_pos = math.ceil(max_long_pos*aggressive_factor_)
    max_short_pos = math.ceil(max_short_pos*aggressive_factor_)

    max_long_pos = max_long_pos
    max_short_pos = max_short_pos
    max_long_exposure = max_long_pos + 2
    max_short_exposure = max_short_pos + 2
    pos_limit_string = prefix + "_FUT" + str(exp_num + 1) + "_MAXLONGPOS = " + str(int(max_long_pos)) + "*ONE_LOT_SIZE" + "\n" + \
                       prefix + "_FUT" + str(exp_num + 1) + "_MAXSHORTPOS = " + str(int(max_short_pos)) +  "*ONE_LOT_SIZE" + "\n" + \
                       prefix + "_FUT" + str(exp_num + 1) + "_MAXLONGEXPOSURE = " + str(int(max_long_exposure)) + "*ONE_LOT_SIZE"  + "\n" + \
                       prefix + "_FUT" + str(exp_num + 1) + "_MAXSHORTEXPOSURE = " + str(int(max_short_exposure)) +  "*ONE_LOT_SIZE" + "\n" + \
                       prefix + "_FUT" + str(exp_num) + "_MAXLONGPOS = " + str(math.ceil(int(max_short_exposure)*float(row['Lotsize'])/float(row['Lotsize0']))) +  "*ONE_LOT_SIZE"  + "\n" + \
                       prefix + "_FUT" + str(exp_num) + "_MAXSHORTPOS = " + str(math.ceil(int(max_long_exposure)*float(row['Lotsize'])/float(row['Lotsize0']))) +  "*ONE_LOT_SIZE"  +"\n" + \
                       prefix + "_FUT" + str(exp_num) + "_MAXLONGEXPOSURE = " + str(math.ceil(int(max_short_exposure)*float(row['Lotsize'])/float(row['Lotsize0'])) + 1) +  "*ONE_LOT_SIZE" + "\n" + \
                       prefix + "_FUT" + str(exp_num) + "_MAXSHORTEXPOSURE = " + str(math.ceil(int(max_long_exposure)*float(row['Lotsize'])/float(row['Lotsize0'])) + 1) + "*ONE_LOT_SIZE"  + "\n"

    return pos_limit_string


base_num_spreads_factor_ = 1
max_num_spreads_diff_ = 0
physical_factor_ = 1
aggressive_factor_ = 1
yesterday_ban_factor_ = 1

parser = argparse.ArgumentParser()
parser.add_argument('date', help='Date for which position file needs to be generated')
parser.add_argument('--base_num_spreads_factor', help='Factor for default number of spreads allowed')
parser.add_argument('--max_num_spreads_diff', help='Factor for maximum number of spreads allowed')
parser.add_argument('--physical_factor', help='Factor for physical derivatives')
parser.add_argument('--aggressive_factor', help='Factor for physical derivatives')
parser.add_argument('--yesterday_ban_factor', help='Factor for yesterday ban derivatives')

product_list_file = "/home/dvctrader/usarraf/list_products"
expiry_file = '/spare/local/files/ExpiryDates/monthly'

args = parser.parse_args()

if args.date:
    date = args.date
else:
    sys.exit('Please provide date')

if args.base_num_spreads_factor:
    base_num_spreads_factor_ = float(args.base_num_spreads_factor)

if args.max_num_spreads_diff:
    max_num_spreads_diff_ = float(args.max_num_spreads_diff)

if args.physical_factor:
    physical_factor_ = float(args.physical_factor)
if args.aggressive_factor:
    aggressive_factor_ = float(args.aggressive_factor)
if args.yesterday_ban_factor:
    yesterday_ban_factor_ = float(args.yesterday_ban_factor)

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
position_file = "/NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_" + date + ".txt"
output_position_file_ = "/home/dvctrader/usarraf/PositionLimits_agg." + date
banned_prod_file_ = "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/fo_secban_" + next_working_date_ + ".csv"
yesterday_banned_file_ = "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/fo_secban_" +date + ".csv"
num_spread_file_ = "/home/dvctrader/usarraf/spread_products"
physical_settlement_prod_file_ = "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderPhysicalSettlement/fo_securities_under_physical_settlement.csv"
banned_products = []
yesterday_banned_products = []
physical_settlement_products = []

'''
  We'll loading FUT2 positions on the day of expiry, 
  other days FUT1
'''
expiry = subprocess.check_output(['bash','-c','cat %s | awk \'$1 >= %s\' | head -n1'%(expiry_file, date)]).decode('utf-8').rstrip();

overnight_contract = 'FUT2' if date == expiry else 'FUT1'

exp_num = 0 if expiry_date != next_working_date_ else 1

content = [line.strip().split() for line in open(position_file) if overnight_contract in line]
content = [[x[1].split('_')[0], int(x[-4])] for x in content]

#content = []
with open(product_list_file) as f:
    products = f.read().splitlines()

if os.path.exists(banned_prod_file_):
    with open(banned_prod_file_) as f:
         banned_products = f.read().splitlines()
if os.path.exists(yesterday_banned_file_):
    with open(yesterday_banned_file_) as f:
         yesterday_banned_products = f.read().splitlines()
if os.path.exists(physical_settlement_prod_file_):
    with open(physical_settlement_prod_file_) as f:
         physical_settlement_products = f.read().splitlines()

with open(num_spread_file_) as f:
    num_spread_content = f.read().splitlines()
    num_spread_content = [x.split(' ') for x in num_spread_content]

low_physical_ = []
for x in num_spread_content:
    if x[1] == "0":
         #print(x[1])
         low_physical_.append(x[0])
pos_df = pd.DataFrame(content,columns=['Stock', 'Num_Lots'])
#pos_df = pd.DataFrame(columns=['Stock', 'Pos'])
num_spread_df = pd.DataFrame(num_spread_content,columns=['Stock', 'NumSpread'])

pos_df['Stock'] = pos_df['Stock'].str.replace('~','&')
num_spread_df['Stock'] = num_spread_df['Stock'].str.replace('~','&')

for x in products:
     if x not in pos_df['Stock'].tolist():
         pos_df = pos_df.append({'Stock':x, 'Num_Lots':0}, ignore_index=True)

pos_df.set_index('Stock', inplace=True)
num_spread_df.set_index('Stock', inplace=True)
lot_size_df =  get_lot_size(lot_size_filename_, exp_num);

pos_df = pos_df.merge(lot_size_df, left_index=True, right_index=True, how='inner')
pos_df = pos_df.merge(num_spread_df, left_index=True, right_index=True, how='inner')
pos_df = pos_df[pos_df['Lotsize'] !='']
pos_df['NumSpread'] = pos_df['NumSpread'].astype(float)
list_pos = [item for x in pos_df.reset_index().apply(get_agg_position_limits, args=(base_num_spreads_factor_,physical_factor_,aggressive_factor_,yesterday_ban_factor_,max_num_spreads_diff_,banned_products,yesterday_banned_products,physical_settlement_products, exp_num),axis=1) for item in (x.strip().split('\n'))]
pos_limit_df_ = pd.DataFrame([x.split(' ') for x in list_pos], columns=['Field', 'Sign', 'Value'])
pos_limit_df_.loc[[i in banned_products for i in pos_limit_df_.Field.str.split('_').str[1]], 'Value'] = 0
#pos_limit_df_.loc[[i in physical_settlement_products for i in pos_limit_df_.Field.str.split('_').str[1]], 'Value'] = 0
pos_limit_df_.loc[[i in low_physical_ for i in pos_limit_df_.Field.str.split('_').str[1]], 'Value'] = 0

risk_check_ = []
risk_check_.insert(0, {'Field': 'GROSS_EXPOSURE_LIMIT', 'Sign': '=', 'Value': '35'})
risk_check_.insert(0, {'Field': 'TOTAL_PORTFOLIO_STOPLOSS', 'Sign': '=', 'Value': '100000'})

pos_limit_df_ = pd.concat([pd.DataFrame(risk_check_), pos_limit_df_], ignore_index=True)
pos_limit_df_.to_csv(output_position_file_, header=None, index=None, sep=' ')

