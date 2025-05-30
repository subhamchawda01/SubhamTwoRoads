#!/bin/bash
source $HOME/.bash_switch_new_gcc ;

# call same script multiple times for resource utilization

for i in `seq 1 5`
do
~/basetrade_install/scripts/generate_global_results.pl 2>&1 &
sleep 3
done
