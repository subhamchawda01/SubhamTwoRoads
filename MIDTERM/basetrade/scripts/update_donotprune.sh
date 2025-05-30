#!/bin/bash

if [ $USER == "rahul" ]
then
    mkdir -p ~/choices
    products=(FOAT_0 FGBS_0 FGBM_0 FGBL_0 FESX_0 FDAX_0 ZT_0 ZF_0 ZN_0 ZB_0 UB_0 CGB_0 BAX_0 BAX_1 BAX_2 BAX_3 BAX_4 BAX_5 BR_DOL_0 BR_IND_0 BR_WIN_0 DI1F16 DI1F19 DI1F17 DI1N14 DI1F18)
    if [ $# -ge 1 ]; then products=($1);fi
    if [ $# -ge 2 ]; then cutoff=$2; fi

    t_folder=$HOME/tmp_rank_folder/
    mkdir -p $t_folder
    for SHC in ${products[*]}; do
	~/basetrade/scripts/rank_hist_queries.pl US $SHC TODAY-40 TODAY-1 2>/dev/null > $t_folder/rank_file_$SHC;
	l=`wc -l $t_folder/rank_file_$SHC | cut -d' ' -f1`;
	c=0.2; if [ $l -gt 30 ]; then c=0.4; fi
	ol=`echo "($l * $c + 0.5)/1" | bc`;
	if [ $ol -gt 30 ]; then ol=30; fi;
	echo "Prod: $SHC     Lines to remove: $ol";
	if [ $cutoff ]; then awk '{if($1>$cutoff) print $3}' $t_folder/rank_file_$SHC > $HOME/choices/DONOTPRUNE.$SHC;
	else awk '{print $3, $1*$4}' $t_folder/rank_file_$SHC  | sort -nr -k2 | head -n -$ol > $HOME/choices/DONOTPRUNE.$SHC
	fi
    done
else
    if [ $# -gt 0 ] ; then
	mkdir -p $HOME"/choices"

	SHC=$1; shift;
	for name in `~/basetrade/scripts/rank_hist_queries.pl US $SHC TODAY-250 TODAY-1 | awk '{print $3}'`; do $HOME/basetrade/scripts/print_strat_from_base.sh $name 2>/dev/null ; done > $HOME/choices/DONOTPRUNE.$SHC

    else
	mkdir -p $HOME"/choices"

	for SHC in \
	BR_IND_0 ; do 
	    echo " >>> "$SHC;
	    for name in `~/basetrade/scripts/rank_hist_queries.pl EU $SHC TODAY-30 TODAY-1 | awk '{print $3}'`; do $HOME/basetrade/scripts/print_strat_from_base.sh $name 2>/dev/null ; done > $HOME/choices/DONOTPRUNE.$SHC
	done

    fi
fi
