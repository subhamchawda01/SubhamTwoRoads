import sys
import argparse
import pandas as pd
import datetime as dt

parser = argparse.ArgumentParser()
parser.add_argument('date', help='Date for which position file needs to be generated')
product_list_file = "/home/dvctrader/usarraf/list_products"
args = parser.parse_args()
if args.date:
    date = args.date
else:
    sys.exit('Please provide date')

position_file = "/NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_" + date + ".txt"
lotsize_file = '/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_' + date[4:6] + date[2:4]+'.csv'
output_position_file_ = "/home/dvctrader/usarraf/LotPositions." + date
with open(product_list_file) as f:
    products = f.read().splitlines()

content = [line.strip().split() for line in open(position_file) if 'FUT1' in line]
content = [[x[1].split('_')[0], int(x[-7])] for x in content]

pos_df = pd.DataFrame(content,columns=['Stock', 'Pos'])
pos_df['Stock'] = pos_df['Stock'].str.replace('~','&')
for x in products:
     if x not in pos_df['Stock'].tolist():
         pos_df = pos_df.append({'Stock':x, 'Pos':0}, ignore_index=True)
pos_df.set_index('Stock', inplace=True)
lot_size_df = pd.read_csv(lotsize_file, usecols=[1,3], names = ['Stock', 'Lotsize'], skiprows=[0])
lot_size_df = lot_size_df.applymap(lambda y: y.strip()).set_index('Stock')
lot_size_df = lot_size_df[lot_size_df['Lotsize'] != '']
pos_df = pos_df.merge(lot_size_df, left_index=True, right_index=True, how='inner')
pos_df['Num_Lots']  = pos_df['Pos']/pos_df['Lotsize'].astype(int)
pos_df = pos_df[pos_df['Num_Lots'] != 0]
pos_df.to_csv(output_position_file_, header=None, sep=' ')
