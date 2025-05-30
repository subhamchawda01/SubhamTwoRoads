#!/bin/bash

today=`date +"%Y%m%d"`;

if [ $# -eq 1 ];
then
    today=$1;
fi

i=0;

LIVE_DIR="/home/dvctrader/nishit/SIM_CHECK_CONFIG_OPT_IDX_ATM_STRADDLE_SPOT_IBB_HDG_LONGSPLIT_DAILY/"
TRADE_ENGINE="/home/dvctrader/nishit/execs/trade_engine_idx_longrestruct_20211223"
logs="/tmp/straddle_logs.txt"

sed -i s#"^TOTAL_PORTFOLIO_STOPLOSS = .*"#"TOTAL_PORTFOLIO_STOPLOSS = 10000000"#g $LIVE_DIR/PositionLimits.csv
sed -i s#"^GROSS_EXPOSURE_LIMIT = .*"#"GROSS_EXPOSURE_LIMIT = 50"#g $LIVE_DIR/PositionLimits.csv

$TRADE_ENGINE $LIVE_DIR/LIVE_FILE_IDX.csv $today IST_915 IST_1530 1235521 > $logs &
sleep 60;

sed -i 's/LO1 = Long1.cfg/#LO1 = Long1.cfg/' $LIVE_DIR/NSE_*W_MM/Main*
$TRADE_ENGINE $LIVE_DIR/LIVE_FILE_IDX.csv $today IST_915 IST_1530 1235522 > $logs &
sleep 60;

sed -i 's/#LO1 = Long1.cfg/LO1 = Long1.cfg/' $LIVE_DIR/NSE_*W_MM/Main*
sed -i 's/SST1 = Sst1.cfg/#SST1 = Sst1.cfg/' $LIVE_DIR/NSE_BANKN*W_MM/Main*

sed -i 's/IBB1 = Ibb1.cfg/#IBB1 = Ibb1.cfg/' $LIVE_DIR/NSE_*NIFTY_FUT0_MM/Main*
sed -i 's/IBB2 = Ibb2.cfg/#IBB2 = Ibb2.cfg/' $LIVE_DIR/NSE_*NIFTY_FUT0_MM/Main*
sed -i 's/GAPIDX1 = GapIdx1.cfg/#GAPIDX1 = GapIdx1.cfg/' $LIVE_DIR/NSE_*NIFTY_FUT0_MM/Main*

$TRADE_ENGINE $LIVE_DIR/LIVE_FILE_IDX.csv $today IST_915 IST_1530 1235523 > $logs &
sleep 60;

sed -i 's/MAIDX1 = MaIdx1.cfg/#MAIDX1 = MaIdx1.cfg/'  $LIVE_DIR/NSE_*NIFTY_FUT0_MM/Main*
sed -i 's/#GAPIDX1 = GapIdx1.cfg/GAPIDX1 = GapIdx1.cfg/' $LIVE_DIR/NSE_*NIFTY_FUT0_MM/Main*

$TRADE_ENGINE $LIVE_DIR/LIVE_FILE_IDX.csv $today IST_915 IST_1530 1235524 > $logs &
sleep 60;


sed -i 's/GAPIDX1 = GapIdx1.cfg/#GAPIDX1 = GapIdx1.cfg/' $LIVE_DIR/NSE_*NIFTY_FUT0_MM/Main*
sed -i 's/#IBB2 = Ibb2.cfg/IBB2 = Ibb2.cfg/' $LIVE_DIR/NSE_*NIFTY_FUT0_MM/Main*

$TRADE_ENGINE $LIVE_DIR/LIVE_FILE_IDX.csv $today IST_915 IST_1530 1235525 > $logs &
sleep 60;


sed -i 's/IBB2 = Ibb2.cfg/#IBB2 = Ibb2.cfg/' $LIVE_DIR/NSE_*NIFTY_FUT0_MM/Main*
sed -i 's/#IBB1 = Ibb1.cfg/IBB1 = Ibb1.cfg/' $LIVE_DIR/NSE_*NIFTY_FUT0_MM/Main*

$TRADE_ENGINE $LIVE_DIR/LIVE_FILE_IDX.csv $today IST_915 IST_1530 1235526 > $logs &

