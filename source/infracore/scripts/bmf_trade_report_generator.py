import os
import argparse
import subprocess
import pandas as pd
import datetime as dt


def UpdateFields(row, date):
	row['Date'] = date
	row['InvestorCode'] = 'NRE000000043'
	code = row['Instrument'][0:3]
	row['ConntractCode'] = 'FUT ' + str(code);  
	row['ContractMaturity'] = row['Instrument'][3:len(row['Instrument'])];
	row['BS'] = 'C' if row['BS'] == 0 else 'V'
	row['BrokerID'] = 'BRPLURAL'
	row['FixedField1'] = 'S'
	row['FixedField2'] = 'N'
	return row;
 
'''
	This function is specific to the formar of the trade file
	and the returned data frame to if specific to exchange format
'''

def getTradeReport(file, date):
	df = pd.read_csv(file,delimiter="\001",header=None);
	df = df[ [0,1,2,3] ];
	df.columns= ['Instrument','BS','Volume','Price'];
	df = df.apply(lambda x:UpdateFields(x, date),axis=1)
	df = df [ ['Date','InvestorCode','ConntractCode','ContractMaturity','BS','BrokerID','Volume','Price','FixedField1','FixedField2'] ]
	return df

def main():
	#add arguement and parse argument
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'--date',
		type=str,
		help='DD/MM/YYYY formatted date for which trade report to be generated',
		default=None,
		dest='date')
	parser.add_argument(
		'--trade_file',
		type=str,
		help='complete path of the tradefile',
		default=None,
		dest='trade_file')
	parser.add_argument(
		'--outfile',
		type=str,
		default=None,
		dest='outfile')
	args = parser.parse_args();
	if not os.path.isfile(args.trade_file):
		print ("Unable to find the tradefile, exiting...");
		exit(0);
	trade_report_df = getTradeReport(args.trade_file, args.date);
	trade_report_df.to_csv(args.outfile,index=None,header=None,sep='\t',mode='a')

if __name__ == '__main__':
	main()