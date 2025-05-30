 
format_data = function( options_file, contract_file){
  
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
  
  dates = contract_specification_data[1,];
  
  num_elem_per_day = c();
  
  for( i in 1:length(dates) ){
    ind = which( dates[i] == options_data[,1]);
    num_elem_per_day[i] = length(ind);
  }
  
  strikes_data = cbind(contract_specification_data, num_elem_per_day );
  
  strikes_data_expanded <- strikes_data[ rep(row.names(strikes_data), strikes_data[,ncol( strikes_data )] ), 1:( ncol( strikes_data ) -1 )];
  strike_prices = strikes_data_expanded[,2:(num_strikes+1)];
  strike_prices = as.vector(t(strike_prices));
  
  times = strikes_data_expanded[,(num_strikes+2):(2*num_strikes +1 )];
  times = as.vector(t(times));
  
  option_types = strikes_data_expanded[,(2*num_strikes +2 ):(3*num_strikes +1)];
  option_types = as.vector(t(option_types));
  
  all_df = data.frame(option_prices, option_types, future_prices, strike_prices, times, atm_vols);
}

fit_normal_mixture_model = function( option_prices, option_types, future_prices, strikes, times, atm_vols ,interest_rate, model_size  ) {
  require("fOptions");
  require("stats");
     
  stock_prices = future_prices*exp(-interest_rate*times);
  
  optim_func = function( param ){
    
    theta_vec = param[1:(model_size-1)];
    mean_vec = param[ model_size: ( ( 2*model_size ) -2 ) ]; 
    vol_vec = param[ ( ( 2*model_size ) - 1 ): ( ( 3 * model_size )- 3 ) ];    
    
    weights = c(); 
    sin_vec = c(1, sin(theta_vec) );
    cos_vec = cos(theta_vec);
    
    for ( k in 1:length(theta_vec)){
      weights[k] = cos_vec[k]*prod(sin_vec[1:k]);      
    }
    
    weights[model_size] = prod(sin_vec);
    weights = weights * weights;
   
    model_option_prices = rep(0, length(option_prices));
    atm_mean = exp(interest_rate*times) ;
    
    for( i in 1:length(mean_vec) ){
      temp_price = GBSOption(TypeFlag = option_types,S = stock_prices,X = strikes,Time = times,r = mean_vec[i],b = 0,sigma = vol_vec[i]  )@price;
      model_option_prices = model_option_prices + weights[i]*temp_price;
      atm_mean = atm_mean - weights[i]*exp(mean_vec[i]*times);
    }
    atm_mean = atm_mean/weights[model_size];    
    atm_mean = log( atm_mean)/times;
    
    temp_price = GBSOption(TypeFlag = option_types,S = stock_prices,X = strikes,Time = times,r = atm_mean,b = 0,sigma = atm_vols  )@price;
    
    model_option_prices = model_option_prices + weights[model_size]*temp_price;
    
    mse = sqrt( sum( (option_prices - model_option_prices)^2 ) / length(option_prices) );
     return(mse);
  }
  
  param = rep( 1/(model_size - 1), model_size -1   );
  param = c( param, rep( interest_rate, model_size - 1 ) );
  param = c( param, rep( mean(atm_vols), model_size - 1 ) ); 
  optim_param = optim(par = param,fn = optim_func ); 
  
  theta_vec = param [1:(model_size - 1)];
  weights = c(); 
  sin_vec = c(1, sin(theta_vec) );
  cos_vec = cos(theta_vec);
  
  for ( k in 1:length(theta_vec)){
    weights[k] = cos_vec[k]*prod(sin_vec[1:k]);      
  }
  
  weights[model_size] = prod(sin_vec);
  weights = weights * weights;
  
  mean_vec = param[model_size:(2*model_size-1)];
  vol_vec = param [(2*model_size):(3*model_size - 1)];  
  
  return(c(weights, mean_vec, vol_vec));
}

