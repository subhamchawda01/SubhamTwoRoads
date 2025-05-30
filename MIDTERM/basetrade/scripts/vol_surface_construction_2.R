get_complement_option = function(short_code){
 tokens = strsplit(short_code,split = "_")[[1]];
 
 if(tokens[3] == "P0"){
   tokens[3] = "C0"
 }else{
   tokens[3] = "P0"
 }
 
 if( tokens[4] != "A") {
   t = strsplit(tokens[4],split = "")[[1]];
   if( t[1] == "I"){ t[1] = "O" }
   else{      t[1] = "I" };
   tokens[4] = paste(t[1], t[2],sep = "");
   
 }
 
 complement_short_code =tokens[1];
 for( i in 2:length(tokens)  ){
   complement_short_code = paste( complement_short_code, tokens[i],sep = "_");
 }
 return(complement_short_code);
}

#===============================================================================================

get_average_volume = function( short_code, begin_date, end_date, begin_time, end_time ){
  
  call_str = "~/basetrade/scripts/get_avg_volume_in_timeperiod.pl" ;
  temp_file = tempfile(tmpdir = "/media/ephemeral2");
  system( paste(call_str,short_code, begin_date, end_date, begin_time, end_time,">",temp_file,sep = " " ),intern = TRUE);
  
  con = file(temp_file)
  lines = readLines(con);
   last_line = lines[length(lines)];
 print(last_line) 
  volume = as.numeric( strsplit(last_line,split = " ")[[1]][5] );
  close(con);
  return(volume);
  
}

#===============================================================================================

format_data = function( options_file, contract_file ){
  
  options_data = read.table(options_file);
  num_strikes = ncol(options_data) - 4;
  
  option_prices = options_data[,4:(ncol(options_data) -1)];
  option_prices = as.vector(t(option_prices));
  
  future_prices = options_data[,3];
  
  for( i in 1:( num_strikes-1) ){
    future_prices = cbind(future_prices, options_data[,3] );
  }
  
  future_prices = as.vector( t(future_prices));  
  
  atm_vols = options_data[,ncol(options_data)];
  
  for( i in 1:( num_strikes - 1) ){
    atm_vols = cbind(atm_vols, options_data[,ncol(options_data)] );
  }  
  
  atm_vols = as.vector( t(atm_vols));  
  
  contract_specification_data = read.table(contract_file) ;  
  
  dates = contract_specification_data[,1];
  
  num_elem_per_day = c();
  
  for( i in 1:length(dates) ){
    ind = which( dates[i] == options_data[,1]);
    num_elem_per_day[i] = length(ind);
  }
  
  strikes_data = cbind(contract_specification_data, num_elem_per_day );
  
  strikes_data_expanded <- strikes_data[ rep( row.names(strikes_data), strikes_data[,ncol( strikes_data )] ), 1:( ncol( strikes_data ) -1 )];
  strike_prices = strikes_data_expanded[,2:(num_strikes+1)];
  strike_prices = as.vector(t(strike_prices));
  
  times = strikes_data_expanded[,(num_strikes+2):(2*num_strikes +1 )];
  times = as.vector(t(times));
  
  option_types = strikes_data_expanded[,(2*num_strikes +2 ):(3*num_strikes +1)];
  option_types = as.vector(t(option_types));
  
  volume_weights = strikes_data_expanded[,(3*num_strikes +2 ):(4*num_strikes +1)];
  volume_weights = as.vector(t(volume_weights));
  
  all_df = data.frame(option_prices, option_types, future_prices, strike_prices, times, atm_vols, volume_weights);
}

