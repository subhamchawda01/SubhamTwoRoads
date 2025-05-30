#!/bin/bash
# Input $1 = Path of Folder containing raw files.

for file in $1/*;
do
	#echo $file
        echo "Starting on '$file'"
	date=$(echo $file | grep -oP '[\d]{4}.[\d]{2}.[\d]{2}')
        YYYY=${date:0:4}
        MM=${date:5:2}
        DD=${date:8:2}

        symbol=$(echo $file | grep -oP '[A-Z]{2}_')
        symbol=${symbol:0:2}
        #echo $symbol

        SYMBOLTEMP=$symbol"temp.csv"
        cut -d, -f2 $file > "$SYMBOLTEMP"

	ORDER_FEED=$HOME"/media/shared/ephemeral17/tmp_data_files/SGXOFLoggedData/SPR/"$YYYY"/"$MM"/"$DD"/"
	PRICE_FEED=$HOME"/media/shared/ephemeral17/tmp_data_files/SGXPFLoggedData/SPR/"$YYYY"/"$MM"/"$DD"/"
	LOG=$HOME"/media/shared/ephemeral17/tmp_data_files/SGXLoggedData/SPR/"$YYYY"/"$MM"/"$DD"/"
        SYMBOLRAWDATA=$symbol"Orderfeed/"

	mkdir -p "$SYMBOLRAWDATA"
	mkdir -p $ORDER_FEED
	mkdir -p $PRICE_FEED
	mkdir -p $LOG

	for i in `sort -u -k 1,1 "$SYMBOLTEMP"`;
	do
	if [ "$i" != "series" ];then
            grep $i $file > $SYMBOLRAWDATA$i".csv";
            ./sgx_offline_decoder $SYMBOLRAWDATA$i".csv" $ORDER_FEED$i"_"$YYYY$MM$DD;
            echo "OrderFeed Generated"
            ./offline_sgx_orderlvl_to_pricefeed_converter $ORDER_FEED$i"_"$YYYY$MM$DD $PRICE_FEED$i"_"$YYYY$MM$DD > $LOG$i"_"$YYYY$MM$DD 2>&1 ;
            echo "Pricefeed and Logs Generated at '$PRICE_FEED$i"_"$YYYY$MM$DD' and '$LOG$i"_"$YYYY$MM$DD'"
	fi
	done
	rm "$SYMBOLTEMP"
	rm -rf "$SYMBOLRAWDATA"
	unset date
        echo "Processed '$file'"

done
 

