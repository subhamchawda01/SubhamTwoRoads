get_r_date = function(date){
  year = substr(date,start = 1,stop = 4);
  month = substr(date,start = 5,stop = 6);
  date = substr(date,start = 7,stop = 8);
  
  r_date = paste(year, month, date, sep="-");
  
  return(r_date);
  
}

#===================================================================================================================
get_str_date = function(r_date){
 r_date = as.character(r_date);
 tokens = strsplit(r_date, split="-")[[1]];
  year = tokens[1];
  month = tokens[2];
  date = tokens[3];

  date_str = paste(year, month, date, sep="");

  return(date_str);

}

#====================================================================================================================

get_product = function( model_file ){

  conn = file(model_file);
  all_lines = readLines(conn);
  close(conn);
  product = strsplit( all_lines[1], split = " " )[[1]][3];
 
  return( product );

}

#=====================================================================================================================

get_tick_size = function ( product, date ) {

tick_size <- as.numeric( system( paste ( "~/basetrade_install/bin/get_min_price_increment", product, date, sep =" " ), intern = TRUE ));

return(tick_size );

}


#======================================================================================================================

is_holiday = function ( product , date ) {

call_str = "~/basetrade/scripts/is_product_holiday.pl";
call_str = paste(call_str, date, product, sep = " " )

is_holiday <- as.numeric ( system( call_str, intern = TRUE ) );



return (is_holiday );

}

#======================================================================================================================

get_daily_avg = function(product,date, begin_time, end_time ){

temp_file = tempfile( tmpdir = "/spare/local/" );

call_str = paste( "~/basetrade/scripts/get_avg_volume_price_l1_size.sh", product, date , begin_time, end_time, temp_file, sep = " "  );
system(call_str, intern = TRUE );

data = read.table( temp_file );
avg_l1_bid_size = mean(as.numeric(data[,11] ) );
avg_l1_ask_size = mean(as.numeric(data[,14] ) );
avg_volume = sum(as.numeric(data[,5]));
avg_trade_size = avg_volume/nrow(data);
avg_traded_price = mean(data[,7]);
avg_sd = sd(data[,7]);
if (file.exists(temp_file)) {
file.remove(temp_file);
}
stats  =  c( date, avg_l1_bid_size, avg_l1_ask_size, avg_volume, avg_trade_size, avg_traded_price, avg_sd ) ;
return(stats);
}

#=======================================================================================================================
get_param_file =function(strat_file){
  conn <- file(strat_file,open="r")
  linn <-readLines(conn);
  close(conn);
  
  line = linn[1];
  tokens = strsplit(line,split = " ")[[1]];
  param_file = tokens[5];
  return(param_file);
  
}

#=======================================================================================================================

get_model_file =function(strat_file){
  conn <- file(strat_file,open="r")
  linn <-readLines(conn);
  close(conn);
  
  line = linn[1];
  tokens = strsplit(line,split = " ")[[1]];
  model_file = tokens[4];
  return(model_file);  
}

#======================================================================================================================
generate_timed_data = function(ilist, out_file,date, begin_time, end_time , time = 1000, l1_events = 0, trade_events = 0){
  options(scipen = 999);

  system( paste (">" , out_file, sep = "" ) , intern = TRUE );
 

  
  call_str = "~/basetrade_install/bin/datagen";
  
  arg = paste(ilist, " ", date," ",begin_time, " ", end_time, " ", "1234567"," ", out_file, " ", time," " ,l1_events," " ,trade_events, " ", "0", sep ="" );
  
  command = paste( call_str," ", arg, sep = "" );

  system(command, intern = TRUE);
  
  if( !is.na( file.info(out_file)$size ) && ( file.info(out_file)$size > 0 ) ){
    temp_df = read.table(out_file);
    dates = data.frame( rep(as.numeric(date) , nrow(temp_df ) ) );
    temp_df = cbind (dates, temp_df);
    
  
    write.table(x = temp_df,file = out_file,row.names =FALSE ,col.names = FALSE );
    
  }  
}

