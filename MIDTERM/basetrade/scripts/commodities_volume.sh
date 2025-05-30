#!/bin/bash -e

export IFS=";"
if [ $# -ne 1 ];
then
    echo "$0 DATE";
    exit;
fi
YYYYMMDD=$1;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi

EXEC=$HOME/LiveExec/bin/get_volume_on_day
List_of_symbols="YFEBM_0;YFEBM_1;YFEBM_2;YFEBM_3;YFEBM_4;"
MAIL_FILE=/tmp/mail_commodities_volumes.txt

rm $MAIL_FILE;
for symbol in $List_of_symbols;
do
    $EXEC $symbol $YYYYMMDD 0000 2330 2>/dev/null >> $MAIL_FILE ;
done

/bin/mail -s "CommodityVolumes" -r "commodityvolumes@ny11" "mayank@tworoads.co.in ravi@tworoads.co.in sghosh@dvcnj.com" < $MAIL_FILE
