#STEPS TO GET DISP TRADES

1. /home/dvctrader/anaconda3/bin/python /home/dvctrader/important/MANUAL_TRADE/Intraday_trades_match.py $today_date
2. /home/dvctrader/important/MANUAL_TRADE/generate_trade.sh $today_date $start_num $tranch_id >/tmp/disp_order_20210429
   eg. generate_trade.sh 20210429 100 40 >/tmp/disp_order_20210429 