#======================================================================================================================
generate_reg_data = function(ilist, out_file,date, begin_time, end_time , pred_dur =100, time = 1000, l1_events = 0, trade_events = 0, dir = "/spare/local/", pred_algo = "na_t3"){
  options(scipen = 999 ); 
  system( paste (">" , out_file, sep = "" ) , intern = TRUE );
  temp_file = tempfile( tmpdir = dir ) ;
  generate_timed_data (ilist, temp_file,date, begin_time, end_time , time , l1_events , trade_events )
  
if( !is.na( file.info(temp_file)$size ) && ( file.info(temp_file)$size > 0 ) ){
  tt = read.table(temp_file);
  tt = tt[,-1];
  write.table(x = tt,file = temp_file,row.names =FALSE ,col.names = FALSE );

  product = get_product ( model_file = ilist );

  HOME_DIR <- Sys.getenv("HOME");
  REPO <- "basetrade";
  SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");
  MODELSCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/ModelScripts/", sep="");
  pred_counter_script <- paste(MODELSCRIPTS_DIR, "print_pred_counters_for_this_pred_algo.pl", sep="");
  counter_str = paste(pred_counter_script, product, pred_dur, pred_algo, temp_file, begin_time, end_time, sep =" " );
  pred_counter <- system( counter_str, intern = TRUE );


  call_str = "~/basetrade_install/bin/timed_data_to_reg_data";
  arg = paste(ilist, " ", temp_file," ",pred_counter, " ", pred_algo, " ",  out_file, sep ="" );
  command = paste( call_str," ", arg, sep = "" );
  system(command, intern = TRUE);

  if( !is.na( file.info(out_file)$size ) && ( file.info(out_file)$size > 0 ) ){
    temp_df = read.table(out_file);
    dates = data.frame( rep(as.numeric(date) , nrow(temp_df ) ) );
    temp_df = cbind (dates, temp_df);


    write.table(x = temp_df,file = out_file,row.names =FALSE ,col.names = FALSE );

  }
}
if(file.exists(temp_file) ) {
file.remove(temp_file);
}
}

#=============================================================================================================================================

timed_data_to_reg_data = function ( timed_data,ilist, pred_dur, pred_algo, begin_time, end_time, dir = "/spare/local/" ){

  product = get_product ( model_file = ilist );
  call_str = "~/basetrade_install/bin/timed_data_to_reg_data";

 dates =  unique( timed_data[,1] );


 reg_data = data.frame() ;
 temp_file = tempfile(tmpdir = dir);
 out_file = tempfile(tmpdir = dir);

 for ( i in 1:length(dates) ) {

     ind = which( timed_data[,1] == dates[i] ) ;
     write.table( timed_data[ind,-1] , temp_file, append = FALSE, row.names = FALSE, col.names = FALSE );

     HOME_DIR <- Sys.getenv("HOME");
     REPO <- "basetrade";
     SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");
     MODELSCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/ModelScripts/", sep="");
     pred_counter_script <- paste(MODELSCRIPTS_DIR, "print_pred_counters_for_this_pred_algo.pl", sep="");

     counter_str_2 = paste(pred_counter_script , product, pred_dur, pred_algo, temp_file, begin_time, end_time, sep =" " );
  pred_counter <- system( counter_str_2, intern = TRUE );
  arg = paste(ilist, " ", temp_file," ",pred_counter, " ", pred_algo, " ",  out_file, sep ="" );
  command = paste( call_str," ", arg, sep = "" );
  system(command, intern = TRUE);
  
  reg_data = rbind(reg_data, read.table( out_file ) );

 }

  return(reg_data)

}

#==============================================================================================================================================

generate_timed_data_dates = function(ilist, out_file,begin_date, end_date,begin_time, end_time , time = 1000, l1_events = 0, trade_events = 0, dir = "/spare/local/", is_reg_data = FALSE, pred_dur = 100, pred_algo = "na_t3" ){
  system( paste (">" , out_file, sep = "" ) , intern = TRUE );
  product = get_product ( model_file = ilist );


   if( !grepl("-", begin_date ) ) {
      begin_date = as.character ( get_r_date(begin_date ) )
   }  

  if( !grepl("-", end_date ) ) {
      end_date = as.character ( get_r_date(end_date ) )
   } 


  begin_date_r = as.Date(begin_date);
  end_date_r = as.Date(end_date);
  
  dates_range = seq( from = begin_date_r, to = end_date_r, by = "days" );
  dates_range_str = c();
  
  for ( i in 1:length(dates_range) )
  {
    temp = as.character( dates_range[i] );
    temp_1 = strsplit( temp,split = "-");
    temp_2 = paste ( temp_1[[1]][1], temp_1[[1]][2], temp_1[[1]][3], sep ="" );
    dates_range_str[i] = temp_2;
  }
  all_data = data.frame();
  for ( i in 1:length(dates_range_str ) )
  {
    
    date = as.character ( dates_range_str[i] );
    date_r = as.character (dates_range[i]);
    file_name = tempfile(tmpdir= dir);
      
    is_holiday = is_holiday( product, date ) ;
    if( is_holiday  != 1 ) { 
    if( !is_reg_data ) {
     generate_timed_data(ilist, file_name,date, begin_time, end_time , time , l1_events , trade_events  );
    }else{
       generate_reg_data (ilist, file_name,date, begin_time, end_time , pred_dur , time , l1_events , trade_events , dir, pred_algo ) ;   
    }
    
    if( !is.na( file.info(file_name)$size ) && ( file.info(file_name)$size > 0 ) ){
      temp_df = read.table(file_name);
      all_data = rbind(all_data, temp_df );
      
    }
    
   }
   if(file.exists(file_name)) {
      file.remove(file_name);
    }
  }

   write.table(x = all_data,file = out_file,row.names =FALSE ,col.names = FALSE );
}


