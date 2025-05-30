
#!/bin/bash

if [ "$#" -ne 1 ] ; then
  echo "USAGE: SCRIPT <YYYYMMDD>"
  exit
fi

today=$1;

scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.65:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_START_RATIO_IDX/PositionLimits.csv 
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_START_RATIO_IDX/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_START_RATIO_IDX/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.64:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_START_RATIO_IDX/PositionLimits.csv

scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.65:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL/PositionLimits.csv 
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.65:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL_NEW_202111/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits_agg.${today} dvctrader@10.23.227.65:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_AGGRESSIVE/PositionLimits.csv

scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL_NEW_202111/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits_agg.${today} dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_AGGRESSIVE/PositionLimits.csv

scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits_agg.${today} dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_AGGRESSIVE/PositionLimits.csv

scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.64:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_START_RATIO_IDX/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_START_RATIO_IDX/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.65:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.65:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL_NEW_202111/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL_NEW_202111/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits_agg.${today} dvctrader@10.23.227.65:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_AGGRESSIVE/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits_agg.${today} dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_AGGRESSIVE/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits_agg.${today} dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_AGGRESSIVE/PosLimits_bkp/.



scp /home/dvctrader/usarraf/PositionLimits.13 dvctrader@10.23.227.63:/home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_CUSTOM_PRICE_SPC/PositionLimits.csv 
scp /home/dvctrader/usarraf/PositionLimits.13 dvctrader@10.23.227.63:/home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_CUSTOM_PRICE_SPC/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits.13 dvctrader@10.23.227.63:/home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_CUSTOM_PRICE_SIMPLETREND2/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits.13 dvctrader@10.23.227.63:/home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_CUSTOM_PRICE_SPC_QUOTER_MASTER/PositionLimits.csv

scp /home/dvctrader/usarraf/PositionLimits_momentum.13 dvctrader@10.23.227.63:/home/dvctrader/ATHENA/CONFIG_FUT0_MOMENTUM/PositionLimits.csv

scp /home/dvctrader/usarraf/PositionLimits.13 dvctrader@10.23.227.65:/home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_CUSTOM_PRICE_SPC/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits.13 dvctrader@10.23.227.65:/home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_CUSTOM_PRICE_SPC/PosLimits_bkp/.


scp /home/dvctrader/usarraf/PositionLimits.13 dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_CUSTOM_PRICE_SPC/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits.13 dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_CUSTOM_PRICE_SPC/PosLimits_bkp/.

scp /home/dvctrader/usarraf/PositionLimits.13 dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_CUSTOM_PRICE_SPC/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits.13 dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_CUSTOM_PRICE_SPC/PosLimits_bkp/.


scp /home/dvctrader/usarraf/PositionLimits_momentum.13 dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT0_MOMENTUM/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits_momentum.13 dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT0_MOMENTUM/PositionLimits.csv

scp /home/dvctrader/usarraf/PositionLimits_momentum.13 dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT0_MACD/PositionLimits.csv

scp /home/dvctrader/usarraf/PositionLimits_momentum.13 dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT0_MACD/PositionLimits.csv

scp /home/dvctrader/usarraf/PositionLimits_momentum.13 dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT0_MR/PositionLimits.csv

scp /home/dvctrader/usarraf/PositionLimits_momentum.13 dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT0_MIDTERM_ONE_SPLIT2/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits_momentum.13 dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT0_MIDTERM_ONE_SPLIT1/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits_momentum.13 dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT0_MIDTERM_ONE_SPLIT2/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits_momentum.13 dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT0_MIDTERM_ONE_SPLIT1/PositionLimits.csv



scp /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@10.23.227.64:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL/PositionLimits.csv 
scp /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@10.23.227.64:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@10.23.227.64:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL_20200505/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@10.23.227.64:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL_20200505/PosLimits_bkp/.

scp /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL/PositionLimits.csv 
scp /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL/PosLimits_bkp/.

