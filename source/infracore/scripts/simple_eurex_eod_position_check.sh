#! /bin/bash

yday=`cat /tmp/YESTERDAY_DATE`;
if [ $# -gt 0 ]; then yday=`echo -n $1` ; fi

SUBJECT="Eurex_Positions "$yday;

if [ -e /NAS1/logs/ExchangeFiles/EUREX/$yday/00RPTTC810DVCNJ$yday"FIMFR.TXT.gz" ] ; then
    mailbody=`zgrep "Product Total\|DVCNJ" /NAS1/logs/ExchangeFiles/EUREX/$yday/00RPTTC810DVCNJ$yday"FIMFR.TXT.gz" | awk '{if ( $1 == "DVCNJ" ) { prodn=$4;} else { if ( $3 == "Buy" ) { buya=substr($6,1,length($6)-1); } else { sella=substr($6,1,length($6)-1); if(buya != sella){print "\nProduct: " prodn " BUYS: " buya " SELLS: " sella;} } }}'`;
fi

if [ -e /NAS1/logs/ExchangeFiles/EUREX/$yday/00RPTTC810DVCNJ$yday"FIMFR.TXT" ] ; then 
    mailbody=`egrep "Product Total|DVCNJ" /NAS1/logs/ExchangeFiles/EUREX/$yday/00RPTTC810DVCNJ$yday"FIMFR.TXT" | awk '{if ( $1 == "DVCNJ" ) { prodn=$4;} else { if ( $3 == "Buy" ) { buya=substr($6,1,length($6)-1); } else { sella=substr($6,1,length($6)-1); if(buya != sella){print "\nProduct: " prodn " BUYS: " buya " SELLS: " sella;} } }}'`;
fi
if [ -n "$mailbody" ]; then
    echo "$mailbody" | /bin/mail -s "$SUBJECT" -r "dvcinfra@ny11" "nseall@tworoads.co.in";

    # tty -s >/dev/null
    # if [ $? -eq 0 ] ; then echo "$mailbody"; fi
fi
