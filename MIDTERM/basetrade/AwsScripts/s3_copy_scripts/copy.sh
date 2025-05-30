#!/bin/bash
d=`date -d "-$1 day" '+%Y%m%d' `
echo $d
sh /home/dvctrader/s3_copy_scripts/get_data_for_date.sh $d > /home/dvctrader/s3_copy_scripts/x

cat /home/dvctrader/s3_copy_scripts/x  | awk '{ if ( (NR % 5) == 0 ) print $0 }' > /home/dvctrader/s3_copy_scripts/x5
cat /home/dvctrader/s3_copy_scripts/x  | awk '{ if ( (NR % 5) == 1 ) print $0 }' > /home/dvctrader/s3_copy_scripts/x1
cat /home/dvctrader/s3_copy_scripts/x  | awk '{ if ( (NR % 5) == 2 ) print $0 }' > /home/dvctrader/s3_copy_scripts/x2
cat /home/dvctrader/s3_copy_scripts/x  | awk '{ if ( (NR % 5) == 3 ) print $0 }' > /home/dvctrader/s3_copy_scripts/x3
cat /home/dvctrader/s3_copy_scripts/x  | awk '{ if ( (NR % 5) == 4 ) print $0 }' > /home/dvctrader/s3_copy_scripts/x4

sh /home/dvctrader/s3_copy_scripts/part.sh /home/dvctrader/s3_copy_scripts/x1 > /home/dvctrader/s3_copy_scripts/log1 2>&1 &
sh /home/dvctrader/s3_copy_scripts/part.sh /home/dvctrader/s3_copy_scripts/x2 > /home/dvctrader/s3_copy_scripts/log2 2>&1 &
sh /home/dvctrader/s3_copy_scripts/part.sh /home/dvctrader/s3_copy_scripts/x3 > /home/dvctrader/s3_copy_scripts/log3 2>&1 &
sh /home/dvctrader/s3_copy_scripts/part.sh /home/dvctrader/s3_copy_scripts/x4 > /home/dvctrader/s3_copy_scripts/log4 2>&1 &
sh /home/dvctrader/s3_copy_scripts/part.sh /home/dvctrader/s3_copy_scripts/x5 > /home/dvctrader/s3_copy_scripts/log5 2>&1 &