#==============================================================================================================================================

get_pred_diff = function( df, ind, pred_dur, append = FALSE ){
  
  price = df[,ind];
  diff = diff(x = price,lag = pred_dur); 
  
  temp_df = df[ (  1 ):( nrow(df) - pred_dur ), ];
  if(append){
    temp_df = cbind(temp_df, diff);
  }else{
    temp_df[,ind] = diff;
  }
  return(temp_df);
  
}  
#=================================================================================================================================================

get_returns = function( df, ind, pred_dur,append = FALSE ){
  
  price = df[,ind];
  diff = diff(x = price,lag = pred_dur);
  
  l = price[(pred_dur + 1):length(prices)];
  
  ret = diff/l;
  
  temp_df = df[ ( pred_dur + 1 ):nrow(df), ];
  
  if(append){
    temp_df = cbind(temp_diff, ret);
  }else{
    temp_df[,ind] = ret;
  }
  return(temp_diff);
  
}

#=========================================================================================================================================

run_strat = function(strat_file, begin_date, end_date, out_file ){
  system( paste (">" , out_file, sep = "" ) , intern = TRUE );
  
  begin_date_r = as.Date(get_r_date(begin_date));
  end_date_r = as.Date(get_r_date(end_date));
  
  dates_range = seq( from = begin_date_r, to = end_date_r, by = "days" );
  dates_range_str = c();

  model_file = get_model_file (strat_file);
  product = get_product( model_file );
  
  for ( i in 1:length(dates_range) )
  {
    temp = as.character( dates_range[i] );
    temp_1 = strsplit( temp,split = "-");
    temp_2 = paste ( temp_1[[1]][1], temp_1[[1]][2], temp_1[[1]][3], sep ="" );
    dates_range_str[i] = temp_2;
  }
  
  for ( i in 1:length(dates_range_str ) )
  {
    date = dates_range_str[i];
    is_holiday = is_holiday( product, date );   
 
    if( is_holiday != 1 ) {
    call_str = "~/basetrade_install/bin/sim_strategy SIM";
    call_str = paste(call_str, " ", strat_file, " ",19891111, " ", date," ", "ADD_DBG_CODE -1", ">>",out_file,sep ="");
    system(call_str, intern = TRUE);
    }
  }  
}

#=============================================================================================================================================

merge_ilists = function(ilist_0, ilist_1, merged_ilist ){


  begin_lines = c();
  end_lines = c("INDICATOREND");

  indicator_lines_0 = c();
  indicator_lines_1 = c();

  conn = file( ilist_0 );
  all_lines_0 = readLines(conn);
  close(conn);

  conn = file( ilist_1 );
  all_lines_1 = readLines(conn);
  close(conn);

  start_reading = 0;

   for( i in all_lines_0 ) {
   
   is_indicator = ( strsplit(i, split = " " )[[1]][1] == "INDICATOR" );

   if( !is_indicator ) {
    if( start_reading == 0 ) 
      { begin_lines = c( begin_lines, i ) } ;
   }else{
    indicator_lines_0 = c( indicator_lines_0, i );
    start_reading = 1;
   } 
   
}


  for ( i  in all_lines_1 ) {
   
   is_indicator = ( strsplit(i, split = " " )[[1]][1] == "INDICATOR" );

   if( !is_indicator ) {
   
   }else{
    indicator_lines_1 = c( indicator_lines_1, i );
   } 

  }

 all_indicators = unique( c( indicator_lines_0, indicator_lines_1 ) ) ;

 system( paste(">", merged_ilist, sep = " " ) );
 write( begin_lines, merged_ilist, append = TRUE );
 write( all_indicators, merged_ilist, append = TRUE );
 write( end_lines, merged_ilist, append = TRUE );
 


}


