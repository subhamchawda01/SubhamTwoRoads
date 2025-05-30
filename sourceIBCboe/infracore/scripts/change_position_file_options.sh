today=$1;
/apps/anaconda/anaconda3/bin/python /home/dvctrader/usarraf/generate_position_file_options.py $today --physical_factor 1
scp /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@10.23.227.64:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL/PositionLimits.csv 
scp /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@10.23.227.64:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL/PosLimits_bkp/.
scp /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@10.23.227.64:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL_20200505/PositionLimits.csv
scp /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@10.23.227.64:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL_20200505/PosLimits_bkp/.

#scp -P 22764 /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@202.189.245.205:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL/PositionLimits.csv
#scp -P 22764 /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@202.189.245.205:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL/PosLimits_bkp/.

scp /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL/PositionLimits.csv 
scp /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL/PosLimits_bkp/.

#scp -P 22784 /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@202.189.245.205:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL/PositionLimits.csv
#scp -P 22784 /home/dvctrader/usarraf/PositionLimits.Options.${today}.txt dvctrader@202.189.245.205:/home/dvctrader/ATHENA/CONFIG_OPT_MM_HDG_FULL/PosLimits_bkp/.

