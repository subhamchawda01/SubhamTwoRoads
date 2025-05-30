#!/bin/bash

shc_list=($(~/basetrade/scripts/shc_list_from_retailstrat.sh $1));
id_list=($(~/basetrade/scripts/strat_ids_from_retailstrat.sh $1));

for (( i=0; i<${#id_list[@]}; i++ )); do
  awk -vid=${id_list[$i]} -vshc=${shc_list[$i]} 'BEGIN{s=".*\\."id; } { if($3 ~ s){ v+=$5; if($NF != "0"){p=$NF;} else{p=$9;} } } END{if(v>0){print shc, p - 0.1*v, v}}' $2;
done
