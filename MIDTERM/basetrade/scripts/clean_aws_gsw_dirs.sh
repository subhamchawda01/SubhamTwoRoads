#!/bin/bash

#find /spare/local/$USER/GSW/* -type d -mtime +4 -exec rm -rf {} \;
find /spare/local/$USER/CombineIndicators/* -type d -mmin +4320 -exec rm -rf {} \;

find /spare/local/DailyTimedDataDir/ /spare/local/DailyRegDataDir /spare/local/RegDataDir -type f -amin +1440 -exec rm -f {} \;
find /spare/local/dvctrader/DailyTimedDataDir/ /spare/local/dvctrader/DailyRegDataDir /spare/local/dvctrader/RegDataDir -type f -amin +1440 -exec rm -f {} \;

if [ -d /media/disk-ephemeral2/indicatorwork ]
then
    find /media/disk-ephemeral2/indicatorwork/ -atime +20 -exec rm -rf {} \;
fi

for i in 0 1 2 3
do
    #find /spare$i/local/$USER/GSW/* -type d -mtime +4 -exec rm -rf {} \;

    find /spare$i/local/DailyTimedDataDir/ /spare$i/local/DailyRegDataDir /spare$i/local/RegDataDir -type f -amin +1440 -exec rm -f {} \;
    find /spare$i/local/dvctrader/DailyTimedDataDir/ /spare$i/local/dvctrader/DailyRegDataDir /spare$i/local/dvctrader/RegDataDir -type f -amin +1440 -exec rm -f {} \;
done
