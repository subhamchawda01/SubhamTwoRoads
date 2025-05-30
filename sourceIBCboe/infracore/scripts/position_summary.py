import os
import sys
import subprocess
import numpy as np
import pandas as pd
import datetime as dt

datasource_exchange_symbol_file = \
	'/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt' 
monthly_expiry_file = '/spare/local/files/ExpiryDates/monthly'

def BashExec(command):
    command = command.replace('&', '\&')
    return subprocess.check_output(['bash', '-c',command])\
    					.decode('UTF-8').rstrip();
def getExpiryList(date):
	return BashExec('cat %s | awk \'$1>= %s\''\
			%(monthly_expiry_file,date)).split('\n');

def formatSymbol(symbol):
	tokens = symbol.split('_');
	return tokens[0] + '_' + tokens[1] + '_' \
			+ tokens[2]  + str(exp_num_map[tokens[3][4:6]]);

def GetDataSourceExchSymbol():
	df = pd.read_csv(datasource_exchange_symbol_file,delim_whitespace=True,header=None);
	df.columns = ['DS','Instrument'];
	df = df[ df['Instrument'].apply(lambda x: 'FUT' in x and \
		( int(x.split('_')[3])) >= int(date) and \
		int(x.split('_')[3]) <= int(monthly_expiry_list[2])) ];
	df['Instrument'] = df['Instrument'].apply(formatSymbol)
	return df;

#main
if len(sys.argv) != 4 :
	print ('Usage : <script> <segment>(FO/CM) <tradefile> <output summary file>');
	exit(-1)
else:
	segment = sys.argv[1]
	tradefile = sys.argv[2]
	outfile = sys.argv[3];
date = (tradefile.split('.')[1]).split('_')[0];
monthly_expiry_list = getExpiryList(date);
exp_num_map = {};
num = 0 ;
for exp in monthly_expiry_list:
	exp_num_map[exp[4:6]] = num;
	num += 1;
trades_df = pd.read_csv(tradefile,delimiter='\001',header=None,\
			names=['DS','BS','Size','U','V','W','X','Y','Z']);
trades_df = trades_df.groupby(['DS','BS']).agg({'Size':'sum'}).reset_index();
trades_df['BS'] = trades_df['BS'].apply(lambda x: 'Buy' if x == 0 else 'Sell')
trades_df = pd.pivot_table(trades_df,values='Size',index=['DS'],columns=['BS'],aggfunc=np.sum).fillna(0).reset_index();
trades_df['Pos'] = trades_df.apply(lambda x: x['Buy'] - x['Sell'],axis=1);
trades_df['Pos'] = trades_df['Pos'].astype(int);
trades_df['Buy'] = trades_df['Buy'].astype(int);
trades_df['Sell'] = trades_df['Sell'].astype(int);
if segment == "FO":
	datasource_exchsymbol_df = GetDataSourceExchSymbol();
	trades_df = pd.merge(datasource_exchsymbol_df,trades_df,how='inner');
	del trades_df['DS']
else:
	trades_df.rename(columns={'DS':'Instrument'},inplace=True);
trades_df['Instrument'] = trades_df['Instrument'].str.ljust(20);
trades_df.to_csv(outfile,index=None,header=True,cols=['Instrument','Buy','Sell','Pos'],sep='\t');