#============================================================================================================================================

get_weights_from_model_file = function ( model_file ) {


  weights = c();
  conn = file( model_file );
  all_lines = readLines(conn);
  close(conn);


   for( i in all_lines ) {

   is_indicator = ( strsplit(i, split = " " )[[1]][1] == "INDICATOR" );

   if( is_indicator ) {
    weight =  strsplit(i, split = " " )[[1]][2] ;
    weights = c( weights, weight );
    
   }

}

return( weights );

}

#==============================================================================================================================================

get_weighted_indicators_data = function ( indicators_df, weights ) {

 is_siglr = grepl( ":", as.character( weights[1] ) );
 
 
 
 for ( i in 1:ncol(indicators_df ) ) {
  
   if( !is_siglr ) {
   indicators_df[,i] = as.numeric( weights[ i ] )*indicators_df[,i];
   }  
   else{
    alpha = as.numeric( strsplit(weights[i], split = ":" )[[1]][1] );
    weight = as.numeric( strsplit(weights[i], split = ":" )[[1]][2] );
  
    siglr_val =  weight * ( 0.5 - ( 1/( 1 + exp ( -alpha * indicators_df [,i] ) ) ) ); 
    indicators_df[,i] = siglr_val ;   
  }

 } 

  return ( indicators_df );

}

#===============================================================================================================================================


get_indicators = function(ilist) {


   indicators = c();

   conn = file( ilist );
   all_lines = readLines(conn);
   close(conn);


   for( i in all_lines ) {

   is_indicator = ( strsplit(i, split = " " )[[1]][1] == "INDICATOR" );

   if( is_indicator ) {
    indicators = c( indicators, i )  ;
    }

  }
  return( indicators );

}

#=================================================================================================================================================
sd_m = function( vec, mean = 0  ) {

  t = vec - mean ;
  t = sqrt( sum ( t*t )/length( vec ) ); 

  return( t ) ;

}
#===========================================================================================================================================
cor_m = function( vec_1, vec_2, m1 =0 , m2 =0 ) {

  sd_1 = sd_m (vec_1, m1 );
  sd_2 = sd_m (vec_2, m2 );

  s = sum (  (vec_1 - m1 )*(vec_2 - m2 ) )/length(vec_1) ;
  s = s/( sd_1 * sd_2 ) ;
  
  return(s);
}


#===========================================================================================================================================


