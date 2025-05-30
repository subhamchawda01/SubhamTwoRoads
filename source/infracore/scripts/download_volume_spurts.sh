#!/bin/bash
tmp_volume_spurts=/home/dvctrader/trash/volume_spurts_json
tmp_csv_volume_spurts=/home/dvctrader/trash/volume_spurts_csv
VOL_DIR=/spare/local/tradeinfo/NSE_Files/Volume_Spurts
rm $tmp_csv_volume_spurts

today_=`date +"%Y%m%d"`
FILE=$VOL_DIR/volume_gainer_${today_}

/usr/bin/curl 'https://www1.nseindia.com/live_market/dynaContent/live_analysis/volume_spurts/volume_spurts.json' -H 'User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:88.0) Gecko/20100101 Firefox/88.0' -H 'Accept: */*' --compressed -H 'X-Requested-With: XMLHttpRequest' >$tmp_volume_spurts

# file empty exist 
if [ ! -s $tmp_volume_spurts ];then
  echo "FILE EMPTY/....";
  exit
fi

jq -r '.data | map(.sym), map(.turn_lkh), map(.week1a), map(.week1vc), map(.week2a), map(.week2vc), map(.ltp), map(.netpr), map(.value) | @csv' $tmp_volume_spurts | sed --expression='s/","/ /'g > $tmp_csv_volume_spurts

awk '
{ 
      for (i=1; i<=NF; i++)  {
                a[NR,i] = $i
                        }
}
NF>p { p = NF }
END {    
      for(j=1; j<=p; j++) {
                str=a[1,j]
                            for(i=2; i<=NR; i++){
                                          str=str" "a[i,j];
                                                  }
                        print str
                                }
}' $tmp_csv_volume_spurts > $FILE

cp $FILE $tmp_csv_volume_spurts

cat $tmp_csv_volume_spurts | tr -d '",' >$FILE

rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.81:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.82:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.83:/spare/local/tradeinfo --delete-after
