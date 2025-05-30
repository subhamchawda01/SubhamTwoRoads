#!/bin/bash

if [ $# -lt 2 ];
then
	echo "USAGE:<script><SHC><STRATFOLDER>[PREDDAY=TODAY][LOOKBACK=200][START_HHMM=0000][END_HHMM=2400][FEATURE-CONFIG-FILE][RESULTS_DIR=~/ec2_globalresults]";
	exit;
fi

SHC=$1;
STRATFOLDER=$2;
PREDDAY="`date --date="today" +%Y%m%d`";
LOOKBACK=200;
FEATURE_CONFIG="/spare/local/tradeinfo/day_features/product_configs/"$SHC"_config.txt";
RESULTS_DIR="$HOME/ec2_globalresults";
START_HHMM="0000";
END_HHMM="2400";

if [ $# -ge 3 ]
then
	PREDDAY=$3;
fi

if [ $# -ge 4 ]
then
	LOOKBACK=$4;
fi

if [ $# -ge 5 ]
then
	START_HHMM=$5;
fi

if [ $# -ge 6 ]
then
	END_HHMM=$6;
fi

if [ $# -ge 7 ]
then
	FEATURE_CONFIG=$7;
fi

if [ $# -ge 8 ]
then
	RESULTS_DIR=$8;
fi

FEATURE_FILE="/spare/local/tradeinfo/day_features/product_feature_files/"$SHC"_feature_file.txt";
YESTERDAY=$(~/basetrade_install/bin/calc_prev_week_day $PREDDAY);
START_DAY=$(~/basetrade_install/bin/calc_prev_week_day $PREDDAY $LOOKBACK);
echo $HOME/basetrade/WKoDii/get_day_features.pl $SHC $YESTERDAY $LOOKBACK $FEATURE_CONFIG DAY $START_HHMM $END_HHMM;
$HOME/basetrade/WKoDii/get_day_features.pl $SHC $YESTERDAY $LOOKBACK $FEATURE_CONFIG DAY $START_HHMM $END_HHMM 2>/dev/null 1>$FEATURE_FILE;

if [ -d $STRATFOLDER ]; then 
  ls $STRATFOLDER > STRATLIST;
elif [ -f $STRATFOLDER ]; then
  cat $STRATFOLDER > STRATLIST;
else
  echo "Error:this stratfolder does not exist";
  exit 1;
fi;

rm -rf results_File;

for STRAT in `cat STRATLIST`;do
	awk -v beg="$START_DAY" -v end="$PREDDAY" '{if ( $1 >= beg && $1 < end ){for(i=2;i<NF;i++){printf "%s ",$i }print $NF}}' "$FEATURE_FILE" > train_X;
	awk -v beg="$START_DAY" -v end="$PREDDAY" '{if ( $1 >= beg && $1 < end ){print $1}}' "$FEATURE_FILE" > train_X_dates;
	sh "$HOME/basetrade/scripts/get_results_for_strat_between_dates.sh" "$SHC" "$STRAT" "$START_DAY" "$PREDDAY" "$RESULTS_DIR" > train_Y_data;
	"$HOME/basetrade/WKoDii/forecast_dayfeatures.R" "$PREDDAY" "1" "ARIMA" "$FEATURE_FILE" > test_X;

	for i in `cat train_X_dates`
	do 
		j=`grep ^$i train_Y_data`;
		if [ ! -z "$j" ]
		then 
			echo "$j"; 
		else 
			echo "$i 0"; 
		fi
	done | awk '{print $2}' > train_Y;

	echo "$STRAT" `python "$HOME/basetrade/WKoDii/forecast_pnl_adaboost.py" train_X train_Y test_X` >> results_File; 

	rm -rf train_X
	rm -rf train_Y
	rm -rf train_X_dates
	rm -rf train_Y_data
	rm -rf test_X
done

sort -k2 -rn results_File;

rm -rf results_File;
rm -rf STRATLIST;
