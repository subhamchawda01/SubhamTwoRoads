today=$1;
/apps/anaconda/anaconda3/bin/python /home/dvctrader/usarraf/generate_position_file.py $today --max_num_spreads_diff 2 --base_num_spreads_factor 2 --physical_factor 1
/apps/anaconda/anaconda3/bin/python /home/dvctrader/usarraf/generate_position_file_agg.py $today --max_num_spreads_diff 2 --base_num_spreads_factor 2 --physical_factor 1 --aggressive_factor 0.5

scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.65:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_START_RATIO_IDX/PositionLimits.csv 
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_START_RATIO_IDX/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_START_RATIO_IDX/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.64:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_START_RATIO_IDX/PositionLimits.csv

scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.65:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL/PositionLimits.csv 
scp /home/dvctrader/usarraf/PositionLimits_agg.${today} dvctrader@10.23.227.65:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_AGGRESSIVE/PositionLimits.csv

scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits_agg.${today} dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_AGGRESSIVE/PositionLimits.csv

scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits_agg.${today} dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_AGGRESSIVE/PositionLimits.csv

scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.64:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_START_RATIO_IDX/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_START_RATIO_IDX/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.65:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits.${today} dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_FINAL/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits_agg.${today} dvctrader@10.23.227.65:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_AGGRESSIVE/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits_agg.${today} dvctrader@10.23.227.69:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_AGGRESSIVE/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits_agg.${today} dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_FUT1_HDG_VWAP_AGGRESSIVE/PosLimits_bkp/.
