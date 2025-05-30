import argparse
import time
import sys
# from tendo import singleton
from dateutil.rrule import DAILY, rrule, MO, TU, WE, TH, FR
from datetime import date,timedelta,datetime
import datetime as dt
import pickle
import pandas as pd
from pandas.tseries.offsets import BDay
import matplotlib.pyplot as plt
import numpy as np

from OrderManager import OrderManager
from PnlHandler import PnlHandler
from BarGenerator import BarGenerator
from StratFramework import StratFramework
from MomentumTheo import MomentumTheo
from GapTheo import GapTheo
from SectorMomentumTheo import SectorMomentumTheo
from MACDTheo import MACDTheo
from MRTheo import MRTheo
from MultipleMATheo import MultipleMATheo

#convert given date, time to epoch
def convertDateTimeToEpoch(_date,_time,_timezone):
	datetime_ = _date + " " + _time
	pattern = '%Y_%m_%d %H_%M'
	epoch = int(time.mktime(time.strptime(datetime_, pattern)))
	if _timezone == "BRZ":
		epoch = epoch + 27000
	return epoch


#get all trading days from start date to end date
def dateRange(_start_day,_end_day):
    start_ = _start_day.split('_')
    end_ = _end_day.split('_')
    start_ = date(int(start_[0]),int(start_[1]),int(start_[2]))
    end_ = date(int(end_[0]),int(end_[1]),int(end_[2]))
    return rrule(DAILY, dtstart=start_, until=end_, byweekday=(MO,TU,WE,TH,FR))

home_path = "/home/nishitbhandari/Desktop/MFT/"
slippage_file = home_path + "Transaction_Cost.csv"


parser = argparse.ArgumentParser()
# parser.add_argument('all_prod_file', help='List of products to trade on')
parser.add_argument('config_file', help='config file to load params')
parser.add_argument('strat_start_date', help='Starting date on which results are to be calculated')
parser.add_argument('strat_end_date', help='Ending date on which results are to be calculated')
parser.add_argument('--start_time', help='Start Time of strategy')
parser.add_argument('--end_time', help='End time of strategy')
parser.add_argument('--num_days', help='Training start time of strategy')
# parser.add_argument('train_end_date', help='Training end time of strategy')

args = parser.parse_args()

# if args.all_prod_file:
#     all_prod_file = args.all_prod_file
# else:
#     sys.exit('Please provide exhaustive products file')

if args.config_file:
    config_file = args.config_file
else:
    sys.exit('Please provide config file')


if args.strat_start_date:
    strat_start_date = args.strat_start_date
else:
    sys.exit('Please provide strat_start_date')

if args.strat_end_date:
    strat_end_date = args.strat_end_date
else:
    sys.exit('Please provide strat_end_date')


if args.start_time:
    start_time = args.start_time

if args.end_time:
    end_time = args.end_time

if args.num_days:
    num_days = int(args.num_days)

granularity_ = 0

with open(home_path+config_file, "r") as main_file:
    for line in main_file:
        row = str.split(line)
        if row[0] == "PROD_FILE":
            all_prod_file = home_path + row[2]
        if row[0] == "START_TIME":
            start_time = row[2]
        if row[0] == "END_TIME":
            end_time = row[2]
        if row[0] == "TRAIN_START_TIME":
            train_start_time_ = row[2]
        if row[0] == "TRAIN_END_TIME":
            train_end_time_ = row[2]
        if row[0] == "NUM_DAYS":
            num_days = int(row[2])
        if row[0] == "GRANULARITY":
            granularity_ = 60*int(row[2])
        if row[0] == "MOMENTUM":
            momentum_config_ = home_path + row[2]
        if row[0] == "MACD":
            macd_config_ = home_path + row[2]
        if row[0] == "PKL_PATH":
            pkl_path_ = row[2]
timezone_ = "IND"

