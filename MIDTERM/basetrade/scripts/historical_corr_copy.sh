#!/bin/bash
modellinginfodir="/spare/local/tradeinfo/modellinginfo"
for shc in `cat /spare/local/tradeinfo/datageninfo/prod_list.txt`; do 
    summary_file=/NAS1/indicatorwork/"$shc"_32_fst1_na_e3_US_MORN_DAY_OfflineMixMMS_OfflineMixMMS/summary_indicator_corr_record_file.txt.gz
    hist_corr_file=$modellinginfodir/historical_corr_"$shc".gz 
    if [ -e $summary_file ]
        then 
            cp $summary_file $hist_corr_file
            gunzip -f $hist_corr_file 
    fi
done

shc=FDAX_0
hist_corr_file=$modellinginfodir/historical_corr_"$shc".gz
summary_file=/NAS1/indicatorwork/FDAX_0_32_fst1_na_e3_US_MORN_DAY_Midprice_OfflineMixMMS/summary_indicator_corr_record_file.txt.gz
if [ -e $summary_file ]
    then
        cp $summary_file $hist_corr_file
        gunzip -f $hist_corr_file
fi

shc=HHI_0
hist_corr_file=$modellinginfodir/historical_corr_"$shc".gz 
summary_file=/NAS1/indicatorwork/HHI_0_32_fst1_na_t3_AS_MORN_STB_MidPrice_MktSizeWPrice/summary_indicator_corr_record_file.txt.gz
if [ -e $summary_file ]
    then
        cp $summary_file $hist_corr_file
        gunzip -f $hist_corr_file
fi

shc=HSI_0
hist_corr_file=$modellinginfodir/historical_corr_"$shc".gz 
summary_file=/NAS1/indicatorwork/HSI_0_32_fst1_na_t3_AS_MORN_STB_MidPrice_MktSizeWPrice/summary_indicator_corr_record_file.txt.gz
if [ -e $summary_file ]
    then
        cp $summary_file $hist_corr_file
        gunzip -f $hist_corr_file
fi

shc=MHI_0
hist_corr_file=$modellinginfodir/historical_corr_"$shc".gz 
summary_file=/NAS1/indicatorwork/MHI_0_32_fst1_na_t3_AS_MORN_STB_MidPrice_MktSizeWPrice/summary_indicator_corr_record_file.txt.gz
if [ -e $summary_file ]
    then
        cp $summary_file $hist_corr_file
        gunzip -f $hist_corr_file
fi

shc=Si_0
hist_corr_file=$modellinginfodir/historical_corr_"$shc".gz 
summary_file=/NAS1/indicatorwork/Si_0_32_fsg1_na_e3_US_MORN_DAY_OfflineMixMMS_OfflineMixMMS/summary_indicator_corr_record_file.txt.gz
if [ -e $summary_file ]
    then
        cp $summary_file $hist_corr_file
        gunzip -f $hist_corr_file
fi

/home/dvctrader/infracore_install/scripts/sync_dir_to_all_dev_machines.pl $modellinginfodir
