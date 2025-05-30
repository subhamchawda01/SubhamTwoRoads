#!/bin/bash
cur_time=`date`
wiki_up=`wget -O - --timeout 10 tworoads.co.in/mediawiki 2>/dev/null | grep -c "Login required"`
if [ $wiki_up -le 0 ]; then
  /bin/mail -s "URGENT: wiki down now $cur_time" chandan.kumar@tworoads.co.in < /dev/null;
fi
simula_up=`/usr/bin/curl -m 30 -X POST -d "SHC=FGBM_0&SD=20151101&ED=20151110" "http://10.0.1.15:3000/pulldates" 2>/dev/null | grep -c 2015`
if [ $simula_up -le 0 ]; then
  /bin/mail -s "URGENT: simula backend down now $cur_time" chandan.kumar@tworoads.co.in < /dev/null;
fi
main_simula_up=`wget -O - --timeout 10 tworoads.co.in/simula.php 2>/dev/null | grep -c "DVC Simula Login"`
if [ $main_simula_up -le 0 ]; then
  /bin/mail -s "URGENT: simula frontend down now $cur_time" chandan.kumar@tworoads.co.in < /dev/null;
fi
