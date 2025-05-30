import sys, os
import sqlite3

if len(sys.argv) != 4:
	print('Usage : python /home/dvctrader/infracore/scripts/pair_adf_stat_updator.py <database_name> <source_date> <target_date>')
	print('Example: python /home/dvctrader/infracore/scripts/pair_adf_stat_updator.py /spare/local/tradeinfo/NSE_Files/midterm_db 20170108 20170109')
	sys.exit()

db_name_ 	= sys.argv[1]
source_date_ 	= sys.argv[2]
target_date_ 	= sys.argv[3]

#conn = sqlite3.connect('/spare/local/tradeinfo/NSE_Files/midterm_db')
conn = sqlite3.connect(db_name_)

conn.execute("delete from PAIRS_ADF_STAT where day = " + target_date_ + ";")
conn.execute("insert into PAIRS_ADF_STAT (day, stock1, stock2, adf , halflife) select "+ target_date_ + ",stock1, stock2, adf , halflife from PAIRS_ADF_STAT where day =" + source_date_ + ";" )

conn.commit()

conn.close()
