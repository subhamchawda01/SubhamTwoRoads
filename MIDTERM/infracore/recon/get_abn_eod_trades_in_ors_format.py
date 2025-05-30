import xml.etree.ElementTree
import sys
from datetime import datetime
from time import mktime

USAGE="script YYYYMMDD"
if len(sys.argv) < 2:
	print USAGE
	sys.exit(0)

def get_unixtime (date_str,time_str):
	dt = datetime.strptime(date_str+":"+time_str, "%Y%m%d:%H:%M:%S")
	return int(mktime(dt.timetuple()))

input_date_=sys.argv[1]
INPUT_FILE="/apps/data/AbnamroTrades/AbnamroFiles/" +str(input_date_)+"_abnamro_trx.xml"
tree = xml.etree.ElementTree.parse(INPUT_FILE).getroot()
symbol_last_traded_price_map={}
symbol_last_traded_time_map={}
soh = '\x01' # start of header

for node in tree.getiterator('FutureMovement'):
	product_= str(node.findtext('Product/Symbol'))
	product_expiry_ = str(node.findtext('Product/Expiry'))[0:-2]
	buy_or_sell_= str(node.findtext('BuySellCode'))
	trade_price_ = node.findtext('TransactionPrice')
	trade_timestamp_ = str(node.findtext('TimeStamp')) #in AEST
	trade_unix_time_=get_unixtime(input_date_,trade_timestamp_)
	transaction_type_ = node.findtext('TransactionType')
	transaction_quantity_ = node.findtext('TransactionQuantity')

	exchange_symbol_ = product_ + product_expiry_
	#not assuming trades are in sorted order of timestamps
	if not (exchange_symbol_ in symbol_last_traded_price_map) :
		symbol_last_traded_price_map[exchange_symbol_] = trade_price_
		symbol_last_traded_time_map[exchange_symbol_] = trade_unix_time_

  	elif (trade_unix_time_ > symbol_last_traded_time_map[exchange_symbol_]) :
		symbol_last_traded_time_map[exchange_symbol_] = trade_unix_time_
		symbol_last_traded_price_map[exchange_symbol_] = trade_price_

	side=1;
	if("BUY" in buy_or_sell_):
		side=0
	print exchange_symbol_+soh+str(side)+soh+transaction_quantity_+soh+trade_price_+soh+"0"

#add an dummy entry per symbol to record last trade price
for symbol in symbol_last_traded_price_map:
	print symbol+soh+str(0)+soh+str(0)+soh+symbol_last_traded_price_map[symbol]+soh+"0"
