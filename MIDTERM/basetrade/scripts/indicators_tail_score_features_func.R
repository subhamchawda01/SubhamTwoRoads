get_tails = function(  indicator_val, fac = 2 ){
  
  mean = mean(indicator_val );
  sd   =   sd(indicator_val );
  
  high = mean + (fac * sd );
  low  = mean - ( fac * sd );
    
  ind_0  = which ( indicator_val > high );
  ind_1  = which ( indicator_val < low ) ;
  
  ind = union(ind_0, ind_1 );
  
  return(ind);
  
}

get_indicator_tail_score_1 = function(df, pred_val_ind, indicator_ind, feature_vec_ind, sd_fac, thres, indicator_name = "",feature_vec_names = rep(" " ,length( feature_vec_ind ) ), print_features = TRUE ){
  
  indicator_val = df[,indicator_ind];
  
  
  positive_ind = which( df[,pred_val_ind]*indicator_val > 0 );
  negative_ind = which( df[,pred_val_ind]*indicator_val<= 0 );  
    
    
  tail_ind = get_tails(indicator_val, sd_fac  );
  
  x = indicator_val[ tail_ind ]*df[ tail_ind, pred_val_ind ];
  
  positive_tail_ind = which( x > 0);
  negative_tail_ind = which ( x <= 0);
  
  total_score = length(positive_ind) / ( length(positive_ind) +  length( negative_ind ) ) ;
  total_cor = cor_m( indicator_val, df[, pred_val_ind ] );
  tail_score = length(positive_tail_ind) / ( length(positive_tail_ind) +  length( negative_tail_ind ) ) ;
  tail_cor = cor_m( indicator_val[tail_ind], df[tail_ind, pred_val_ind ] )   ;   
  num_points = length(tail_ind)/ nrow(df);
  
  output_df = data.frame();

  if( print_features ) {
  
  for( i in feature_vec_ind ){
    
    total_feature_avg = mean(df[,i]);
    total_positive_feature_avg = mean(df[positive_ind,i]);
    total_negative_feature_avg = mean(df[negative_ind,i]);
    
    tail_feature_avg = mean(df[tail_ind,i])
    tail_positive_feature_avg = mean(df[tail_ind, ][positive_tail_ind,i]);
    tail_negative_feature_avg = mean(df[tail_ind, ][negative_tail_ind,i]);
    
    output_df = rbind(output_df, c( total_feature_avg, total_positive_feature_avg, total_negative_feature_avg, 
                                    tail_feature_avg, tail_positive_feature_avg, tail_negative_feature_avg ) );
    
    
  }
  
  }

  cat(indicator_name, sep = "\n" );
  
  cat(c("fraction_of_points", num_points, "total_score", total_score,"total_cor", total_cor , "tail_score", tail_score, "tail_cor", tail_cor  ) );
  cat ( "\n" );
  cat ( "\n");
  if( print_features ) {  

  cat("FEATURES_STATS" );
  cat ("\n");
  row.names(output_df) = c();
  names(output_df) = rep(" ", ncol(output_df) );
  
  for( i in 1:nrow( output_df ) ) {
    cat(feature_vec_names[i] ) ; 
    cat ("\n")
    cat( as.numeric( output_df[i,] ) ); 
    cat ("\n")
    cat ( "\n");
   }
  }
  cat( "======================================================================================================================================"  );  
  cat ("\n")
  
} 

get_data_wrapper = function ( model_file,begin_date,end_date,begin_time,end_time, sampling_time = 1000, dir = "/media/ephemeral2/" ) {
  
  source("~/basetrade/scripts/r_utils.R");
  r_begin_date = get_r_date(begin_date);
  r_end_date   = get_r_date(end_date)  ;
  data_file =tempfile(tmpdir= dir);
  generate_timed_data_dates(model_file, data_file, r_begin_date, r_end_date, begin_time, end_time , sampling_time, l1_events = 0, trade_evenst = 0, dir = dir);
  data = read.table(data_file);
  
  return(data);  
}

get_ilist_tail_score = function ( model_file,feature_ilist, begin_date, end_date, begin_time, end_time, pred_dur, thres = 0, 
                                  sd_fac = 2, sampling_time = 1000, dir = "/media/ephemeral2/", delta_y_cutoff = 0, use_weights = FALSE, timed_data = data.frame() ){
  
  source("~/basetrade/scripts/r_utils.R");

  options(width=200)
  
  indicator_names = c() ;
  num_indicators  = 0;
  feature_names = c() ;
  num_features   = 0;
  feature_file_exists = FALSE;

  if( nrow(timed_data)  == 0 ){
    merged_ilist_file = tempfile( tmpdir = dir);

    if( ! file.exists(feature_ilist ) ) {
       merged_ilist_file = model_file;
       indicator_names = get_indicators ( model_file) ;
       num_indicators  = length(indicator_names);
       feature_names =  c() ;
       num_features   = length( feature_names );
    } else{
        merge_ilists (model_file, feature_ilist, merged_ilist_file );
        indicator_names = get_indicators ( model_file) ;
        num_indicators  = length(indicator_names);
        feature_names = get_indicators ( feature_ilist ) ;
        num_features   = length( feature_names );
        feature_file_exists = TRUE;
    }
    timed_data = get_data_wrapper ( merged_ilist_file, begin_date, end_date,begin_time,end_time, sampling_time, dir );
  } 

  timed_data = timed_data[,-(2:4)];
  
  conn = file(model_file);
  all_lines = readLines(conn);
  close(conn);

  
  product = strsplit( all_lines[1], split = " " )[[1]][3];
  tick_size <- as.numeric( system( paste ( "~/basetrade_install/bin/get_min_price_increment", product, timed_data[1,1], sep =" " ), intern = TRUE ));
  delta_y_cutoff = delta_y_cutoff*tick_size;
  
 
  pred_data = get_pred_diff(df = timed_data,ind = 2,pred_dur = pred_dur );
  
  names = c();
  for( i in 1:ncol(pred_data)){
    str = paste("V",i,sep ="" );
    names = c(names, str);
  }
  
  names(pred_data) = names;
  
  ind =which( abs(pred_data[,2]) > delta_y_cutoff );
  pred_data = pred_data[ind, ];
  
  weights = c();
  if ( use_weights ) {
    weights =  get_weights_from_model_file ( model_file ) ;
  }else
  {
     lm_data = pred_data[,-1];
     lm_data = pred_data[, 2:(3+num_indicators -1 )  ]
     lm = lm( V2 ~.+0, lm_data );
     weights = as.character( lm$coefficients );
  }  
 
   ind =which( abs(pred_data[,2]) > delta_y_cutoff );
   pred_data = pred_data[ind, ];

 
  weighted_indicators_data = get_weighted_indicators_data ( pred_data[,3:(3+num_indicators -1 ) ], weights );

  pred_data[,3:(3+num_indicators -1 )] = weighted_indicators_data;
  


  feature_ind_vec = (3+num_indicators ):( 3 + num_indicators+ num_features - 1 );

   if( !feature_file_exists ) {
   feature_ind_vec = c();
   } 

  
  for ( i in 3:(3+num_indicators -1 ) ) {
    get_indicator_tail_score_1 (pred_data, 2, i, feature_ind_vec, sd_fac, thres, indicator_name = indicator_names[i-2], feature_names, feature_file_exists );
    
  } 
  
}