fit_normal_mixture_model = function( option_prices, option_types, future_prices, strikes, times, interest_rate, volume_weights,model_size ) {
  require("fOptions");
  require("stats");  
  
  stock_prices = future_prices*exp(-interest_rate*times);  
  ind_1 = which( option_types == "c");
  ind_2 = which( option_types == "p");
  
  
  
  optim_func = function( param ){
    
    weights = param[1:(model_size)];
    mean_vec = param[ (model_size +1): ( 2*model_size  ) ]; 
    vol_vec = sqrt( param[ ( ( 2*model_size ) + 1 ): ( 3 * model_size )  ]^2);      
    
    weights = weights * weights/(sum(weights*weights));
    
    model_option_prices = rep(0, length(option_prices));    
    norm_const = exp(interest_rate*times[1])/sum( weights*exp(mean_vec*times[1]) );
    norm_const = log(norm_const)/times[1];
    mean_vec = mean_vec + norm_const;
    
    if( ( length(ind_1) ==0 ) || ( length(ind_2) == 0 )  ){
    
    for( i in 1:length(mean_vec) ){
      temp_price = GBSOption(TypeFlag = option_types,S = stock_prices*exp(mean_vec[i]*times),X = strikes,Time = times,r = mean_vec[i],b = 0,sigma = vol_vec[i]  )@price;
      model_option_prices = model_option_prices + ( weights[i]*temp_price );      
      
    }   
    }else{
      for( i in 1:length(mean_vec) ){
        
        temp_price_1 = GBSOption(TypeFlag = option_types[ind_1][1],S = stock_prices[ind_1]*exp(mean_vec[i]*times[ind_1]),
                                 X = strikes[ind_1],Time = times[ind_1],r = mean_vec[i],b = 0,sigma = vol_vec[i]  )@price;
        
        temp_price_2 = GBSOption(TypeFlag = option_types[ind_2][1],S = stock_prices[ind_2]*exp(mean_vec[i]*times[ind_2]),
                                 X = strikes[ind_2],Time = times[ind_2],r = mean_vec[i],b = 0,sigma = vol_vec[i]  )@price;
        
        model_option_prices[ind_1] = model_option_prices[ind_1] + ( weights[i]*temp_price_1 );      
        model_option_prices[ind_2] = model_option_prices[ind_2] + ( weights[i]*temp_price_2 );      
        
      }  
      
    }
    
    #print((option_prices - model_option_prices));
    mse = sqrt( sum( (option_prices - model_option_prices)^2*(volume_weights) ) / length(option_prices) );
    return(mse);
  }
  
  
  
  
  param = rep( 1/(model_size ), model_size   );
  lower = rep( -1e6, model_size );
  upper = rep( 1e6, model_size );
  
  param = c( param, rep( interest_rate, model_size  ) );
  lower = c( lower, rep( -50*interest_rate, model_size  ) );
  upper = c( upper, rep( 50*interest_rate, model_size  ) );
  
  param = c( param, rep( 0.2, model_size ) ); 
  lower = c( lower, rep( -10, model_size  ) );  
  upper = c( upper, rep( 10, model_size  ) );
  
  
  
  #opt = optim(par = param,fn = optim_func ,control = list(maxit = 1e7),method = "L-BFGS-B",lower = lower, upper = upper ); 
  opt = optim(par = param,fn = optim_func ,control = list(maxit = 1e7,abstol = 0.05) ); 
  param = opt$par;
  #optim_plot_func(param);
  print(c("mse", opt$value));
  
  weights = param[1:model_size]   ;
  weights = weights * weights;
  weights = weights/sum(weights);
  
  mean_vec = param[(model_size+1):(2*model_size)];
  norm_const = exp(interest_rate*times[1])/sum( weights*exp(mean_vec*times[1]) );
  norm_const = log(norm_const)/times[1];
  mean_vec = mean_vec + norm_const;
  
  vol_vec = param [( (2*model_size) + 1 ):(3*model_size )];  
  vol_vec = sqrt(vol_vec^2);
  
  #get_log_normal_mixure_prices (weights, mean_vec, vol_vec,  option_types = option_types, strikes, 
  #                                future_prices, times[1], obs_option_prices = option_prices,interest_rate = 0.075);
  
  return(list(weights, mean_vec, vol_vec, param));
}



