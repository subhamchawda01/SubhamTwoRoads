#!/bin/bash

# Input $1 = File containing list of Dates (YYYYMMDD).

for  date in `cat $1 | grep -v "#"` ; do
	echo $date 
    YYYY=${date:0:4}
    MM=${date:4:2}
    DD=${date:6:2}

    L1_OUT_DIR="/media/shared/ephemeral16/tmp_data_files/NSEL1Volume/Volumes/"
    LOG="/media/shared/ephemeral16/tmp_data_files/NSEL1Volume/Log/"$YYYY"/"$MM"/"$DD"/"

    mkdir -p $L1_OUT_DIR
    mkdir -p $LOG

    count=20
    total=20

    for shc in `~/basetrade_install/bin/get_contract_specs  ALL $date | grep "SHC" | grep "NSE_" | grep -v "FUT0_1" | cut -d' ' -f2 ` ;
    do
        if [ $count -gt 0 ] ; then

            echo $shc" !!"   #Identifier so that we can do `grep -v "!!" form log`  
            ./get_volume_on_day $shc $date USEL1DATA 2>$LOG$shc"_"$date | grep "$shc " | cut -d' ' -f3 > $L1_OUT_DIR$shc"_"$date  &
            ((count=count-1))
        fi
        if [ $count -eq 0 ] ; then
            while [ 1 ] ; do
                sleep 2
                remaining=`ps -ef | grep "./get_volume_on_day" | grep -v "grep" | wc -l`
                ((count=total-remaining))
                if [ $count -gt 0 ] ; then
                        break
                fi
            done
            #echo "Remaining " $remaining
            #echo "Starting " $count
        fi
    done
done

echo "Finished"

echo "Congratulations. Location : $L1_OUT_DIR" | /bin/mail -s "NSE_L1 Volume Generation Finished" -r "nse_l1_volume_calculator" "pranjal.jain@tworoads.co.in"
