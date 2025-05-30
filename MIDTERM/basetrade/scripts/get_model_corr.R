#!/usr/bin/env Rscript

#this script will output the weight and correlation fo each indicator and model stdev, avg vol, avg l1sz and model correlation with delta y
  args <- commandArgs(trailingOnly = TRUE);
  if ( length ( args ) < 15 )
  {
    stop ( " Usage: <short_code> <model_file> <start_date> <end_date> <start_time> <end_time> <sampling_time> <sampling_events> <sampling trades> <eco_events> <empty_data_file> <pred_dur> <pred_algo> <filter> <is_siglr> \n " ) ;

  }
  #load the arguments
  short_code = args[1];
  model_file = args[2];
  begin_date = args[3];
  end_date = args[4]
  begin_time = args[5];
  end_time   = args[6];
  sampling_time = args[7];
  sampling_events = args[8];
  sampling_trades = args[9];
  eco_events = args[10]
  data_file = args[11];
  pred_dur  = args[12];
  norm_algo = args[13];
  filter    = args[14];
  is_siglr  = as.integer(args[15]);

  call_str = "~/basetrade_install/ModelScripts/generate_reg_indicator_data.pl";
  file_name = tempfile();
  #create the temp file
  system(paste(">", file_name,sep =" " ));

  call_str_2 =  sprintf ( "%s %s %s %s %s %s %s %s %s %s %s %s %s %s", call_str, model_file, begin_date,end_date, begin_time, end_time,sampling_time, sampling_events, 
                           sampling_trades, pred_dur, norm_algo ,file_name, "INVALIDFILE", filter   );

  system(call_str_2 ,intern=TRUE);


#system( paste("cat ",file_name,">",data_file ,sep ="" ), intern=TRUE);

 # read the weights
  weights = c();
  beta = c();
  if(is_siglr == 1)
  {
  fileName = model_file;
  conn=file(fileName,open="r")
    linn=readLines(conn)
      for (i in 4:( length(linn) -1 )){
               temp = (strsplit(linn[i], split = " " )[[1]] [2]) ;
               weights = c( weights,as.numeric(strsplit(temp, split = ":" )[[1]] [2]) );
               beta = c (beta,as.numeric(strsplit(temp, split = ":" )[[1]] [1]));       

                 }
  close(conn)
  }

  if( is_siglr != 1)
  {
  fileName = model_file;
  conn=file(fileName,open="r")
  linn=readLines(conn)
  for (i in 4:( length(linn) - 1 )){
       weights = c( weights,as.numeric(strsplit(linn[i], split = " " )[[1]] [2])) ;
  }
  close(conn)

  weights = weights[ which( !is.na(weights ) ) ]
  }

#weights = weights[-c(1:3  ) ]
#weights = weights[ -length(weights)]

  # read the reg_indicator_data
  data = read.table( file_name);
  #get the stdev of price
  price_stdev = sd( data$V1);

  corr_data =c()
  if( is_siglr == 1)
  {
  alpha = weights;
  X_train = data[,-1];
  T =  t( t(X_train) * alpha);
  H = 1/(1 + exp(-T)) - 0.5;
#  data[,-1] <-t( t(T) * beta);


  }


  for( i in 1:(ncol(data)-1) )
  {

  corr_data[i] = cor(data[,1], data[,i+1] );

  }

  model_corr = cor ( data[,1] , rowSums( t( weights*t(data[,-1] ) ) )) ;
  data_frame = data.frame( weights, corr_data  );
  names(data_frame) = c("weights", "corr"  );
  print(data_frame);

  call_str = "~/basetrade_install/scripts/get_avg_volume_in_timeperiod.pl";
  call_str_2 = sprintf ( "%s %s %s %s %s %s", call_str, short_code, begin_date,end_date, begin_time, end_time  );
  file_name = tempfile();

  system( paste(call_str_2, ">", file_name ,sep = " ") ,intern=TRUE);

  fileName = file_name;

  conn=file(fileName,open="r")
  linn=readLines(conn)
  l1 = rev(linn)[1];
  vol = strsplit( l1, split= " " )[[1]];
  vol = as.numeric(rev(vol)[1] );


  call_str = "~/basetrade_install/scripts/get_avg_l1_size_in_timeperiod.pl";
  call_str_2 = sprintf ( "%s %s %s %s %s %s", call_str, short_code, begin_date,end_date, begin_time, end_time  );
  file_name = tempfile();

  system( paste(call_str_2, ">", file_name ,sep = " ") ,intern=TRUE);

  fileName = file_name;

  conn=file(fileName,open="r")
  linn=readLines(conn)
  l1 = rev(linn)[1];
  l1_size = strsplit( l1, split= " " )[[1]];
  l1_size = as.numeric(rev( l1_size )[1] );

  stats = c(price_stdev,vol, l1_size, model_corr);
  names(stats) = c( "stdev", "vol","l1_size", "model_corr"); 

  print( stats);



