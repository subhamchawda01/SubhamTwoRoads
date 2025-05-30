import os, sys, time
import pandas as pd
import pickle, subprocess
import datetime as dt

pd.set_option('display.width',1000)

ORDERSFILE_ = '/home/rgarg/Ordersfile_SG_201708/xxx'
ORS_LOGS_ = '/home/rgarg/LogPath/'
PARAM_FILE_ = '/home/rgarg/Helper/mult_param_rv'
EXEC_ = '/home/rgarg/cvquant_install/midterm/bin/nse_simple_trader_sim'
KEY_ = 'SG'

def Exec_Bash( command_ ):
	process = subprocess.Popen(command_, shell=True, stdout=subprocess.PIPE)

start_dt_ = '20170801'
end_dt_ = '20170831'

start_ = dt.datetime.strptime( start_dt_, '%Y%m%d' )
end_ = dt.datetime.strptime( end_dt_, '%Y%m%d' )
# Iterate over all days one by one
delta_ = dt.timedelta( days = 1 )
orders_df_ = pd.read_csv( ORDERSFILE_, header = None, comment = '#', delim_whitespace = True )
orders_df_.columns = [ 'Timestamp', 'Shortcode', 'Lots', 'Tranche_ID', 'Ref_Px', 'Type' ]
orders_df_[ 'Date' ] = orders_df_.Timestamp.apply( lambda x : dt.datetime.fromtimestamp( int( x / 1e9 ) ).date() )
date_ = start_

while date_ <= end_:
        # Ignore on weekends
	if date_.weekday() >= 5:
		date_ += delta_
		continue
	print( '=====' )
	readable_date_ = date_.strftime( '%Y%m%d' )
	daily_df_ = orders_df_[ orders_df_.Date == date_.date() ]
	if len( daily_df_ ) == 0:
		print( 'No orders for date -> ' + readable_date_ )
		date_ += delta_
		continue
	print( 'Processing on date -> ' + readable_date_ )
	print( 'Orders ->' )
	print( daily_df_ )
	
	orders_ = '/home/rgarg/Orders/orders_' + readable_date_
	daily_df_.to_csv( orders_, index = False, sep = '\t', header = None )

	cmd_ = EXEC_ + ' ' + ORS_LOGS_ + ' ' + PARAM_FILE_ + ' ' + orders_ + ' ' + readable_date_ + ' > ' + ORS_LOGS_ + 'cout_' + readable_date_
	Exec_Bash( cmd_ )
	print( 'Run -> ' + cmd_ )
	date_ += delta_
	time.sleep( 300 )
