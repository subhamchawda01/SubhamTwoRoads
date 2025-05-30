#!/bin/bash

export NEW_GCC_DIR=/usr/local/bin
export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64

export BOOST_ROOT=/apps/boost/new_gcc_root
export PATH=$BOOST_ROOT:$NEW_GCC_DIR:$PATH
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

USAGE="$0 FORCED[default not forced]"
forced=0;
if [ "$1" = "FORCED" ]; then
    forced=1;
    echo "Oosps I am forced";
fi
cd /spare/local/tradeinfo/PCAInfo/ ; 
yyyymmdd=`date +%Y%m%d`;
mkdir -p $HOME/locks
LOCKFILE=$HOME/locks/comp_pca_stdev_$yyyymmdd.lock
if [ -e $LOCKFILE ]; then
    echo "$LOCKFILE present. Please remove and rerun."; 
    cd -; exit 1;
fi

last_file_date=`ls pca_portfolio_stdev_[0-9]*.txt | sort -nk5 -t'_' 2>/dev/null | tail -n1 | cut -d'_' -f4 | cut -d'.' -f1`;
echo $last_file_date;
if [ $forced -eq 0 ]; then 
    if [ ! -z "$last_file_date" ]; then
	next_run_date=`date +%Y%m%d -d "$last_file_date + 120 days"`
	if [ $next_run_date -gt $yyyymmdd ] ; then 
	    echo "Hey you are running pca_stdev_script too early. Should be run after ". `date -d "$last_file_date + 120 days"`;
	    echo "You have to force it if you want to run today. $USAGE";
	    exit 1;
	fi;
    fi;
fi

touch $LOCKFILE

RECENT_PCA_FILE="/spare/local/tradeinfo/PCAInfo/"pca_portfolio_stdev_$yyyymmdd".txt";
DEFAULT_PCA_FILE=pca_portfolio_stdev_DEFAULT.txt;
PORTFOLIO_INFO_FILE=/spare/local/tradeinfo/PortfolioInfo/portfolio_constituent_info_$yyyymmdd.txt

# start with const
cat pca_portfolio_stdev_eigen_const.txt | grep "PORTFOLIO_STDEV" > $RECENT_PCA_FILE


#OSE # break b/w start and end times handled by lower script
NEW_PCA_FILE_TEMP_OSE=pca_portfolio_stdev_$yyyymmdd".txt_OSE";
if [ -e /spare/local/tradeinfo/PCAInfo/portfolio_inputs_non_const_sorted_OSE ] ; then
    $HOME/basetrade/ModelScripts/generate_pca_coeffs_portfolios_stdev_only.pl /spare/local/tradeinfo/PCAInfo/portfolio_inputs_non_const_sorted_OSE $NEW_PCA_FILE_TEMP_OSE JST_1000 UTC_1800 300 1 | grep PORTFOLIO_STDEV > $NEW_PCA_FILE_TEMP_OSE;
    if [ -e $NEW_PCA_FILE_TEMP_OSE ] ; then
        cat $NEW_PCA_FILE_TEMP_OSE >> $RECENT_PCA_FILE ;
    fi
fi

# HK
NEW_PCA_FILE_TEMP_HK=pca_portfolio_stdev_$yyyymmdd".txt_HK";
if [ -e /spare/local/tradeinfo/PCAInfo/portfolio_inputs_non_const_sorted_HK ] ; then
    $HOME/basetrade/ModelScripts/generate_pca_coeffs_portfolios_stdev_only.pl /spare/local/tradeinfo/PCAInfo/portfolio_inputs_non_const_sorted_HK INVALID_FILE HKT_0900 HKT_1605 300 1 | grep PORTFOLIO_STDEV > $NEW_PCA_FILE_TEMP_HK ; # 1 - stdev only
    if [ -e $NEW_PCA_FILE_TEMP_HK ] ; then 
	cat $NEW_PCA_FILE_TEMP_HK >> $RECENT_PCA_FILE ;
    fi
fi

# EUS
NEW_PCA_FILE_TEMP_EUS=pca_portfolio_stdev_$yyyymmdd".txt_EUS";
if [ -e /spare/local/tradeinfo/PCAInfo/portfolio_inputs_non_const_sorted_EUS ] ; then
    $HOME/basetrade/ModelScripts/generate_pca_coeffs_portfolios_stdev_only.pl /spare/local/tradeinfo/PCAInfo/portfolio_inputs_non_const_sorted_EUS $NEW_PCA_FILE_TEMP_EUS CET_900 EST_1500 300 1 | grep PORTFOLIO_STDEV > $NEW_PCA_FILE_TEMP_EUS;
    if [ -e $NEW_PCA_FILE_TEMP_EUS ] ; then 
	cat $NEW_PCA_FILE_TEMP_EUS >> $RECENT_PCA_FILE ;
    fi
fi

# TODO: the sanity check
if [ -e $RECENT_PCA_FILE ]; then
    # cp $RECENT_PCA_FILE $DEFAULT_PCA_FILE ;
    ~/basetrade/scripts/sync_file_to_all_machines.pl $RECENT_PCA_FILE;
    # ~/basetrade/scripts/sync_file_to_all_machines.pl $DEFAULT_PCA_FILE;
fi
rm -f $LOCKFILE
