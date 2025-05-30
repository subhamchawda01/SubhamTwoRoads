cd /spare/local/tradeinfo/PCAInfo/ ; 

yyyymmdd=`date +%Y%m%d`;

mkdir -p $HOME/locks ;
LOCKFILE=$HOME/locks/comp_pca_setup_$yyyymmdd".lock";
if [ ! -e $LOCKFILE ] ; then
touch $LOCKFILE;

RECENT_PCA_FILE=pca_portfolio_stdev_eigen_$yyyymmdd".txt";
RECENT_RECONCILED_PCA_FILE=pca_portfolio_stdev_eigen_reconciled_$yyyymmdd".txt";
DEFAULT_PCA_FILE=pca_portfolio_stdev_eigen_DEFAULT.txt;
PORTFOLIO_INFO_FILE=/spare/local/tradeinfo/PortfolioInfo/portfolio_constituent_info_$yyyymmdd.txt

# start with const
cat pca_portfolio_stdev_eigen_const.txt > $RECENT_PCA_FILE

# HK
NEW_PCA_FILE_TEMP_HK=pca_portfolio_stdev_eigen_$yyyymmdd".txt_HK";
if [ -e /spare/local/tradeinfo/PCAInfo/portfolio_inputs_non_const_sorted_HK ] ; then
    $HOME/basetrade/ModelScripts/generate_pca_coeffs_portfolios.pl /spare/local/tradeinfo/PCAInfo/portfolio_inputs_non_const_sorted_HK $NEW_PCA_FILE_TEMP_HK HKT_0900 HKT_1605 300;
    if [ -e $NEW_PCA_FILE_TEMP_HK ] ; then 
	cat $NEW_PCA_FILE_TEMP_HK >> $RECENT_PCA_FILE ;
    fi
fi

# EUS
NEW_PCA_FILE_TEMP_EUS=pca_portfolio_stdev_eigen_$yyyymmdd".txt_EUS";
if [ -e /spare/local/tradeinfo/PCAInfo/portfolio_inputs_non_const_sorted_EUS ] ; then
    $HOME/basetrade/ModelScripts/generate_pca_coeffs_portfolios.pl /spare/local/tradeinfo/PCAInfo/portfolio_inputs_non_const_sorted_EUS $NEW_PCA_FILE_TEMP_EUS CET_900 EST_1500 300 ;
    if [ -e $NEW_PCA_FILE_TEMP_EUS ] ; then 
	cat $NEW_PCA_FILE_TEMP_EUS >> $RECENT_PCA_FILE ;
    fi
fi

#OSE # break b/w start and end times handled by lower script
NEW_PCA_FILE_TEMP_OSE=pca_portfolio_stdev_eigen_$yyyymmdd".txt_OSE";
if [ -e /spare/local/tradeinfo/PCAInfo/portfolio_inputs_non_const_sorted_OSE ] ; then
    $HOME/basetrade/ModelScripts/generate_pca_coeffs_portfolios.pl /spare/local/tradeinfo/PCAInfo/portfolio_inputs_non_const_sorted_OSE $NEW_PCA_FILE_TEMP_OSE JST_1000 UTC_1800 300 ;
    if [ -e $NEW_PCA_FILE_TEMP_OSE ] ; then
        cat $NEW_PCA_FILE_TEMP_OSE >> $RECENT_PCA_FILE ;
    fi
fi

#verify if recent file is correct
#compare recent with default
if [ -e $RECENT_PCA_FILE ] ; then
    $HOME/basetrade/ModelScripts/sanity_check_pca.pl $DEFAULT_PCA_FILE $RECENT_PCA_FILE $RECENT_RECONCILED_PCA_FILE > pca_run_summary
    err_cnt=`grep -c ERROR pca_run_summary`
    SUB="PCA-RUN-SUMMARY-"$yyyymmdd ;
    /bin/mail -s $SUB "nseall@tworoads.co.in" < pca_run_summary  
    if [ $err_cnt != 0 ] ; then 
	cp $RECENT_PCA_FILE $RECENT_PCA_FILE"_backup";
	rm $RECENT_PCA_FILE;
    fi

    cp $RECENT_RECONCILED_PCA_FILE $RECENT_PCA_FILE;

    rm $RECENT_RECONCILED_PCA_FILE;

    /home/dvctrader/basetrade/scripts/portfolio_convertor.py $yyyymmdd > $PORTFOLIO_INFO_FILE 

    cp $RECENT_PCA_FILE $DEFAULT_PCA_FILE;
    
    for name in portfolio_inputs $DEFAULT_PCA_FILE $RECENT_PCA_FILE
    do 
	~/basetrade/scripts/sync_file_to_all_machines.pl $PWD/$name ;
    done
    ~/basetrade/scripts/sync_file_to_all_machines.pl $PORTFOLIO_INFO_FILE ;
fi


rm -f $LOCKFILE;
else
echo "$LOCKFILE present. Please delete";
fi


cd -
