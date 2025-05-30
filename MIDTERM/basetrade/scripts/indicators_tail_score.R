#!/usr/bin/env Rscript

args <- commandArgs(trailingOnly = TRUE);

if ( length ( args ) < 8 )
{
  stop ( " Usage: <model_file feature_ilist begin_date end_date begin_time end_time pred_dur sd_fraction use_weights= 1 use_sum_vars = 1  sampling_time =1000  dir = /media/ephemeral2/  delta_y_cutoff = 0( in ticks) \n " ) ;
}

i = 1
model_file = args[i];
i = i + 1;
feature_ilist = args[i];
i= i +1 ;
begin_date = args[i];
i = i + 1;
end_date = args[i];
i = i + 1;
begin_time = args[i];
i = i + 1;
end_time = args[i];
i = i + 1;
pred_dur = as.numeric(args[i]);
i = i + 1;
sd_fac = as.numeric(args[i]);
i = i + 1;
use_weights = FALSE;
use_sum_vars = TRUE;
sampling_time =1000;
dir = "/media/ephemeral2/";
delta_y_cutoff = 0;

 if(length(args)  > i - 1 ){
   use_weights = ( as.numeric(args[i]) != 0 ) ;
   i = i +1 
 }

 if(length(args)  > i - 1 ){
   use_sum_vars = ( as.numeric(args[i]) != 0 ) ;
   i = i + 1;
 }

 if(length(args)  > i - 1 ){
   sampling_time = as.numeric(args[i]);
   i = i + 1;
 }

 if(length(args)  > i - 1 ){
   dir = args[i];
   i = i + 1
 }

 if(length(args) > i - 1 ){
   delta_y_cutoff = as.numeric(args[i]);
   i = i + 1;
 }


if( use_sum_vars) { 
source("~/basetrade/scripts/sumvars_tail_score_features_func.R" )
} else{
source("~/basetrade/scripts/indicators_tail_score_features_func.R" )
}
l = get_ilist_tail_score ( model_file, feature_ilist, begin_date, end_date, begin_time, end_time, pred_dur, thres = 0,  sd_fac = sd_fac, sampling_time = sampling_time, dir = dir,delta_y_cutoff = delta_y_cutoff, use_weights =use_weights, data.frame() )