print(all_prod_file,start_time,end_time,num_days,granularity_,momentum_config_,macd_config_)
start_ = strat_start_date.split('_')
start_ = date(int(start_[0]),int(start_[1]),int(start_[2]))
train_start_date = datetime.combine(start_,dt.time()) - BDay(num_days )
train_start_date = train_start_date.strftime('%Y_%m_%d')


t0 = time.time()
with open(all_prod_file, "r") as input_file:
    product_list_ = (input_file.readlines())
product_list_ = [x.strip() for x in product_list_]

epoch_start_ = convertDateTimeToEpoch(strat_start_date,start_time,timezone_)
bar_start_ = convertDateTimeToEpoch(train_start_date,start_time,timezone_)
bar_end_ = convertDateTimeToEpoch(strat_end_date,end_time,timezone_)
epoch_end_ = convertDateTimeToEpoch(strat_start_date,end_time,timezone_)
bar_gen_ = BarGenerator(product_list_,bar_start_-180,bar_end_)
om_ = OrderManager(all_prod_file,slippage_file,epoch_start_, epoch_end_)
#strat_ = MomentumTheo(om_,all_prod_file,granularity_, train_start_date,strat_start_date,momentum_config_)
# strat_ = GapTheo(om_,all_prod_file,granularity_, train_start_date,strat_start_date,momentum_config_)
# strat_ = SectorMomentumTheo(om_,all_prod_file,granularity_, train_start_date,strat_start_date,momentum_config_)
strat_ = MultipleMATheo(om_,all_prod_file,granularity_, train_start_date,strat_start_date,momentum_config_)
# strat_ = MomentumVolTheo(om_,all_prod_file,granularity_, train_start_date,strat_start_date,momentum_config_)
# strat_ = MACDTheo(om_,all_prod_file,granularity_, train_start_date,strat_start_date,macd_config_)
# strat_ = MACDNextTheo(om_,all_prod_file,granularity_, train_start_date,strat_start_date,macd_config_)
# strat_ = MRTheo(om_,all_prod_file,granularity_, train_start_date,strat_start_date,macd_config_)
# strat_ = MRRSITheo(om_,all_prod_file,granularity_, train_start_date,strat_start_date,macd_config_)

bar_gen_.started_at_ = epoch_start_
bar_gen_.ended_at_ = epoch_end_
bar_gen_.addStrat(strat_)
# bar_gen_.addStrat(strat_2_)
bar_gen_.initAgg()
strat_.loadBar(bar_gen_.trainingData)
# om_.loadBar(bar_start_-180,bar_end_)
t1= time.time()
# print("loading " + str(t1-t0))

strat_dates_ = dateRange(strat_start_date,strat_end_date)

t1 = time.time()
#     print(date_)
print("loadtime",t1-t0)

for date_ in strat_dates_:
    epoch_start_ = convertDateTimeToEpoch(date_.strftime('%Y_%m_%d'),start_time,timezone_)
    epoch_end_ = convertDateTimeToEpoch(date_.strftime('%Y_%m_%d'),end_time,timezone_)
    bar_gen_.started_at_ = epoch_start_
    bar_gen_.ended_at_ = epoch_end_
    om_.started_at_ = epoch_start_
    om_.ended_at_ = epoch_end_
    # t0 = time.time()
    bar_gen_.beginTrading()
    bar_gen_.initAgg()
    # t1= time.time()
    # print(date_)
#     om_.reset()
t2 = time.time()
print("trading_end " + str(t2-t1)) 
del bar_gen_.content_
# pickle_out = open(pkl_path_,"wb")
# for strategy_ in bar_gen_.strat_list_:
#     pickle.dump(strategy_.pos_pnl_df_,pickle_out)
# pickle_out.close()

t3= time.time()
# print("corr" + str(t3-t2))
# print("hi2")
del bar_gen_
del om_
del strat_
print("end")
# pnl_handler_ = PnlHandler(all_prod_file,pkl_path_,strat_dates_)
