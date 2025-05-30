today=$1;
/apps/anaconda/anaconda3/bin/python /home/dvctrader/usarraf/generate_position_file_13.py $today --physical_factor 1
scp /home/dvctrader/usarraf/PositionLimits.13 dvctrader@10.23.227.63:/home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_CUSTOM_PRICE_SPC/PositionLimits.csv 
scp /home/dvctrader/usarraf/PositionLimits.13 dvctrader@10.23.227.63:/home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_CUSTOM_PRICE_SPC/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits.13 dvctrader@10.23.227.63:/home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_CUSTOM_PRICE_SIMPLETREND2/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits.13 dvctrader@10.23.227.63:/home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_CUSTOM_PRICE_SPC_QUOTER_MASTER/PositionLimits.csv


/apps/anaconda/anaconda3/bin/python /home/dvctrader/usarraf/generate_position_file_13_momentum.py $today --physical_factor 1
sed -i s#"^TOTAL_PORTFOLIO_STOPLOSS = .*"#"TOTAL_PORTFOLIO_STOPLOSS = 800000"#g /home/dvctrader/usarraf/PositionLimits_momentum.13
sed -i s#"^GROSS_EXPOSURE_LIMIT = .*"#"GROSS_EXPOSURE_LIMIT = 15"#g /home/dvctrader/usarraf/PositionLimits_momentum.13
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

