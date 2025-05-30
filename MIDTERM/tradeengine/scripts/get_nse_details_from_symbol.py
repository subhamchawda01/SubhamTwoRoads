import argparse
import sys
import pandas as pd
import datetime as dt


def load_bhavcopy(date_=None):
    if date_ is None:
        date_ = dt.datetime.strftime(dt.datetime.now(), '%Y%m%d')
    dt_string = dt.datetime.strftime(dt.datetime.strptime(date_, '%Y%m%d'), '%d%b%Y').upper()
    bhav_copy_filename_ = '/spare/local/tradeinfo/NSE_Files/BhavCopy/fo/' + \
        date_[4:6] + date_[2:4] + '/' + 'fo' + dt_string + 'bhav.csv'
    bhav_copy_df_ = pd.read_csv(bhav_copy_filename_, delimiter=',')
    bhav_copy_df_ = bhav_copy_df_.applymap(lambda y: y.strip() if type(y) == str else y)
    bhav_copy_df_  = bhav_copy_df_[(bhav_copy_df_['INSTRUMENT'] == 'FUTSTK') | (bhav_copy_df_['INSTRUMENT'] == 'FUTIDX')]
    bhav_copy_df_['EXPIRY_DT'] = bhav_copy_df_['EXPIRY_DT'].apply(lambda y: dt.datetime.strftime(dt.datetime.strptime(y.upper(), '%d-%b-%Y'), '%Y%m%d'))
    bhav_copy_df_['SYMBOL'] = "NSE_" + bhav_copy_df_['SYMBOL'] + "_FUT_" +  bhav_copy_df_['EXPIRY_DT']
    bhav_copy_df_ = bhav_copy_df_[['SYMBOL','EXPIRY_DT','SETTLE_PR']]
    bhav_copy_df_.columns = ["DataSourceName", "Expiry","Settlement Price"]
    bhav_copy_df_.set_index('DataSourceName', inplace=True)
    return bhav_copy_df_


parser = argparse.ArgumentParser()
parser.add_argument('date', help='Date for which settlement price needs to be calculated')
parser.add_argument('symbol_list_file', help='File containing list of symbols')
parser.add_argument('output_file', help='File containing settlement price of symbols')

product_list_file = "/home/dvctrader/usarraf/list_products"

args = parser.parse_args()

if args.date:
    date = args.date
else:
    sys.exit('Please provide date')

if args.symbol_list_file:
    symbol_list_file = args.symbol_list_file
else:
    sys.exit('Please provide symbol list file')

if args.output_file:
    output_file_ = args.output_file
else:
    sys.exit('Please provide output file name')

datasource_file_ = "/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
exch_sym_df_ = pd.read_csv(datasource_file_, names=['Sym','DataSourceName'],delim_whitespace=True)

with open(symbol_list_file) as f:
    symbol_list_ = f.read().splitlines()

exch_sym_df = exch_sym_df_[exch_sym_df_['Sym'].isin(symbol_list_)]
exch_sym_df.set_index('DataSourceName', inplace=True)
settle_px_df = load_bhavcopy(date)

exch_sym_df = exch_sym_df.merge(settle_px_df, left_index=True, right_index=True, how='inner')
exch_sym_df.to_csv(output_file_, header=None, index=None, sep=' ')
