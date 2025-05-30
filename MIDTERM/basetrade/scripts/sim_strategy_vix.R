test_pca_strategy = function(signal_data_frame, price_data_frame, port_vec, target_level, thres ,max_pos, allowed_loss, utc = 1000  )
{
  buy_val = 0;
  sell_val = 0;
  num_buy = 0;
  num_sell =0;
  profit_vals = c();
  signals = c();
  means = c();
  corrs = c();
  port_val_series = c()
 max_profit = 0;
max_drawdown = 0; 
 
slippage = sum( abs(port_vec) )*0.03 * utc;
#slippage = 0; 
#print("slippage");
#print(slippage);

  for ( i in 1:nrow( signal_data_frame ) )
  {
    price_vec = price_data_frame[i,];
    port_val = sum ( price_vec * port_vec );
    port_val_series[i] = port_val;
    signal = ( sum( signal_data_frame[i,] * port_vec ) ) - target_level ;
profit_vals[i] =  ( ( num_buy* port_val_series[i] * utc ) - buy_val ) + ( sell_val - ( num_sell * port_val_series[i] * utc ) );
   
 max_profit = max( profit_vals[i], max_profit );
   max_drawdown = max ( max_profit - profit_vals[i], max_drawdown );

#print( c( "signal", signal, "target_level", target_level,"num_buy", num_buy, "num_sell", num_sell, "port_val", port_val ));  

 if( profit_vals[i] < -allowed_loss)
    {
      
      #print( profit_vals[i]);
      break;
    }

    
    threshold_up = thres;
    threshold_down = -thres;
    
    signals = c( signals, signal );
    
     if (signal  < threshold_down )
    {
      
      if( ( num_buy - num_sell ) < max_pos )
      {
        buy_val = buy_val + ( port_val*utc ) + slippage ;
        num_buy  =  num_buy + 1; 
      }
    }
    if (signal > threshold_up )
    {
      
      
      if( (num_sell - num_buy ) < max_pos )
      {
        sell_val = sell_val + ( port_val * utc )- slippage;
        num_sell = num_sell + 1;
      }
    }
    
    profit_vals[i] =  ( ( num_buy* port_val_series[i] * utc ) - buy_val ) + ( sell_val - ( num_sell * port_val_series[i] * utc ) );
 #  max_profit = max( profit_vals[i], max_profit );
 #  max_drawdown = max ( max_profit - profit_vals[i], max_drawdown );    

    if( profit_vals[i] < -allowed_loss)
    {
     # print( profit_vals[i]);
      break;
    }
  }  
  
  #plot( corrs );
  # par( new= T);
  #plot()
  
  return( list( profit_vals, port_val_series, signals,  max_drawdown, max( num_buy, num_sell) ) );
  
}








get_intiger_pca_vector = function( prin_comp, space_size, max_pos  )
{
  library("Rglpk");
  ortho_space = prin_comp[ 1:space_size, ];
  ortho_mat =    ( as.matrix( ortho_space ) ) ;
  
  if( space_size == 1)
    ortho_mat =    t( as.matrix( ortho_space ) ) ;   
  
  mat = ortho_mat;
  dir = rep( "==", space_size );
  rhs = rep(0, space_size );
  obj = prin_comp[, space_size + 1 ];
  types = rep("I", nrow( prin_comp ) );
  max = TRUE;
  bounds = list ( lower = list( ind = 1:nrow ( prin_comp ), val = rep(-max_pos, nrow( prin_comp ) ) ),
                  upper = list ( ind = 1:nrow(prin_comp ) , val = rep (max_pos, nrow(prin_comp  ) ) )  
                  )  
  sol = Rglpk_solve_LP(obj = obj, mat = mat, dir = dir, rhs =  rhs,types = types, max = max,bounds = bounds );
  
  vec = sol$solution;
  return(vec)
}