plot_model_vols = function( param, option_prices, option_types, future_prices, strikes, times, interest_rate  ){
  
  model_size = length(param)/3;
  stock_prices = future_prices*exp(-interest_rate*times);  
  ind_1 = which( option_types == "c");
  ind_2 = which( option_types == "p");
  
  weights = param[1:(model_size)];
  mean_vec = param[ (model_size +1): ( 2*model_size  ) ]; 
  vol_vec = sqrt( param[ ( ( 2*model_size ) + 1 ): ( 3 * model_size )  ]^2);      
  
  weights = weights * weights/(sum(weights*weights));
  
  model_option_prices = rep(0, length(option_prices));    
  norm_const = exp(interest_rate*times[1])/sum( weights*exp(mean_vec*times[1]) );
  norm_const = log(norm_const)/times[1];
  mean_vec = mean_vec + norm_const;
  
  if( ( length(ind_1) ==0 ) || ( length(ind_2) == 0 )  ){
    
    for( i in 1:length(mean_vec) ){
      temp_price = GBSOption(TypeFlag = option_types,S = stock_prices*exp(mean_vec[i]*times),X = strikes,Time = times,r = mean_vec[i],b = 0,sigma = vol_vec[i]  )@price;
      model_option_prices = model_option_prices + ( weights[i]*temp_price );      
      
    }   
  }else{
    for( i in 1:length(mean_vec) ){
      
      temp_price_1 = GBSOption(TypeFlag = option_types[ind_1][1],S = stock_prices[ind_1]*exp(mean_vec[i]*times[ind_1]),
                               X = strikes[ind_1],Time = times[ind_1],r = mean_vec[i],b = 0,sigma = vol_vec[i]  )@price;
      
      temp_price_2 = GBSOption(TypeFlag = option_types[ind_2][1],S = stock_prices[ind_2]*exp(mean_vec[i]*times[ind_2]),
                               X = strikes[ind_2],Time = times[ind_2],r = mean_vec[i],b = 0,sigma = vol_vec[i]  )@price;
      
      model_option_prices[ind_1] = model_option_prices[ind_1] + ( weights[i]*temp_price_1 );      
      model_option_prices[ind_2] = model_option_prices[ind_2] + ( weights[i]*temp_price_2 );      
      
    }  
    
  }
  
  model_vols =c ();
  obs_vols = c();
  for( i in 1:length(option_prices ) ){
    model_vols[i] = GBSVolatility(price  = model_option_prices[i],TypeFlag = option_types[i],S = stock_prices[i]*exp(0.075*times[1]),
                                  X = strikes[i],Time = times[1],r = 0.075,b = 0) ;
    obs_vols[i] = GBSVolatility(price  = option_prices[i],TypeFlag = option_types[i],S = stock_prices[i]*exp(0.075*times[1]),
                                X = strikes[i],Time = times[1],r = 0.075,b = 0) ;
  }
  
  #model_vols = model_vols + ( obs_vols[atm_ind] - model_vols[atm_ind]);
  #x11();
  #plot(obs_vols, col = "red", type = "l");
  # x11();
  #lines( model_vols, col = "green", type = "l");
  
  mse = sum( (model_vols - obs_vols)^2 /length(model_vols) );
  print(mse);
  return(list( model_vols, obs_vols));
}




get_volume_profiles = function( options_shc,begin_date, end_date, begin_time, end_time ){
  
  complement_options_shc = c();
  options_volume = c();
  complement_options_volume = c();
  options_shc_to_use = c();
  volumes_to_use = c() ;
  
  for( i in 1:length(options_shc) ){
    complement_options_shc[i] = get_complement_option(short_code = options_shc[i]);
    options_volume[i] = get_average_volume(short_code = options_shc[i], begin_date, end_date, begin_time, end_time);
    complement_options_volume[i] = get_average_volume(short_code = complement_options_shc[i], begin_date, end_date, begin_time, end_time);   
    
    if(options_volume[i] < complement_options_volume[i] )
    { options_shc_to_use[i] =  complement_options_shc[i]
      volumes_to_use[i] = complement_options_volume[i];
    }else{
      options_shc_to_use[i] =  options_shc[i]
      volumes_to_use[i] = options_volume[i];
    }    
    
  }
  
  temp_df = data.frame(options_shc, complement_options_shc, options_volume, complement_options_volume, options_shc_to_use, volumes_to_use );
  return(temp_df);
}

