import sys, os
import sqlite3
import pandas as pd

##### This script updates EARNINGS_DATES in midterm_db database
##### It can handle cases of earnings postponement as well as cases where earnings has already been delcared for a certain stock


def getQuarterFromDate(date_):
        year_  = date_[0:4]
        month_ = date_[4:6]
        return  year_ + '_Q' + str( int( (int(month_) - 1 ) /3 ) + 1 )

def AlreadyDeclaredForQuarter(stock_, date_):
        quarter_= getQuarterFromDate(date_)
        return not declared_earnings_.loc[(declared_earnings_.Stock ==stock_) & (declared_earnings_.Season == quarter_)].empty


db_name_ = '/home/dvctrader/trash/midterm_db_temp'

conn = sqlite3.connect(db_name_)

declared_earnings_ = pd.read_csv('/spare/local/tradeinfo/NSE_Files/consolidated_earnings', header= None, delimiter='\t', names=['Date','Time','Stock'])
declared_earnings_['Season'] = declared_earnings_['Date'].apply(lambda x : getQuarterFromDate(str(x) ) )

upcoming_earnings_ = '/spare/local/tradeinfo/NSE_Files/upcoming_earnings.csv'
earnings_df_ = pd.read_csv(upcoming_earnings_,delim_whitespace = True, header = None, names= ['Ticker','Date'])
earnings_df_.set_index('Ticker', inplace  =True)


tickers_universe = [line.strip() for line in open("/spare/local/tradeinfo/NSE_Files/TICKERS_universe", 'r')]

for row_ in earnings_df_.iterrows():
	ticker_ = row_[0]
	date_ = row_[1][0].replace('-','')
	if ticker_ not in tickers_universe or AlreadyDeclaredForQuarter(ticker_, date_):
		print('Already passed for ' + ticker_)
		continue
	quarter_ = getQuarterFromDate(date_)
	#print(ticker_ + '  ' + quarter_)
	res_ = conn.execute('select day from EARNINGS_DATES where quarter = "' + quarter_ + '" and stock = "' + ticker_ + '";')
	to_insert_ = 1
	for res_row_ in res_:
		to_insert_ = 0
		if len(str(res_row_[0])) == 0 or str(res_row_[0]) == date_:
			continue
		else:
			print('Earnings Date changed for ticker : ' + ticker_)
			conn.execute('update EARNINGS_DATES set day = ' + date_ + ' where quarter = "' + quarter_ + '" and stock = "' + ticker_ + '";')
			conn.commit()
	if to_insert_ == 1:
		#print('Inserting')
		conn.execute('insert into EARNINGS_DATES (quarter,stock,day) values("' + quarter_ + '","' + ticker_ + '",' + date_ + ');')
		conn.commit()
