#!/bin/bash
for i in /apps/data/CMELoggedData/ /apps/data/EUREXLoggedData /apps/data/OSE_L1LoggedData /apps/data/HKEXLoggedData /apps/data/TSELoggedData /apps/data/RTSLoggedData /apps/data/MICEXLoggedData
do
find $i -type f -mtime +5 -exec rm {} \;
#find $i -type f -mtime +3
#find $i -type d -empty -exec rmdir {} \;
find $i -type d -empty -delete;
done