get_formatted_data = function( options_shc, future_shc, begin_date, end_date, begin_time, end_time,  formatted_data_file, atm_ind, interest_rate = 0.075 ){
  
  #ilist format
  #first indicator is future price followed by option prices 
  
  source("~/basetrade/scripts/r_utils.R");
  
  temp_df = get_volume_profiles ( options_shc,begin_date, end_date, begin_time, end_time );
  #options_shc = as.vector(temp_df[,5]);
  volumes     = as.vector( temp_df[,6] ) ;
  volume_weights     = volumes/sum(volumes);
  
  begin_r_date = get_r_date( begin_date);
  begin_r_date = as.Date(begin_r_date);
  
  end_r_date = get_r_date( end_date);
  end_r_date = as.Date(end_r_date);
  
  all_r_dates = seq(from = begin_r_date,to = end_r_date,by = 1);
  all_dates = c()
  
  for( i in 1:length(all_r_dates) ){
    all_dates[i] = get_str_date( all_r_dates[i] );
  }
  
  option_types = c();
  
  for( i in 1:length(options_shc) ){
    type = strsplit(options_shc[i],split = "_")[[1]][3];
    
    if(type == "C0"){
      type = "c"
    }else{
      type = "p"
    }
    
    option_types = c(option_types, type);    
  }
  
  call_str = "~/basetrade_install/bin/option_details";
  
  spec_df = data.frame();
  
  dates = c();
  strikes_df = data.frame();
  times_df = data.frame();
  types_df = data.frame();
  volumes_df = data.frame();
  
  for ( i in 1:length(all_dates)){
    if(!is_holiday(future_shc, all_dates[i])){
      
      dates = c(dates, all_dates[i]);
      temp_strikes =c();
      temp_times = c();
      
      for( j in 1:length(options_shc)){
        call_str_2 = paste(call_str, options_shc[j], all_dates[i],sep = " ");
        
        temp_str <- system( call_str_2, intern = TRUE );
        tokens = strsplit(temp_str,split = " ")[[1]];
        temp_strikes = c(temp_strikes, as.numeric(tokens[1]) );
        num_days = as.numeric( as.Date( get_r_date(tokens[2] ) ) - as.Date( get_r_date( all_dates[i] ) ) ) /365;
        temp_times = c(temp_times,num_days );
      }
      
      strikes_df = rbind(strikes_df, temp_strikes);
      times_df = rbind(times_df, temp_times);
      types_df = rbind(types_df, option_types);
      volumes_df = rbind(volumes_df, volume_weights);
    }
  }
  
  spec_df = cbind(dates, strikes_df, times_df, types_df,volumes_df);
  ilist = tempfile( );
  lines = c();
  lines = c(lines, paste( "MODELINIT", "DEPBASE", future_shc, "MidPrice", "MidPrice",sep = " " ) );
  lines = c(lines, "MODELMATH LINEAR CHANGE");
  lines = c(lines, "INDICATORSTART" );
  lines = c(lines, paste( "INDICATOR", 1.00, "SimplePriceType",future_shc, "MidPrice", sep = " " ) );
  
  for( i in 1:length(options_shc) ){
    lines = c(lines, paste( "INDICATOR", 1.00, "SimplePriceType",options_shc[i], "MidPrice", sep = " " ) );
  }
  
  lines = c( lines, paste( "INDICATOR 1.00 ImpliedVolCalculator", options_shc[atm_ind], "MidPrice")); 
  lines = c(lines, "INDICATOREND");
  
  write( lines, ilist); 
  
  options_file = tempfile(tmpdir = "/media/ephemeral2/");
  spec_file = tempfile(tmpdir = "/media/ephemeral2/");
  
  generate_timed_data_dates (ilist, options_file,begin_date, end_date,begin_time, end_time , time = 1000*60, 
                             l1_events = 0, trade_events = 0, dir = "/media/ephemeral2/", is_reg_data = TRUE, pred_dur = 100 );
  
  
  write.table(spec_df, file =spec_file, row.names = FALSE, col.names = FALSE );
  
  f_data = format_data ( options_file, spec_file);    
  write.table(f_data, formatted_data_file);
  
}



