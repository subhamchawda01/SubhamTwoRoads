#!/bin/bash

SHC=CGB_0;
if [ $# -lt 1 ] ; then echo "need shortcode"; exit; fi

SHC=$1; shift;
mkdir -p $HOME/fbmwork/$SHC; if [ -e $HOME/fbmwork/$SHC/lockfile ] ; then exit ; else touch $HOME/fbmwork/$SHC/lockfile ; fi
for stratpath in `cat ~/fbmwork/$SHC""_strat_order.txt`; 
do 
    if [ -e $stratpath ] ; then
    stratbase=`basename $stratpath`; 

    algo_str_= "80_2_10_gradalpha10";
    stdoutfile=$HOME/fbmwork/$SHC/stdoutfile_$algo_str_"_"$stratbase;
    stderrfile=$HOME/fbmwork/$SHC/stderrfile_$algo_str_"_"$stratbase;
    optmodelpath=$HOME/fbmwork/$SHC/opt_model_$algo_str_"_"$stratbase;
#    $HOME/basetrade/ModelScripts/find_best_model_for_strategy.pl $stratpath $optmodelpath TODAY-5 80 2 gradalpha10 D > $stdoutfile 2>$stderrfile ; 
    $HOME/basetrade/ModelScripts/find_best_model_for_strategy_2.pl $stratpath $optmodelpath TODAY-5 80 2 10 gradalpha10 D kCNASqrtPnlVolBySqrtDDTTC > $stdoutfile 2>$stderrfile ;

    algo_str_= "80_2_20_gradalpha3";
    stdoutfile=$HOME/fbmwork/$SHC/stdoutfile_$algo_str_"_"$stratbase;
    stderrfile=$HOME/fbmwork/$SHC/stderrfile_$algo_str_"_"$stratbase;
    optmodelpath=$HOME/fbmwork/$SHC/opt_model_$algo_str_"_"$stratbase;
#    $HOME/basetrade/ModelScripts/find_best_model_for_strategy.pl $stratpath $optmodelpath TODAY-5 80 2 gradalpha3 D > $stdoutfile 2>$stderrfile ; 
    $HOME/basetrade/ModelScripts/find_best_model_for_strategy_2.pl $stratpath $optmodelpath TODAY-5 80 2 20 gradalpha3 D kCNASqrtPnlVolBySqrtDDTTC > $stdoutfile 2>$stderrfile ;

    algo_str_= "80_2_10_best1";
    stdoutfile=$HOME/fbmwork/$SHC/stdoutfile_$algo_str_"_"$stratbase;
    stderrfile=$HOME/fbmwork/$SHC/stderrfile_$algo_str_"_"$stratbase;
    optmodelpath=$HOME/fbmwork/$SHC/opt_model_$algo_str_"_"$stratbase;
#    $HOME/basetrade/ModelScripts/find_best_model_for_strategy.pl $stratpath $optmodelpath TODAY-5 80 2 best1 D > $stdoutfile 2>$stderrfile ; 
    $HOME/basetrade/ModelScripts/find_best_model_for_strategy_2.pl $stratpath $optmodelpath TODAY-5 80 2 10 best1 D kCNASqrtPnlVolBySqrtDDTTC > $stdoutfile 2>$stderrfile ;

    fi
done
rm -f $HOME/fbmwork/$SHC/lockfile
