#!/bin/bash

if [ $# -lt 2 ]; then echo "USAGE : $0 DATE TID"; exit; fi;

dt=$1;
tid=$2;
tfile=tmp_"$tid";

sed '/getflat_due_to_close_/q' /spare/local/logs/tradelogs/log."$dt"."$tid" > $tfile;
echo "BestInvalidOffers: " `grep OnRetailOfferUpdate $tfile | grep Invalid | wc -l`;
echo "NonBestInvalidOffers: " `grep OnRetailOfferUpdate $tfile | grep -v 'ORDER\|Invalid' | awk 'BEGIN{prev=0} {if($1>prev){if(($(NF-1)<=0) || ($(NF-2)==0)){prev=$1; print "ERROR"}}}' | wc -l`;
echo "AvgOfferUpdateFreq: " `grep OnRetailOfferUpdate $tfile | grep -v 'ORDER\|Invalid' | awk 'BEGIN{sum=0;} {if(NR==1){prev=$1;} if($1>prev){diff=($1-prev); sum+=diff; prev=$1;}} END{print 2*sum/NR}'`;
echo "OFFERED_SPREADS";
grep OnRetailOfferUpdate $tfile | grep -v 'ORDER\|Invalid' | awk 'BEGIN{sum=0; prev=0;} {if($1>prev){diff=($(NF-1) - $(NF-2)); sum+=diff; print diff; prev=$1;}} END{print "AVG_OFFERED_SPREAD:", 2*sum/NR}' | sort -nk1 | uniq -c;
echo "GetFlatsPos: " `grep "getflat.*pos" $tfile | wc -l`;
echo "MaxPos: " `awk 'NF>6{if($7>0){print $7}else{print -1*$7}}' /spare/local/logs/tradelogs/trades."$dt"."$tid" | sort -nk1 | tail -n1`;
~/basetrade/ModelScripts/get_pnl_stats.pl /spare/local/logs/tradelogs/trades."$dt"."$tid";
rm $tfile;
