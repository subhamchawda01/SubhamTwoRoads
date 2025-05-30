format_data = function(data, mse_weights){
  
  ind = which( data[,5] > 2/365 );
  data = data[ind, ];
  
  num_options = length(mse_weights);
  interest_rate = data[,(5*num_options + 2 )][1];
  
  mse_weights = mse_weights/sum(mse_weights);
    
  num_options = length(mse_weights);
  interest_rate = data[,(5*num_options + 2 )][1];
    
  all_options_df = data.frame();
  fut_data = as.numeric(data[,(5*num_options +1 )] );
  names = c("option_price","vol", "strike", "type", "ttm", "fut_price", "mse_weight" );
    
    for( i in 1:num_options ){
      data[,(i-1)*5 + 2] =   data[,(i-1)*5 + 2]/100;
      
      temp_df = data[,((i-1)*5 + 1):((i-1)*5 + 5)];
      types = apply( as.data.frame(temp_df[, 4 ]), MARGIN =  1,FUN = function(x) { t="c";if(x == -1){t = "p"}; return(t); } );
      temp_df = cbind(temp_df, fut_data);
      
      price_norm = (mean(data[,(i-1)*5 + 1]) )^2;
      
      #temp_df = cbind(temp_df, rep(mse_weights[i],nrow(temp_df) ) );
      temp_df = cbind(temp_df, rep(mse_weights[i],nrow(temp_df))/(price_norm*nrow(temp_df) ) );
      colnames(temp_df) = names;
      temp_df$mse_weight = temp_df$mse_weight/sum(temp_df$mse_weight);
      temp_df$type = types;
      all_options_df = rbind(all_options_df, temp_df);
    }
    
  norm_const = sum( mse_weights*nrow(data) );
  all_options_df$mse_weight = all_options_df$mse_weight/norm_const;
  
  return(all_options_df);
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
    #mse = sqrt( sum( (option_prices - model_option_prices)^2*(volume_weights) ) / length(option_prices) );
    mse = sqrt( sum( (option_prices - model_option_prices)^2*(volume_weights) )  );
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
  opt = optim(par = param,fn = optim_func ,control = list(maxit = 1e7,abstol = 0.01) ); 
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
  
  

fit_vol_surface_2= function( data, mse_weights, mixture_size, maturity = 22/365 ){
  
  ind = which( data[,5] > 2/365 );
  data = data[ind, ];  
  
  require(fOptions);
  num_options = length(mse_weights);
  interest_rate = data[,(5*num_options + 2 )][1];
  
  if(TRUE) {
    mse_weights = mse_weights/sum(mse_weights);
    
    num_options = length(mse_weights);
    interest_rate = data[,(5*num_options + 2 )][1];
    
    all_options_df = data.frame();
    fut_data = as.numeric(data[,(5*num_options +1 )] );
    names = c("option_price","vol", "strike", "type", "ttm", "fut_price", "mse_weight" );
    
    for( i in 1:num_options ){
      data[,(i-1)*5 + 2] =   data[,(i-1)*5 + 2]/100;
      
      temp_df = data[,((i-1)*5 + 1):((i-1)*5 + 5)];
      types = apply( as.data.frame(temp_df[, 4 ]), MARGIN =  1,FUN = function(x) { t="c";if(x == -1){t = "p"}; return(t); } );
      temp_df = cbind(temp_df, fut_data);
      
      price_norm = (mean(data[,(i-1)*5 + 1]) )^2;
      
      #temp_df = cbind(temp_df, rep(mse_weights[i],nrow(temp_df) ) );
      temp_df = cbind(temp_df, rep(mse_weights[i],nrow(temp_df))/(price_norm*nrow(temp_df) ) );
      colnames(temp_df) = names;
      temp_df$mse_weight = temp_df$mse_weight/sum(temp_df$mse_weight);
      temp_df$type = types;
      all_options_df = rbind(all_options_df, temp_df);
    }
    
    norm_const = sum( mse_weights*nrow(data) );
    all_options_df$mse_weight = all_options_df$mse_weight/norm_const;
  
}
    #all_options_df = format_data(data, mse_weights );
    
    params = fit_normal_mixture_model ( option_prices = all_options_df$option_price, 
                                        option_types= all_options_df$type, 
                                        future_prices =all_options_df$fut_price, 
                                        strikes= all_options_df$strike, 
                                        times =all_options_df$ttm, 
                                        interest_rate = interest_rate, 
                                        volume_weights = all_options_df$mse_weight,
                                        model_size =  mixture_size 
                                    );
    
}

plot_model_vols = function( param, option_prices, option_types, future_prices, strikes, times, interest_rate, atm_vols  ){
  
  model_size = length(param)/3;
  stock_prices = future_prices*exp(-interest_rate*times);  
  ind_1 = which( option_types == "c");
  ind_2 = which( option_types == "p");
  
  weights = param[1:(model_size)];
  mean_vec = param[ (model_size +1): ( 2*model_size  ) ]; 
  vol_vec = sqrt( param[ ( ( 2*model_size ) + 1 ): ( 3 * model_size )  ]^2);      
  
  weights = weights * weights/(sum(weights*weights));
  
  model_option_prices = rep(0, length(option_prices));    
  model_atm_option_prices = rep(0, length(option_prices));    
  
  norm_const = exp(interest_rate*times[1])/sum( weights*exp(mean_vec*times[1]) );
  norm_const = log(norm_const)/times[1];
  mean_vec = mean_vec + norm_const;
  
  if( ( length(ind_1) ==0 ) || ( length(ind_2) == 0 )  ){
    
    for( i in 1:length(mean_vec) ){
      temp_price = GBSOption(TypeFlag = option_types,S = stock_prices*exp(mean_vec[i]*times),X = strikes,Time = times,r = mean_vec[i],b = 0,sigma = vol_vec[i]  )@price;
      temp_price_atm = GBSOption(TypeFlag = option_types,S = stock_prices*exp(mean_vec[i]*times),X = stock_prices*exp(mean_vec[i]*times),Time = times,r = mean_vec[i],b = 0,sigma = vol_vec[i]  )@price;
      
      model_option_prices = model_option_prices + ( weights[i]*temp_price );      
      model_atm_option_prices = model_atm_option_prices + ( weights[i]*temp_price_atm );      
      
    }   
  }else{
    for( i in 1:length(mean_vec) ){
      
      temp_price_1 = GBSOption(TypeFlag = option_types[ind_1][1],S = stock_prices[ind_1]*exp(mean_vec[i]*times[ind_1]),
                               X = strikes[ind_1],Time = times[ind_1],r = mean_vec[i],b = 0,sigma = vol_vec[i]  )@price;
      
      temp_price_1_atm = GBSOption(TypeFlag = option_types[ind_1][1],S = stock_prices[ind_1]*exp(mean_vec[i]*times[ind_1]),
                               X = stock_prices[ind_1]*exp(mean_vec[i]*times[ind_1]),Time = times[ind_1],r = mean_vec[i],b = 0,sigma = vol_vec[i]  )@price;
      
      temp_price_2 = GBSOption(TypeFlag = option_types[ind_2][1],S = stock_prices[ind_2]*exp(mean_vec[i]*times[ind_2]),
                               X = strikes[ind_2],Time = times[ind_2],r = mean_vec[i],b = 0,sigma = vol_vec[i]  )@price;
      
      temp_price_2_atm = GBSOption(TypeFlag = option_types[ind_2][1],S = stock_prices[ind_2]*exp(mean_vec[i]*times[ind_2]),
                               X = stock_prices[ind_2]*exp(mean_vec[i]*times[ind_2]),Time = times[ind_2],r = mean_vec[i],b = 0,sigma = vol_vec[i]  )@price;
      
      model_option_prices[ind_1] = model_option_prices[ind_1] + ( weights[i]*temp_price_1 );      
      model_option_prices[ind_2] = model_option_prices[ind_2] + ( weights[i]*temp_price_2 );
      
      model_atm_option_prices[ind_1] = model_atm_option_prices[ind_1] + ( weights[i]*temp_price_1_atm );      
      model_atm_option_prices[ind_2] = model_atm_option_prices[ind_2] + ( weights[i]*temp_price_2_atm );
      
    }  
    
  }
  
  model_vols =c ();
  obs_vols = c();
  model_atm_vols = c();
  
  for( i in 1:length(option_prices ) ){
    model_vols[i] = GBSVolatility(price  = model_option_prices[i],TypeFlag = option_types[i],S = stock_prices[i]*exp(interest_rate*times[i]),
                                  X = strikes[i],Time = times[i],r = interest_rate,b = 0) ;
    obs_vols[i] = GBSVolatility(price  = option_prices[i],TypeFlag = option_types[i],S = stock_prices[i]*exp(interest_rate*times[i]),
                                X = strikes[i],Time = times[i],r = interest_rate,b = 0) ;
    
    model_atm_vols[i] = GBSVolatility(price  = model_atm_option_prices[i],TypeFlag = option_types[i],S = stock_prices[i]*exp(interest_rate*times[i]),
                                      X = stock_prices[i]*exp(interest_rate*times[i]),Time = times[i],r = interest_rate,b = 0) ;
  }
  
  #model_vols = model_vols + ( atm_vols - model_atm_vols );
  #x11();
  #plot(obs_vols, col = "red", type = "l");
  # x11();
  #plot( model_vols, col = "green", type = "l");
  
  #plot( model_vols - obs_vols, col = "green", type = "l");
  
  
  #mse = sum( (model_vols - obs_vols)^2 /length(model_vols) );
  #print(mse);
  return(  data.frame(model_vols , obs_vols ) );
}
    
  
  test_vol_surface = function( train_file, test_file, out_file, num_options = 7, params_file = "" ){
    
    train_data = read.table(train_file);
    test_data = read.table(test_file);
    
    data = train_data[,-(1:2)]; 
    
    ind = seq(from =1, to = nrow(data), by = 60) ; 
    data = data[ind, ] ; 
    ind = which( data[,5] > 2/365 );
    data = data[ind, ];
    atm_vol = data[, ncol(data)];
    
    data = data[,-ncol(data)]; 
    
    
    param = fit_vol_surface_2(data = data,mse_weights = rep(1/num_options,num_options),mixture_size = 7,maturity = 22/365);
    if(params_file !=""){
      write(as.character(param[[4]]),file = params_file);
    }  
    
    data = test_data[,-(1:2)];
    
    ind = which( data[,5] > 2/365 );
    data = data[ind, ];
    ind = seq(from =1, to = nrow(data), by = 60) ; 
    data = data[ind, ] ; 
    
    atm_vol = data[, ncol(data)];
    data = data[,-ncol(data)]; 
    
    f_data =  format_data( data = data,mse_weights = rep(1/num_options,num_options) );
    interest_rate = data[,(5*num_options + 2 )][1];
    
    ind = 1:nrow(data);
    otm_p = plot_model_vols(param = param[[4]],option_prices = f_data$option_price[ind], option_types = f_data$type[ind],
                        future_prices = f_data$fut_price[ind], strikes = f_data$strike[ind],times = f_data$ttm[ind],
                        interest_rate = interest_rate ,atm_vols = atm_vol);
    
    ind = ( 1:nrow(data) ) + 3*nrow(data);
    atm_p = plot_model_vols(param = param[[4]],option_prices = f_data$option_price[ind], option_types = f_data$type[ind],
                            future_prices = f_data$fut_price[ind], strikes = f_data$strike[ind],times = f_data$ttm[ind],
                            interest_rate = interest_rate,atm_vols = atm_vol);
    
    
    ind = ( 1:nrow(data) ) + 6*nrow(data);
    otm_c = plot_model_vols(param = param[[4]],option_prices = f_data$option_price[ind], option_types = f_data$type[ind],
                            future_prices = f_data$fut_price[ind], strikes = f_data$strike[ind],times = f_data$ttm[ind],
                            interest_rate = interest_rate,atm_vols = atm_vol);
    
    vols = otm_p;
    vols = cbind(vols,  atm_p);
    vols = cbind(vols, otm_c);
    
    write.table(vols,file = out_file,row.names = FALSE, col.names = FALSE);
    
  }

  
  test_all_data = function( ){
    
    train_files_list = "~/nse_option_train_files";
    test_files_list = "~/nse_option_test_files";
    
    con = file(train_files_list); 
    train_files = readLines(con = con )
    
    con = file(test_files_list); 
    test_files = readLines(con = con );
    
    for ( i in 1:length(train_files ) ) { 
      curr_train_file = train_files[i]; 
      curr_test_file = test_files[i];
      out_file = strsplit( curr_train_file,split = "/" )[[1]];
      out_file = out_file[length(out_file)];
      curr_out_file = paste("~/vol_data/",out_file,sep="");
      curr_param_file = paste("~/param_data/",out_file,sep="");
      
      test_vol_surface ( curr_train_file, curr_test_file, curr_out_file, num_options = 7 ,params_file = curr_param_file);
      
      
   }
    
    
    
  }

  test_data_vol_surface = function(  test_file, out_file, num_options , params_file  ){
    
    test_data = read.table(test_file);
    param = as.numeric(read.table(params_file)[,1]);
    
    data = test_data[,-(1:2)];
    
    ind = which( data[,5] > 2/365 );
    data = data[ind, ];
    ind = seq(from =1, to = nrow(data), by = 60) ; 
    data = data[ind, ] ; 
    
    atm_vol = data[, ncol(data)];
    data = data[,-ncol(data)]; 
    
    f_data =  format_data( data = data,mse_weights = rep(1/num_options,num_options) );
    interest_rate = data[,(5*num_options + 2 )][1];
    
    ind = (1:nrow(data) ) + 2*nrow(data);
    otm_p = plot_model_vols(param = param,option_prices = f_data$option_price[ind], option_types = f_data$type[ind],
                            future_prices = f_data$fut_price[ind], strikes = f_data$strike[ind],times = f_data$ttm[ind],
                            interest_rate = interest_rate ,atm_vols = atm_vol);
    
    ind = ( 1:nrow(data) ) + 3*nrow(data);
    atm_p = plot_model_vols(param = param,option_prices = f_data$option_price[ind], option_types = f_data$type[ind],
                            future_prices = f_data$fut_price[ind], strikes = f_data$strike[ind],times = f_data$ttm[ind],
                            interest_rate = interest_rate,atm_vols = atm_vol);
    
    
    ind = ( 1:nrow(data) ) + 4*nrow(data);
    otm_c = plot_model_vols(param = param,option_prices = f_data$option_price[ind], option_types = f_data$type[ind],
                            future_prices = f_data$fut_price[ind], strikes = f_data$strike[ind],times = f_data$ttm[ind],
                            interest_rate = interest_rate,atm_vols = atm_vol);
    
    vols = otm_p;
    vols = cbind(vols,  atm_p);
    vols = cbind(vols, otm_c);
    
    write.table(vols,file = out_file,row.names = FALSE, col.names = FALSE);
    
  }
  
