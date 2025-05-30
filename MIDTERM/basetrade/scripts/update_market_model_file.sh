#!/bin/bash
cur_date=`date +%Y%m%d`
base_dir=/home/pengine/codebase/devmodel

#compute new delays
$base_dir/GenPerlLib/update_sim_delay_values.pl $cur_date /home/dvctrader/basetrade/OfflineConfigs/MarketModelInfo/market_model_info.txt > /tmp/new_market_model.txt

#update repo with the new mm file

cp /tmp/new_market_model.txt $base_dir/OfflineConfigs/MarketModelInfo/market_model_info.txt
cd $base_dir
git add OfflineConfigs/MarketModelInfo/market_model_info.txt
git commit -m "auto market_model update $cur_date"
git push origin devmodel
