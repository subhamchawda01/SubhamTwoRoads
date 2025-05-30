#!/bin/bash

mkdir -p ~/hrishav/;

for shc in `ls /home/dvctrader/modelling/wf_staged_strats`; do 
  for timeperiod in `ls /home/dvctrader/modelling/wf_staged_strats/$shc`; do 
    echo ~/basetrade/scripts/prune_strats_pool.pl $shc $timeperiod ~/modelling/prune_strats_config/config.default_staged.txt STAGED; 
  done; 
done > ~/hrishav/pruning_cmds ;

/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/run_my_job.py -n dvctrader -m 1 -f ~/hrishav/pruning_cmds ;