get_vol_curve_fit = function( options_shc, future_shc, begin_date, end_date, begin_time, end_time, model_size, options_file, spec_file, atm_ind, interest_rate = 0.075 ){
  
  #ilist format
  #first indicator is future price followed by option prices 
  
  source("~/basetrade/scripts/r_utils.R");
  
  temp_df = get_volume_profiles ( options_shc,begin_date, end_date, begin_time, end_time );
  options_shc = as.vector(temp_df[,5]);
  volumes     = as.vector( temp_df[,6] ) ;
  volume_weights     = volumes/sum(volumes);
  
  begin_r_date = get_r_date( begin_date);
  begin_r_date = as.Date(begin_r_date);
  
  end_r_date = get_r_date( end_date);
  end_r_date = as.Date(end_r_date);
  
  all_r_dates = seq(from = begin_r_date,to = end_r_date,by = 1);
  all_dates = c()
  
  for( i in 1:length(all_r_dates) ){
    all_dates[i] = get_str_date( all_r_dates[i] );
  }
  
  option_types = c();
  
  for( i in 1:length(options_shc) ){
    type = strsplit(options_shc[i],split = "_")[[1]][3];
    
    if(type == "C0"){
      type = "c"
    }else{
      type = "p"
    }
    
    option_types = c(option_types, type);    
  }
  
  call_str = "~/basetrade_install/bin/option_details";
  
  spec_df = data.frame();
  
  dates = c();
  strikes_df = data.frame();
  times_df = data.frame();
  types_df = data.frame();
  volumes_df = data.frame();
  
  for ( i in 1:length(all_dates)){
    if(!is_holiday(future_shc, all_dates[i])){
      
      dates = c(dates, all_dates[i]);
      temp_strikes =c();
      temp_times = c();
      
      for( j in 1:length(options_shc)){
        call_str_2 = paste(call_str, options_shc[j], all_dates[i],sep = " ");
        
        temp_str <- system( call_str_2, intern = TRUE );
        tokens = strsplit(temp_str,split = " ")[[1]];
        temp_strikes = c(temp_strikes, as.numeric(tokens[2]) );
        num_days = as.numeric( as.Date( get_r_date(tokens[1] ) ) - as.Date( get_r_date( all_dates[i] ) ) ) /365;
        temp_times = c(temp_times,num_days );
      }
      
      strikes_df = rbind(strikes_df, temp_strikes);
      times_df = rbind(times_df, temp_times);
      types_df = rbind(types_df, option_types);
      volumes_df = rbind(volumes_df, volume_weights);
    }
  }
  
  spec_df = cbind(dates, strikes_df, times_df, types_df,volumes_df);
  ilist = tempfile( );
  lines = c();
  lines = c(lines, paste( "MODELINIT", "DEPBASE", future_shc, "MidPrice", "MidPrice",sep = " " ) );
  lines = c(lines, "MODELMATH LINEAR CHANGE");
  lines = c(lines, "INDICATORSTART" );
  lines = c(lines, paste( "INDICATOR", 1.00, "SimplePriceType",future_shc, "MidPrice", sep = " " ) );
  
  for( i in 1:length(options_shc) ){
    lines = c(lines, paste( "INDICATOR", 1.00, "SimplePriceType",options_shc[i], "MidPrice", sep = " " ) );
  }
  
  lines = c( lines, paste( "INDICATOR 1.00 ImpliedVolCalculator", options_shc[atm_ind], "MidPrice")); 
  lines = c(lines, "INDICATOREND");
  
  write( lines, ilist); 

  
  generate_timed_data_dates (ilist, options_file,begin_date, end_date,begin_time, end_time , time = 1000*60*5, 
                             l1_events = 0, trade_events = 0, dir = "/media/ephemeral2/", is_reg_data = TRUE, pred_dur = 100 );
  

  write.table(spec_df, file =spec_file, row.names = FALSE, col.names = FALSE );
  
  f_data = format_data ( options_file, spec_file);  
  
  params = fit_normal_mixture_model(option_prices = f_data[,1],
                                   option_types = as.character(f_data[,2]),
                                   future_prices =  f_data[,3],
                                   strikes = f_data[,4],
                                   times = f_data[,5],
                                   interest_rate = interest_rate,
                                   model_size = model_size,
                                   volume_weights = f_data[,7]
                                   );
  
  
  return(params);
}

