#!/bin/bash
today=`date --date='yesterday' +'%Y%m%d'`;

if [ $# -gt 0 ];
then
   today=$1; 
fi        

starttime="BRT_930";
endtime="BRT_1530";

shortcodes="DI1F13 DI1J13 DI1N13 DI1V13 DI1F14 DI1J14 DI1N14 DI1V14 DI1F15 DI1J15 DI1N15 DI1V15 DI1F16 DI1J16 DI1N16 DI1V16 DI1F17 DI1J17 DI1N17 DI1V17 DI1F18 DI1J18 DI1N18 DI1V18 DI1F19 DI1J19 DI1N19 DI1V19 DI1F20 DI1J20 DI1N20 DI1V20 DI1F21 DI1J21 DI1N21 DI1V21 DI1F22 DI1J22 DI1N22 DI1V22 DI1F23 DI1J23 DI1N23 DI1V23 DI1F24 DI1J24 DI1N24 DI1V24 DI1F25 DI1J25 DI1N25 DI1V25";

pxs_filename="/spare/local/tradeinfo/StructureInfo/prices/prices."$today;


/home/dvctrader/basetrade/ModelScripts/generate_last_days_di1_prices.pl $today $starttime $endtime $pxs_filename $shortcodes

for loc in `grep push ~/basetrade/GenPerlLib/get_all_machines_vec.pl | awk '{ print $4; }' | grep "\." | awk -F\" '{ print $2; }'` ; do rsync -avz --delete-after /spare/local/tradeinfo/StructureInfo/prices/ dvctrader@$loc:/spare/local/tradeinfo/StructureInfo/prices ; done
