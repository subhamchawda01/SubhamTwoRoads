#!/bin/bash
today=`date --date='yesterday' +'%Y%m%d'`;
scenario_filename="/spare/local/tradeinfo/StructureInfo/scenarios/scenario."$today;
/home/dvctrader/basetrade/scripts/get_margins.py $today > $scenario_filename;

for loc in `grep push ~/basetrade/GenPerlLib/get_all_machines_vec.pl | awk '{ print $4; }' | grep "\." | awk -F\" '{ print $2; }'` ; do rsync -avz --delete-after /spare/local/tradeinfo/StructureInfo/scenarios/ dvctrader@$loc:/spare/local/tradeinfo/StructureInfo/scenarios ; done
