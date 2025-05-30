#!/bin/bash
# Input $1 = Path of Folder containing raw files.

SESSION="3"
SEPARATOR="|"
for file in $1/*;
do
	echo  #Blank Line
    echo "Starting on '$file'"
	date=$(echo $file | grep -oP '[\d]{8}')
    YYYY=${date:0:4}
    MM=${date:4:2}
    DD=${date:6:2}

	ORDER_FEED=$HOME"/media/shared/ephemeral17/tmp_data_files/BSEOFLoggedData/BSE/"$YYYY"/"$MM"/"$DD"/"
	PRICE_FEED=$HOME"/media/shared/ephemeral17/tmp_data_files/BSEPFLoggedData/BSE/"$YYYY"/"$MM"/"$DD"/"
	PFLOG=$HOME"/media/shared/ephemeral17/tmp_data_files/BSELoggedData/BSE/"$YYYY"/"$MM"/"$DD"/"
	OFLOG=$HOME"/media/shared/ephemeral17/tmp_data_files/BSEOFLog/BSE/"$YYYY"/"$MM"/"$DD"/"
    RAWDATA="Rawdata/"

	mkdir -p $RAWDATA
	mkdir -p $ORDER_FEED
	mkdir -p $PRICE_FEED
	mkdir -p $PFLOG
	mkdir -p $OFLOG

	for inst_code in 1001539 1001554 1001555 1001557 1001663 1001666 1001671 1001709 1002080 1002094 1002098 1002103 1002270 1002280 1002281 1002283 1002673 1002680 1002683 1002687 1002797 1002807 1002808 1002810 1002928 1002937 1002939 1002951 1003043 1003082 1003084 1003086 1003157 1003161 1003199 1003202 1003295 1003298 1003301 1003313 1003444 1003448 1003462 1003489 1003573 1003577 1003597 1003603 1003696 1003711 1003713 1003717 1003808 1003809 1003815 1003851 1003932 1003946 1003949 1003953 1004080 1004091 1004095 1004098 1004223 1004234 1004238 1004242 1004348 1004364 1004366 1004370 1004614 1004629 1004634 1004650 1004754 1004788 1004791 1004794 1004884 1004888 1004923 1004925 1005041 1005045 1005048 1005061 1005298 1005301 1005307 1005334 1005533 1005537 1005547 1005550;
	do
		grep -q $SESSION$SEPARATOR$inst_code$SEPARATOR $file
		if [ $? == 0 ]; then
			echo    #Blank Line
			echo "Processing Instrument $inst_code"
			grep $SESSION$SEPARATOR$inst_code$SEPARATOR $file > $RAWDATA$inst_code"_"$YYYY$MM$DD
			./bse_offline_decoder  $RAWDATA$inst_code"_"$YYYY$MM$DD $ORDER_FEED > $OFLOG$inst_code"_"$YYYY$MM$DD 2>&1
			echo "OrderFeed Generated"
			./offline_bse_order_book $YYYY$MM$DD $inst_code $ORDER_FEED $PRICE_FEED > $PFLOG$inst_code"_"$YYYY$MM$DD 2>&1
			echo "Pricefeed Generated"

			rm $RAWDATA$inst_code"_"$YYYY$MM$DD
		#else
		#	echo "Not Processing Instrument $inst_code"
		fi
	done
	rm -rf $RAWDATA
	unset date
    echo "Processed '$file'"

done
 

