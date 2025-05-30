#!/bin/bash

if [ $# -gt 0 ] ; then
    mkdir -p $HOME"/prod_strats"

    SHC=$1; shift;
    for name in `$HOME/basetrade/scripts/rank_hist_queries.pl US $SHC TODAY-10 TODAY-1 2>/dev/null | awk '{ if ( $4 >= 1 ) { print $3; } }'`; do 
	FILE=`$HOME/basetrade/scripts/print_strat_from_base.sh $name 2>/dev/null`;
	if [ $FILE ] && [ -f $FILE ] ;
	then
	    cat $FILE;
	fi
    done > $HOME/"prod_strats/"$SHC

    head -n 6 $HOME/"prod_strats/"$SHC > $HOME/"prod_strats/"$SHC".1";
    mv $HOME/"prod_strats/"$SHC".1" $HOME/"prod_strats/"$SHC;

	# for name in `$HOME/basetrade/scripts/rank_hist_queries.pl EU $SHC TODAY-10 TODAY-1 2>/dev/null | awk '{ if ( $4 >= 1 ) { print $3; } }'`; do 
	#     FILE=`$HOME/basetrade/scripts/print_strat_from_base.sh $name 2>/dev/null`;
	#     if [ $FILE ] && [ -f $FILE ] ;
	#     then
	# 	cat $FILE;
	#     fi
	# done > $HOME/"prod_strats/"$SHC".2";

	# head -n 6 $HOME/"prod_strats/"$SHC".2" >> $HOME/"prod_strats/"$SHC;
	# rm -f $HOME/"prod_strats/"$SHC".2"

else
    mkdir -p $HOME"/prod_strats"

    for SHC in FGBS_0 FGBL_0 FESX_0 FGBM_0 ZF_0 ZN_0 ZB_0 UB_0 CGB_0 BAX_2 BAX_3 BAX_4 BAX_5 BR_DOL_0 BR_IND_0 BR_WIN_0 ; do 
	for name in `$HOME/basetrade/scripts/rank_hist_queries.pl US $SHC TODAY-10 TODAY-1 2>/dev/null | awk '{ if ( $4 >= 1 ) { print $3; } }'`; do 
	    FILE=`$HOME/basetrade/scripts/print_strat_from_base.sh $name 2>/dev/null`;
	    if [ $FILE ] && [ -f $FILE ] ;
	    then
		cat $FILE;
	    fi
	done > $HOME/"prod_strats/"$SHC

	head -n 6 $HOME/"prod_strats/"$SHC > $HOME/"prod_strats/"$SHC".1";
	mv $HOME/"prod_strats/"$SHC".1" $HOME/"prod_strats/"$SHC;

	# for name in `$HOME/basetrade/scripts/rank_hist_queries.pl EU $SHC TODAY-10 TODAY-1 2>/dev/null | awk '{ if ( $4 >= 1 ) { print $3; } }'`; do 
	#     FILE=`$HOME/basetrade/scripts/print_strat_from_base.sh $name 2>/dev/null`;
	#     if [ $FILE ] && [ -f $FILE ] ;
	#     then
	# 	cat $FILE;
	#     fi
	# done > $HOME/"prod_strats/"$SHC".2";

	# head -n 6 $HOME/"prod_strats/"$SHC".2" >> $HOME/"prod_strats/"$SHC;
	# rm -f $HOME/"prod_strats/"$SHC".2"

    done

fi
