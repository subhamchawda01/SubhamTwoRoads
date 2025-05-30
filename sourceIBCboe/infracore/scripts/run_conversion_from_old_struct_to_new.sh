#!/bin/bash

today_=20220107
date_=20211004

count=20
total=20
DATA_CONVERTER_EXEC="/home/pengine/prod/live_execs/bse_generic_to_dotex_offline_converter_between_old_to_new";
AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;

while [[ $date_ -lt $today_ ]] ; do
        DD=${YYYYMMDD:6:2}
        MM=${YYYYMMDD:4:2}
        YYYY=${YYYYMMDD:0:4}
        DATA_DIR="/NAS5/data/BSELoggedData/BSE/$YYYY/$MM/$DD/"
        OUTDATA_DIR="/NAS5/data/BSELoggedData_NEW_2022/BSE/$YYYY/$MM/$DD/"
        for filename in `ls ${DATA_DIR}`
        do
          basefilename=$(basename "$filename" .gz);
          echo "$DATA_CONVERTER_EXEC $filename $date_ $OUTDATA_DIR/$basefilename"
          DATA_CONV_PID=$! ;
          $AFFIN_EXEC ASSIGN $DATA_CONV_PID "DATACONVERTION"
          ((count=count-1))
          if [ $count -eq 0 ] ; then
                while [ 1 ] ; do
                        sleep 0.2
                        remaining=`ps -ef | grep "bse_generic_to_dotex_offline_converter" | grep -v "grep" | wc -l`
                        ((count=total-remaining))
                        if [ $count -gt 0 ] ; then
                                break
                        fi
                done
        fi
        done
        echo "EMAIL  Converted all GENERIC at $date_"
        while [ 1 ] ; do
          sleep 2
          remaining=`ps -ef | grep "bse_generic_to_dotex_offline_converter" | grep -v "grep" | wc -l`
          if [ $remaining -eq 0 ] ; then  #no conversion running
                break
          fi
        done

        cd $OUTDATA_DIR
        gzip *
        exit
        date_=`/home/pengine/prod/live_execs/update_date $date_ N A`
        is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
        while [[ $is_holiday = "1" ]] 
        do
                date_=`/home/pengine/prod/live_execs/update_date $date_ N A`
                is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
        done
echo
done

