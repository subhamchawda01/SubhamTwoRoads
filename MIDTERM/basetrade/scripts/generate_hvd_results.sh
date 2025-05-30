#!/bin/bash
num_days=362;

YYYYMMDD=`date +%Y%m%d`;

for algo in kCNAPnlAdjAverage kCNAGainPainRatio kCNAPnlByMaxloss kCNAPnlSqrtDD; 
do  
   shc="HHI_0";
   start_end_hhmm="HKT_935-HKT_1155";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
      
   shc="HHI_0";
   start_end_hhmm="HKT_1300-HKT_1605";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
      
   shc="HSI_0";
   start_end_hhmm="HKT_935-HKT_1155";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
      
   shc="HSI_0";
   start_end_hhmm="HKT_1300-HKT_1605";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
      

   shc="NK_0";
   start_end_hhmm="EST_800-UTC_1730";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
      
   shc="NK_0";
   start_end_hhmm="JST_901-JST_1500";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
      
   shc="NK_0";
   start_end_hhmm="JST_1700-EST_800";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
      
   shc="NKM_0";
   start_end_hhmm="EST_800-UTC_1730";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
      
   shc="NKM_0";
   start_end_hhmm="JST_901-JST_1500";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
      
   shc="NKM_0";
   start_end_hhmm="JST_1700-EST_800";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
      

   shc="FGBM_0";
   start_end_hhmm="CET_800-EST_800";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
      
   shc="FGBL_0";
   start_end_hhmm="CET_800-EST_800";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
      
   shc="FGBX_0";
   start_end_hhmm="CET_800-EST_800";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
      
   shc="FGBS_0";
   start_end_hhmm="CET_800-EST_800";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;    

   shc="LFR_0";
   start_end_hhmm="BST_810-EST_800";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;      

   shc="FOAT_0";
   start_end_hhmm="CET_800-EST_800";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="FBTP_0";
   start_end_hhmm="CET_800-EST_800";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="LFZ_0";
   start_end_hhmm="CET_800-EST_800";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="FESX_0";
   start_end_hhmm="CET_800-EST_800";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="ZN_0";
   start_end_hhmm="CET_800-EST_800";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="ZF_0";
   start_end_hhmm="CET_800-EST_800";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="ZB_0";
   start_end_hhmm="CET_800-EST_800";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="FGBM_0";
   start_end_hhmm="EST_800-CET_1900";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
   
   shc="FGBS_0";
   start_end_hhmm="EST_800-CET_1900";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;    

   shc="FGBL_0";
   start_end_hhmm="EST_800-CET_1900";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
      
   shc="FGBX_0";
   start_end_hhmm="EST_800-CET_1900";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
      
   shc="LFR_0";
   start_end_hhmm="EST_800-CET_1900";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;      

   shc="LFZ_0";
   start_end_hhmm="EST_800-CET_1900";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="FOAT_0";
   start_end_hhmm="EST_800-CET_1900";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="FBTP_0";
   start_end_hhmm="EST_800-CET_1900";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="FESX_0";
   start_end_hhmm="EST_800-CET_1900";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="ZN_0";
   start_end_hhmm="EST_800-EST_1600";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="ZF_0";
   start_end_hhmm="EST_800-EST_1600";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="ZB_0";
   start_end_hhmm="EST_800-EST_1600";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="UB_0";
   start_end_hhmm="EST_800-EST_1600";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="BR_DOL_0";
   start_end_hhmm="EST_800-BRT_1750";
   max_loss_per_uts="800";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$max_loss_per_uts"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" MAX_LOSS_PER_UTS = $max_loss_per_uts > $output_file;
     
   shc="BR_WIN_0";
   start_end_hhmm="BRT_1000-BRT_1750";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="BR_IND_0";
   start_end_hhmm="BRT_1000-BRT_1750";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="DI1F15";
   start_end_hhmm="BRT_900-BRT_1750";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="DI1F16";
   start_end_hhmm="BRT_900-BRT_1750";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="DI1F17";
   start_end_hhmm="BRT_900-BRT_1750";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="DI1F18";
   start_end_hhmm="BRT_900-BRT_1750";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
  
   shc="DI1F21";
   start_end_hhmm="BRT_900-BRT_1750";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="SXF_0";
   start_end_hhmm="EST_930-EST_1613";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;

   shc="CGB_0";
   start_end_hhmm="EST_730-EST_1455";
   output_file="/home/dvctrader/HVD_results/HVD_res_"$shc"_"$start_end_hhmm"_"$num_days"_"$algo"_"$YYYYMMDD;
   /home/dvctrader/basetrade_install/ModelScripts/summarize_strats_for_specific_days.pl $shc $start_end_hhmm $num_days SORT_ALGO = $algo FEATURES_MEDIAN_FACTOR = "VOL 1.0 HIGH" > $output_file;
done

find /home/dvctrader/HVD_results -mtime +1 -exec rm -f {} \;
