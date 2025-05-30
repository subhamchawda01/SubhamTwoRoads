#!/bin/bash

source $HOME/.bash_switch_new_gcc ; 

hname=`hostname -s`; 
count_diff_=`$HOME/LiveExec/scripts/sort_and_diff_queues.sh $HOME/modelling/gsq_store/$hname $HOME/gsq/$hname | grep home | wc -l`;

if [ $count_diff_ -gt "0" ] ; then
    echo "$HOME/modelling/gsq_store/$hname $HOME/gsq/$hname not consistent. Please fix";
    $HOME/LiveExec/scripts/sort_and_diff_queues.sh $HOME/modelling/gsq_store/$hname $HOME/gsq/$hname | grep home
else
    $HOME/LiveExec/ModelScripts/run_next_gen_strat.pl
fi