get_product_stats = function( dep, indep, begin_date, end_date, begin_time, end_time, file ){

  dur = 300;
  dir = "/spare/local";

  ilist = tempfile( );
  lines = c();
  lines = c(lines, paste( "MODELINIT", "DEPBASE", dep, "OfflineMixMMS", "OfflineMixMMS",sep = " " ) );
  lines = c(lines, "MODELMATH LINEAR CHANGE");
  lines = c(lines, "INDICATORSTART" );
  lines = c(lines, paste( "INDICATOR", 1.00, "SimpleReturns" ,dep ,dur ,"OfflineMixMMS", sep = " " ) );
  lines = c(lines, paste( "INDICATOR", 1.00, "SimpleReturns",indep, dur, "OfflineMixMMS", sep = " " ) );
  lines = c(lines, paste( "INDICATOR", 1.00, "SimplePriceType",dep, "BidPrice",  sep = " " ) );
  lines = c(lines, paste( "INDICATOR", 1.00, "SimplePriceType",dep, "AskPrice",  sep = " " ) );
  lines = c(lines, "INDICATOREND"); 
 
  write( lines, ilist);

  out_file = tempfile( tmpdir = "/spare/local/" );

  generate_timed_data_dates (ilist, out_file,begin_date, end_date,begin_time, end_time , time = 1000, l1_events = 0, trade_events = 0, dir = dir, is_reg_data = TRUE, pred_dur = 100 ); 
  data = read.table( out_file );
  dates = data[,1];
  unique_dates = unique( dates ) ;
 

  dep_volume_file= tempfile( tmpdir = dir );
  dep_l1_size_file = tempfile( tmpdir = dir );

  indep_volume_file= tempfile( tmpdir = dir );
  indep_l1_size_file = tempfile( tmpdir = dir );

  temp_file = tempfile( tmpdir = dir );
 
  call_str = paste( "~/basetrade/scripts/get_avg_volume_in_timeperiod.pl", dep, begin_date, end_date, begin_time, end_time,">", dep_volume_file, sep = " " )
  system(call_str, intern = TRUE ); 
  system( paste( "head -n -1",  dep_volume_file, ">" ,temp_file,  sep = " " ), intern = TRUE );
  system ( paste ("cp", temp_file, dep_volume_file, sep= " " ), intern = TRUE );
  dep_volume_df = read.table(dep_volume_file )[,-2];


  call_str = paste( "~/basetrade/scripts/get_avg_volume_in_timeperiod.pl", indep, begin_date, end_date, begin_time, end_time,">", indep_volume_file, sep = " " );
  system(call_str, intern = TRUE );
  system( paste( "head -n -1",  indep_volume_file, ">", temp_file,  sep = " " ), intern = TRUE );
  system ( paste ("cp", temp_file, indep_volume_file, sep= " " ), intern = TRUE );
  indep_volume_df = read.table(indep_volume_file )[,-2];


  call_str = paste( "~/basetrade/scripts/get_avg_l1_size_in_timeperiod.pl", dep, begin_date, end_date, begin_time, end_time,">", dep_l1_size_file, sep = " " )
  system(call_str, intern = TRUE );
  system( paste( "head -n -1",  dep_l1_size_file, ">", temp_file,  sep = " " ), intern = TRUE );
  system ( paste ("cp", temp_file, dep_l1_size_file, sep= " " ), intern = TRUE );
  dep_l1_size_df = read.table(dep_l1_size_file )[,-2];
  

  call_str = paste( "~/basetrade/scripts/get_avg_l1_size_in_timeperiod.pl", indep, begin_date, end_date, begin_time, end_time,">", indep_l1_size_file, sep = " " );
  system(call_str, intern = TRUE );
  system( paste( "head -n -1",  indep_l1_size_file, ">", temp_file,  sep = " " ), intern = TRUE );
  system ( paste ("cp", temp_file, indep_l1_size_file, sep= " " ), intern = TRUE );
  indep_l1_size_df = read.table(indep_l1_size_file )[,-2];

if(file.exists(temp_file)) {
  file.remove(temp_file);
}

if(file.exists(out_file)) { 
 file.remove(out_file);
}

if(file.exists( dep_volume_file)) {
 file.remove(dep_volume_file);
}

if(file.exists(dep_l1_size_file)) {
  file.remove( dep_l1_size_file);
}

if(file.exists(indep_volume_file)) {
 file.remove(indep_volume_file);
}

if(file.exists(indep_l1_size_file)) {
 file.remove( indep_l1_size_file);
}

  stats_df = data.frame();

 for( i in 1:length(unique_dates) ) {

  ind = which( unique_dates[i] == dates );
  temp_df = data[ind,];

  dep_sd = sd( temp_df[,3] ) ;
  indep_sd = sd( temp_df[,4] );  
  cor = cor( temp_df[,3], temp_df[,4 ] ) ;
  beta = cor*dep_sd/indep_sd;

  dep_bid_price = temp_df[,5];
  dep_ask_price = temp_df[,6]; 
  avg_spread = mean( dep_ask_price - dep_bid_price ) ;
  avg_price = mean( dep_ask_price + dep_bid_price ) /2;

  dep_avg_volume = dep_volume_df[ which( dep_volume_df[,1] == unique_dates[i] ),2];
  indep_avg_volume = indep_volume_df[ which( indep_volume_df[,1] == unique_dates[i] ),2];
  dep_avg_l1_size = dep_l1_size_df[ which( dep_l1_size_df[,1] == unique_dates[i] ),2 ];   
  indep_avg_l1_size = indep_l1_size_df[ which( indep_l1_size_df[,1] == unique_dates[i] ),2 ]; 
 
  vec = c(  unique_dates[i], dep_avg_l1_size, dep_avg_volume, indep_avg_volume, avg_price, avg_spread, dep_sd, indep_sd, cor, beta  );
  stats_df = rbind( stats_df, vec );
}
  colnames(stats_df) = c( "date", "l1_size",  "dep_volume", "indep_volume", "avg_price", "avg_spread", "dep_sd", "indep_sd", "cor", "beta" ) ;
  rownames(stats_df) = c();
 
  if (file.exists( file ) ) {
   write.table( stats_df, file );
  
  }
  return( stats_df );
}


