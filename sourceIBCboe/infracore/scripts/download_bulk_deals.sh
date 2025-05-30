#!/bin/bash

if [ "$1" == "TODAY" ];then
        Date=`date +"%Y%m%d"`;
else
        Date=$1;
fi

today=`date -d${Date} +%d-%b-%Y`
date_=`date -d${Date} +%Y%m%d`
echo "date: $Date : $today : $date_"

bulk_deal_file_="/spare/local/tradeinfo/NSE_Files/BulkDeals/bulk_deals_${date_}"

echo $bulk_deal_file_
[[ $2 == "FORCE" ]] && `rm -rf $bulk_deal_file_`

[ -f $bulk_deal_file_ ] && [[ `wc -l $bulk_deal_file_ | cut -d' ' -f1` != "1" ]] && exit
echo "#DEAL_TYPE SYMBOL BS QTY_TRD PRICE" >$bulk_deal_file_

#/usr/bin/curl "https://www1.nseindia.com/products/dynaContent/equities/equities/bulkdeals.jsp?symbol=&segmentLink=13&symbolCount=&dateRange=&fromDat=${today}&toDate=${today}&dataType=DEALS" -H 'User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:88.0) Gecko/20100101 Firefox/88.0' -H 'Accept: */*' --compressed -H 'X-Requested-With: XMLHttpRequest' | grep -i -e '</\?TABLE\|</\?TD\|</\?TR\|</\?TH' | sed 's/^[\ \t]*//g' | tr -d '\n\r' | sed 's/<\/TR[^>]*>/\n/Ig' | sed 's/<\/\?\(TABLE\|TR\)[^>]*>//Ig' | sed 's/^<T[DH][^>]*>\|<\/\?T[DH][^>]*>$//Ig' | sed 's/<\/T[DH][^>]*><T[DH][^>]*>/#/Ig' >/tmp/ppp # |  awk -F# '{print $2 $6 $7}' | sort -u >${bulk_deal_file_}


/usr/bin/curl "https://www1.nseindia.com/products/dynaContent/equities/equities/blockdeals.jsp?symbol=&segmentLink=12&symbolCount=&dateRange=week&fromDat=&toDate=&dataType=BLOCK" -H 'User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:88.0) Gecko/20100101 Firefox/88.0' -H 'Accept: */*' --compressed -H 'X-Requested-With: XMLHttpRequest' >/tmp/block_deal_file_${date_}
echo "block_done"
/usr/bin/curl "https://www1.nseindia.com/products/dynaContent/equities/equities/bulkdeals.jsp?symbol=&segmentLink=13&symbolCount=&dateRange=week&fromDat=&toDate=&dataType=DEALS" -H 'User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:88.0) Gecko/20100101 Firefox/88.0' -H 'Accept: */*' --compressed -H 'X-Requested-With: XMLHttpRequest' >/tmp/bulk_deal_file_${date_}

echo "bulk_done"
grep -i -e '</\?TABLE\|</\?TD\|</\?TR\|</\?TH' /tmp/block_deal_file_${date_} | sed 's/^[\ \t]*//g' | tr -d '\n\r' | sed 's/<\/TR[^>]*>/\n/Ig' | sed 's/<\/\?\(TABLE\|TR\)[^>]*>//Ig' | sed 's/^<T[DH][^>]*>\|<\/\?T[DH][^>]*>$//Ig' | sed 's/<\/T[DH][^>]*><T[DH][^>]*>/#/Ig' | grep $today | awk -F# '{print "BLOCK "$2,$5,$6,$7}' | sort -u >>${bulk_deal_file_}

echo "block grep _done"
grep -i -e '</\?TABLE\|</\?TD\|</\?TR\|</\?TH' /tmp/bulk_deal_file_${date_} | sed 's/^[\ \t]*//g' | tr -d '\n\r' | sed 's/<\/TR[^>]*>/\n/Ig' | sed 's/<\/\?\(TABLE\|TR\)[^>]*>//Ig' | sed 's/^<T[DH][^>]*>\|<\/\?T[DH][^>]*>$//Ig' | sed 's/<\/T[DH][^>]*><T[DH][^>]*>/#/Ig' | grep $today | awk -F# '{print "BULK "$2,$5,$6,$7}' | sort -u >>${bulk_deal_file_}
echo "bulk grep_done"

[ -f $bulk_deal_file_ ] && [[ `wc -l $bulk_deal_file_ | cut -d' ' -f1` != "1" ]] && echo "" |  mailx -s "BULK DEAL IS UPDATED : ${date_}" -r $HOSTNAME hardik.dhakate@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in