get_vol_curve_fit = function( options_shc, future_shc, begin_date, end_date, begin_time, end_time, model_size, options_file, spec_file, atm_ind, interest_rate = 0.075 ){
  
  #ilist format
  #first indicator is future price followed by option prices 
  
  source("~/basetrade/scripts/r_utils.R");
   
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
  
  call_str = "~/basetrade_install/bin/option_details"
  
  spec_df = data.frame();
  
  dates = c();
  strikes_df = data.frame();
  times_df = data.frame();
  types_df = data.frame();
  
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
    }
  }
  
  spec_df = cbind(dates, strikes_df, times_df, types_df);
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
  
  #options_file = tempfile(tmpdir =  "/media/ephemeral2/" );
  
  generate_timed_data_dates (ilist, options_file,begin_date, end_date,begin_time, end_time , time = 1000, 
                                       l1_events = 0, trade_events = 0, dir = "/media/ephemeral2/", is_reg_data = TRUE, pred_dur = 100 );
  
  #spec_file = tempfile(tmpdir = "/media/ephemeral2/" );
  write.table(spec_df, file =spec_file, row.names = FALSE, col.names = FALSE );
  
  all_data = format_data ( options_file, spec_file);  

  
  params = fit_normal_mixture_model ( option_prices = all_data[,1], option_types = all_data[,2], future_prices = all_data[,3], 
                             strikes = all_data[,4], 
                             times = all_data[,5],
                             atm_vols = all_data[,6] ,
                             interest_rate, model_size  
                           );
  
  return(params);
}

get_log_normal_mixure_prices = function(weights, mean_vec, vol_vec, atm_ind, option_types,strikes, future_price, time, obs_option_prices,interest_rate = 0.075){
  
  stock_price = future_price*exp(-interest_rate * time);
  atm_vol = GBSVolatility(price  = obs_option_prices[atm_ind],TypeFlag = option_types[atm_ind],S = stock_price,X = strikes[atm_ind],
                          Time = time,r = 0.075,b = 0) ;
  atm_mean = exp(interest_rate*time) ;
  
  for( i in 1:length(mean_vec) ){    
    atm_mean = atm_mean - weights[i]*exp(mean_vec[i]*time);
  }
  atm_mean = atm_mean/weights[length(weights)];    
  atm_mean = log( atm_mean)/time;
  
  
  #temp_price = GBSOption(TypeFlag = option_types,S = stock_price,X = strikes,Time = time,r = atm_mean,b = 0,sigma = atm_vol  )@price;
  option_prices = 0;
  for( i in 1:length(mean_vec)){
    option_prices = option_prices+ 
      weights[i]*GBSOption(TypeFlag = option_types,S = stock_price,X = strikes,Time = time,r = mean_vec[i],b = 0,sigma = vol_vec[i]  )@price;
  }
  
  option_prices = option_prices + 
    weights[length(weights)]*GBSOption(TypeFlag = option_types,S = stock_price,X = strikes,Time = time,r = atm_mean,b = 0,sigma = atm_vol  )@price;

  model_vols =c ();
  obs_vols = c();
  for( i in 1:length(option_prices ) ){
    model_vols[i] = GBSVolatility(price  = option_prices[i],TypeFlag = option_types[i],S = stock_price,X = strikes[i],Time = time,r = 0.075,b = 0) ;
    obs_vols[i] = GBSVolatility(price  = obs_option_prices[i],TypeFlag = option_types[i],S = stock_price,X = strikes[i],Time = time,r = 0.075,b = 0) ;
  }
  
  model_vols = model_vols + ( obs_vols[atm_ind] - model_vols[atm_ind]);
  
  plot(obs_vols, col = "red", type = "l");
  lines( model_vols, col = "green", type = "l");
  print(sqrt(sum( (model_vols - obs_vols )^2 )/9 ) );
  return(option_prices);
  
}
