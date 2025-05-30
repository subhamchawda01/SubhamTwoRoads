#!/usr/bin/env Rscript

args <- commandArgs(trailingOnly = TRUE);

if ( length ( args ) < 8 )
{
  stop ( " Usage: <model_file begin_date end_date begin_time end_time pred_dur sd_fraction output_file sampling_time =1000, dir = /media/ephemeral2/ , delta_y_cutoff = 0( in ticks) use_cor = 0 > \n " ) ;
}

model_file = args[1];
begin_date = args[2];
end_date = args[3];
begin_time = args[4];
end_time = args[5];
pred_dur = as.numeric(args[6]);
sd_fac = as.numeric(args[7]);
out_file =args[8];
sampling_time =1000;
dir = "/media/ephemeral2/";
delta_y_cutoff = 0;
use_cor = 0;

 if(length(args)  > 8 ){
   sampling_time = as.numeric(args[9]);
 }

 if(length(args)  > 9 ){
   dir = args[10];
 }

 if(length(args) > 10 ){
   delta_y_cutoff = as.numeric(args[11]);
 }

if(length(args)  > 11 ){
   use_cor = as.numeric(args[12]);
 }

get_tails = function(df , pred_val_ind, indicator_ind, fac = 2 ){
  
  mean = mean(df[,indicator_ind]);
  sd = sd(df[,indicator_ind]);
  
  high = mean + (fac * sd );
  low  = mean - ( fac * sd );
  
  indicator_val = df[,indicator_ind];
  ind_0  = which ( indicator_val > high );
  ind_1  = which ( indicator_val < low ) ;
  
  ind = union(ind_0, ind_1 );
  
  temp_df = df[ind, c(pred_val_ind,indicator_ind) ];  
  return(temp_df);
  
}


get_indicator_score = function( indicator_val, pred_val ,weight = 1 , thres = 0 ){
  
  num_all = length(indicator_val);
  x=0;
  
  if( weight > 0 ){
    x = indicator_val*pred_val;
  }else{
    x = -indicator_val*pred_val;
  }
  ind = which ( x > thres ) ; 
  
  score = length(ind)/ num_all;
  
  return(score);
}



get_ilist_score_from_data = function(model_file,timed_data, pred_dur = 100, thres = 0, sd_fac, out_file, delta_y_cutoff, use_cor ){
  source("~/basetrade/scripts/r_utils.R");
  conn = file(model_file);
  all_lines = readLines(conn);
  close(conn);
  indicator_lines = c();
  weights = c();
  alphas = c();
  model_type = "linear" 
 product = strsplit( all_lines[1], split = " " )[[1]][3];

  tick_size <- as.numeric( system( paste ( "~/basetrade_install/bin/get_min_price_increment", product, timed_data[1,1], sep =" " ), intern = TRUE ));
  delta_y_cutoff = delta_y_cutoff*tick_size; 


  
  
  for( line in all_lines ){
    if(substr(line,start = 1,stop = 10) == "INDICATOR "){
      indicator_lines = c(indicator_lines, line);
      tokens = strsplit(x = line,split = " ")[[1]];
      temp_weight = 0;
      temp_alpha = 0;
      if( grepl(":",tokens[2] ) )
      { 
        temp_alpha = as.numeric(strsplit(tokens[2],split = ":")[[1]][1]); 
        temp_weight = as.numeric(strsplit(tokens[2], split = ":" )[[1]][2]);        
        model_type = "siglr";
      }
      else{
       temp_weight = as.numeric(tokens[2] );
      }

      alphas = c(alphas, temp_alpha );
      weights = c(weights, temp_weight );

    }
    
  }
 
 
  temp_data = timed_data;  
  dim(temp_data);
  
  temp_data = temp_data[,-(2:4)];
  
  temp_data = get_pred_diff(df = temp_data,ind = 2,pred_dur = pred_dur );
  names = c();
  for( i in 1:ncol(temp_data)){
    str = paste("V",i,sep ="" );
    names = c(names, str);
  }



sum_vars = rep(0,nrow(temp_data));
  for(  i in 3:ncol(temp_data ) ) {

   if( model_type == "linear" ) {
    temp_data[,i] = weights[i-2] * temp_data[,i];
   sum_vars = sum_vars + temp_data[,i];
   }
   else{
      temp_data[,i] = weights[i-2] * ( 0.5 - 1/( 1 + exp ( -alphas [i-2]*temp_data[,i] ) ) );
      sum_vars = sum_vars + temp_data[,i];
   }

 } 
  
  names(temp_data) = names;
   ind =which( abs(temp_data[,2]) > delta_y_cutoff ); 

   temp_data = temp_data[ind,] ;
   sum_vars = sum_vars[ind];

   tail_ind = which( ( abs ( sum_vars - mean(sum_vars) ) /sd(sum_vars) > sd_fac )) ;

   temp_data = temp_data[tail_ind, ] ;
   sum_vars = sum_vars[tail_ind];
     
   wrong_ind = which ( sum_vars*temp_data[,2] < 0 ) ;
   
   temp_data = temp_data[ wrong_ind, ] ;
   sum_vars = sum_vars[ wrong_ind];

  scores = c();
  
  for( i in 3:ncol(temp_data)){
    this_score = 0; 
    if(use_cor == 0){  
      
    temp_ind = which ( sum_vars*temp_data[,i] >= 0 ) ;    

    this_score =   ( length(temp_ind ) /length(sum_vars ) ) * (  mean ( abs( temp_data[temp_ind,i] )/abs ( sum_vars[temp_ind] ) ) );
    }else{
       this_score = cor( temp_data[,i], temp_data[,2] );
     }
    scores = c(scores, this_score);
    
  }
  
  
  list_0 = list();
  out_lines = c();
  for( i in 1:length(indicator_lines) ){      
    line = indicator_lines[i];      
    list_0[[line]] = scores[i];
    new_line = paste(line,"score",scores[i],sep = " ");
    out_lines = c(out_lines,new_line);
  }  
   write(x = out_lines,file = out_file,append = FALSE);
  return(list_0);
  
}

get_ilist_score = function(model_file,begin_date,end_date,begin_time,end_time, pred_dur, out_file, thres = 0, sd_fac = 2, sampling_time = 1000, dir = "/media/ephemeral2/", delta_y_cutoff = 0, use_cor = 0){  
  
  source("~/basetrade/scripts/r_utils.R");
  r_begin_date = get_r_date(begin_date);
  r_end_date   = get_r_date(end_date)  ;
  data_file =tempfile(tmpdir= dir);
  generate_timed_data_dates(model_file, data_file, r_begin_date, r_end_date, begin_time, end_time , sampling_time, l1_events = 0, trade_evenst = 0, dir = dir);
  data = read.table(data_file);
  l = get_ilist_score_from_data (model_file,data, pred_dur, thres , sd_fac, out_file, delta_y_cutoff, use_cor);
  
}
  

l = get_ilist_score ( model_file, begin_date, end_date, begin_time, end_time, pred_dur, out_file, thres = 0, sd_fac = sd_fac, sampling_time = sampling_time, dir = dir,delta_y_cutoff = delta_y_cutoff, use_cor =use_cor )
