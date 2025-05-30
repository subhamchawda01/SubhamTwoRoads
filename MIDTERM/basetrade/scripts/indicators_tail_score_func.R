
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



get_ilist_score_from_data = function(model_file,timed_data, pred_dur = 100, thres = 0, sd_fac = 1, out_file =  " " , delta_y_cutoff, use_cor ){
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
  
  #temp_data = temp_data[,-(2:4)];
  
  #temp_data = get_pred_diff(df = temp_data,ind = 2,pred_dur = pred_dur );
  names = c();
  for( i in 1:ncol(temp_data)){
    str = paste("V",i,sep ="" );
    names = c(names, str);
  }

  names ( temp_data ) = names;

  for(  i in 3:ncol(temp_data ) ) {

   if( model_type == "linear" ) {
    #temp_data[,i] = weights[i-2] * temp_data[,i];
   }
   else{
      temp_data[,i] = weights[i-2] * ( 0.5 - 1/( 1 + exp ( -alphas [i-2]*temp_data[,i] ) ) );
   }

 } 
  
  names(temp_data) = names;
   ind =which( abs(temp_data[,2]) > delta_y_cutoff );  
   temp_data = temp_data[ind,] 


  scores = c()
  tail_scores = c();
  cor_scores = c();
  tail_cor_scores = c()
  num_points_vec = rep(0,ncol(temp_data)-2 );
  
  for( i in 3:ncol(temp_data)){
    

   #tail_data = get_tails (temp_data , 2, i, fac = sd_fac );
   #num_points_vec = c( num_points_vec, nrow( tail_data ) ) ;
    tail_data = data.frame();
    this_score = 0; 
    this_tail_score = 0;      
    this_cor = 0;
    this_tail_cor = 0;
    this_weight = 1 ;
  
   if( !is.na(sd(temp_data[,i] ) ) && !is.na(sd(temp_data[,2] ) ) && (sd(temp_data[,i] ) > 0) && (sd(temp_data[,2] ) > 0)   ){ 
     this_weight = 1 ;
    if( this_cor < 0 ) { this_weight = -1; } 
    
    this_score = get_indicator_score(indicator_val = temp_data[,i],pred_val = temp_data[,2],weight = this_weight, thres = thres);
    this_cor =  cor( temp_data[,i], temp_data[,2] ) ; 

    tail_data = get_tails (temp_data , 2, i, fac = sd_fac );
    num_points_vec[i-2] =  round( nrow( tail_data )/nrow(temp_data) , 3) ;

   
    if( !is.na(sd(tail_data[,1] ) ) && !is.na(sd(tail_data[,2] ) ) && (sd(tail_data[,1] ) > 0) && (sd(tail_data[,2] ) > 0)  ){
         this_tail_cor =  cor( tail_data[,2], tail_data[,1] );

    this_weight = 1 ;
    if( this_tail_cor < 0 ) { this_weight = -1; }

     this_tail_score = get_indicator_score(indicator_val = tail_data[,2],pred_val = tail_data[,1],weight = this_weight, thres = thres); 
    }


  }
    scores = c( scores, round( this_score,3) );
    tail_scores = c( tail_scores, round( this_tail_score,3) );

    cor_scores = c( cor_scores, round(this_cor,3) );    
    tail_cor_scores = c( tail_cor_scores,round( this_tail_cor,3) ) 
    
  }
  
  
  out_lines = c();
  for( i in 1:length(indicator_lines) ){      
    line = indicator_lines[i];      
    tokens = strsplit(line, split = " " ) [[1]];
    line = "";

   for( j in 3:length(tokens) ) {
     line = paste(line, tokens[j] ,sep = " ");
   }

  new_line = paste(line,"-----",cor_scores[i], scores[i],  num_points_vec[i], tail_cor_scores[i], tail_scores[i],sep = " ");
  out_lines = c(out_lines,new_line);
  }  
  if ( !file.exists(out_file)) {
    print(out_lines)
  }else{
   write(x = out_lines,file = out_file,append = FALSE);
  }
  return(0);
  
}

get_ilist_score = function(model_file,begin_date,end_date,begin_time,end_time, pred_dur, out_file, thres = 0, sd_fac = 2, sampling_time = 1000, dir = "/media/ephemeral2/", delta_y_cutoff = 0, use_cor = 0){  
  
  source("~/basetrade/scripts/r_utils.R");
  r_begin_date = get_r_date(begin_date);
  r_end_date   = get_r_date(end_date)  ;
  data_file =tempfile(tmpdir= dir);
  generate_timed_data_dates(model_file, data_file, r_begin_date, r_end_date, begin_time, end_time , sampling_time, l1_events = 0, trade_events = 0, dir = dir, is_reg_data = TRUE, pred_dur = pred_dur);
  data = read.table(data_file);
  l = get_ilist_score_from_data (model_file,data, pred_dur, thres , sd_fac, out_file, delta_y_cutoff, use_cor);
  
}

get_ilist_score_wrapper = function ( model_file,data, pred_dur, sd_fac = 2, delta_y_cutoff = 0, use_cor = 0) {

source("~/basetrade/scripts/r_utils.R");
  
  l = get_ilist_score_from_data (model_file,data, pred_dur, thres = 0  , sd_fac = sd_fac, out_file = " ", delta_y_cutoff, use_cor);
  return(l);
} 

get_data_wrapper = function ( model_file,begin_date,end_date,begin_time,end_time, sampling_time = 1000, dir = "/media/ephemeral2/" ) {

  source("~/basetrade/scripts/r_utils.R");
  r_begin_date = get_r_date(begin_date);
  r_end_date   = get_r_date(end_date)  ;
  data_file =tempfile(tmpdir= dir);
  generate_timed_data_dates(model_file, data_file, r_begin_date, r_end_date, begin_time, end_time , sampling_time, l1_events = 0, trade_events = 0, dir = dir, is_reg_data = TRUE, pred_dur = pred_dur);
  data = read.table(data_file);

  return(data);

}


