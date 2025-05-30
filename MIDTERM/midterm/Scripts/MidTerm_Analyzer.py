import os, sys, pytz
import pandas as pd
import numpy as np
import pickle, subprocess
import datetime as dt

def foo( df_ ):
	x = df_.tolist()
	if len( df_ ) > 1:
		if x[ 0 ] != x[ 1 ]:
			return 'MIX'
	return x[ 0 ]

def ile10( df_ ):
	return np.percentile( df_, 10 )

def ile90( df_ ):
	return np.percentile( df_, 90 )

pd.set_option('display.width',1000)

ORDERSFILE_ = '/home/rgarg/Ordersfile_SG_201708/xxx'

EXECS_PATH_ = '/spare/local/logs/alllogs/MediumTerm/'
EXECS_PREFIX = 'GENERAL_execlogic_trades_'

PRODUCTS_ = [ 'NIFTY', 'BANKNIFTY' ]

# Read orders files
df_ = pd.read_csv( ORDERSFILE_, delim_whitespace = True, header = None, comment = '#' ) 
df_.columns = [ 'Timestamp', 'Shortcode', 'Lots', 'Tranche_ID', 'Ref_Px', 'Type' ]
df_.Timestamp = df_.Timestamp.apply( lambda x : str( x ) )
df_.Lots = df_.Lots.apply( lambda x : int( x ) )
df_ = df_.set_index( 'Timestamp', drop = False )

# Read exec files
execfile_names=[filename for filename in os.listdir( EXECS_PATH_ ) if filename.startswith(EXECS_PREFIX)]
execs_=pd.DataFrame()
for execfile in execfile_names:
	execs_ = pd.concat( [ execs_, pd.read_csv( EXECS_PATH_ + execfile, delim_whitespace = True, header = None, comment = '#', names = [ 'Order_ID', 'Shortcode', 'Action', 'Qty', 'Exec_Px', 'Comm', 'Comp_Time', 'Exec_Logic' ] ) ] )

execs_['exec_call'] = -1
execs_['exec_put'] = -1
execs_['opt_type'] = execs_.Order_ID.apply( lambda x : True if ( 'CE' in x ) else ( False if 'PE' in x else None) )
#execs_.set_index( execs_.Order_ID, inplace = True )
execs_.reset_index( inplace = True )
execs_.loc[ execs_.opt_type == True, 'exec_call' ] = execs_.Exec_Px
execs_.loc[ execs_.opt_type == False, 'exec_put' ] = execs_.Exec_Px

# Need to average the exec calls and puts yet since order ID has CE/PE info
execs_ = execs_.groupby( [ 'Order_ID' ] ).agg( { 'Order_ID' : 'first', 'Action' : 'first', 'Qty' : 'sum', 'Exec_Px' : 'mean', 'Comm' : 'sum', 'exec_call' : 'mean', 'exec_put' : 'mean', 'Comp_Time' : 'max', 'Exec_Logic' : foo } )
execs_ = execs_[[ 'Order_ID', 'Action', 'Qty', 'Exec_Px', 'Comm', 'exec_call', 'exec_put', 'Comp_Time', 'Exec_Logic' ]]
execs_[ 'Timestamp' ] = execs_.Order_ID.apply( lambda x : x.split( '_' )[0] )
execs_.Comp_Time = execs_.Comp_Time.astype( int )
conversion_ = { 'Action' : 'first', 'Qty' : 'mean', 'Exec_Px' : 'sum', 'Comm' : 'sum', 'exec_call' : 'max', 'exec_put' : 'max', 'Comp_Time' : 'max', 'Exec_Logic' : foo }

grouped_ = execs_.groupby( [ 'Timestamp' ] ).agg( conversion_ )

out_ = pd.merge( df_, grouped_, left_index = True, right_index = True, how = 'outer' )
out_.Timestamp = out_.Timestamp.apply( lambda x : dt.datetime.fromtimestamp( int( x ) / 1e9, pytz.timezone( 'Asia/Calcutta' ) ) )

# Now do slippage analysis
out_ = out_[[ 'Timestamp', 'Shortcode', 'Ref_Px', 'Comp_Time', 'Qty', 'Exec_Px', 'Comm', 'exec_call', 'exec_put', 'Exec_Logic', 'Type' ]]

# Check time taken to execute
out_.Comp_Time -= out_.index.map( lambda x : int( x ) / 1e9 )

out_[ 'TypeX' ] = out_.Shortcode.apply( lambda x : 'FUT' if 'FUT' in x else 'STRADDLE' )
out_[ 'Slippage' ] = ( -np.sign( out_.Qty ) * ( out_.Exec_Px / out_.Ref_Px - 1 ) - abs( out_.Comm / ( out_.Ref_Px * out_.Qty ) ) ) * 10000
out_[ 'Ticker' ] = out_.Shortcode.apply( lambda x : 'BANKNIFTY' if 'BANKNIFTY_' in x else 'NIFTY' )


print( str( len( out_ ) - len( out_.dropna() ) ) + ' executions are being removed' )
out_.dropna( inplace = True )

out_ = out_[ out_.Type == 'Entry' ]

out_ = out_[ abs( out_.Slippage ) < 1000 ]

answer_ = out_.pivot_table( columns = [ 'Ticker' ], index = [ 'TypeX', 'Exec_Logic' ], values = [ 'Slippage' ], aggfunc = [ np.mean, np.std, np.median, ile10, ile90, len ] )

temp_ = out_[ out_.TypeX == 'STRADDLE' ]
temp_[ 'Date' ] = temp_.Timestamp.apply( lambda x : x.date() )
answer_1_ = temp_.pivot_table( columns = [ 'Date' ], index = [ 'Ticker', 'Exec_Logic' ], values = [ 'Slippage' ], aggfunc = [ np.mean ] )
answer_2_ = temp_.pivot_table( columns = [ 'Date' ], index = [ 'Ticker', 'Exec_Logic' ], values = [ 'Slippage' ], aggfunc = [ len ] )


print( answer_ )

print( answer_1_ )


#dsfsdfsd
#out_[ 'Slippage' ] = 0
##out_.loc[  ]
#
#
#asdsadsa
#
## Process BNF gamma hedge
#[ bnf_execs_, bnf_semi_, bnf_no_ ] = ProcessTicker( out_, 'BANKNIFTY' )
## Process stocks IV short
#[ stock_execs_, stock_semi_, stock_no_ ] = ProcessTicker( out_, 'STOCKS' )
#
#bnf_slippage_ = bnf_execs_.Slippage.describe()
#stock_slippage_ = stock_execs_.Slippage.describe()
#bnf_time_ = bnf_execs_.Comp_Time.describe()
#stock_time_ = stock_execs_.Comp_Time.describe()
#
#
#sys.exit()
#out_[ 'Slippage_Execution' ]  = ( ( abs( out_.Exec_Px ) - abs( out_.Ref_Px ) ) / abs( out_.Strike ) ) * 10000
#out_[ 'Slippage_ExecutionX' ] = out_.Slippage_Execution
#out_.loc[ out_.Action == 'BUY', 'Slippage_ExecutionX' ] = -out_.Slippage_Execution
#out_.Slippage_Execution = out_.Slippage_ExecutionX
#del out_[ 'Slippage_ExecutionX' ]
#
## Add commission
#out_[ 'Slipage_Total' ] = out_.Slippage_Execution - abs( out_.Comm / ( out_.Qty * out_.Ref_Px ) ) * 10000
#out_ = out_[ ( out_.exec_call != -1 ) & ( out_.exec_put != -1 ) ]
