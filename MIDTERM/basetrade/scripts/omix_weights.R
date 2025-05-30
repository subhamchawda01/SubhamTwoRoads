get_omix_weights = function( price_types, product, time_dur, begin_time, end_time, begin_date, end_date ){
  source("~/basetrade/scripts/r_utils.R");
  library("stats");
  
  ilist = tempfile();  
  lines = c();  

  lines = c(lines, paste( "MODELINIT", "DEPBASE", product, price_types[1], price_types[1],sep = " " ) );
  lines = c(lines, "MODELMATH LINEAR CHANGE");
  lines = c(lines, "INDICATORSTART" );


  for( i in 1:length(price_types ) ) {
   lines = c(lines, paste( "INDICATOR", 1.00, "SimplePriceType" ,product ,price_types[i], sep = " " ) );  
  }
 
  lines = c(lines, "INDICATOREND");
  write( lines, ilist ) ;  
  out_file = tempfile();

  generate_timed_data_dates ( ilist, out_file, begin_date, end_date, 
                              begin_time, end_time,1000, 0, 0, "/media/ephemeral2", FALSE, time_dur);   
  
  data = read.table(out_file);
  write.table( data, out_file, row.names = FALSE, col.names = FALSE ) ;

ilist_vec = c();

for( i in 1:length(price_types) ) {
  lines = c();

  lines = c(lines, paste( "MODELINIT", "DEPBASE", product, price_types[i], price_types[i], sep = " " ) );
  lines = c(lines, "MODELMATH LINEAR CHANGE");
  lines = c(lines, "INDICATORSTART" );

  lines = c(lines, paste( "INDICATOR", 1.00, "SimplePriceType" ,product ,price_types[i], sep = " " ) );
  lines = c(lines, "INDICATOREND");
   temp_ilist = tempfile();
   write( lines, temp_ilist ) ;
  ilist_vec = c(ilist_vec, temp_ilist);
  
}


  reg_data = data.frame();
  price_data = data.frame();

  for( i in 1:length(price_types) ) { 
  temp_data = data[,c(1,2, 3,5 +  i, 5+ i, 5 + i  ) ] ;
  temp_data_2 = timed_data_to_reg_data ( temp_data,ilist_vec[i], time_dur,"na_t3", begin_time, end_time );
  reg_data = rbind(reg_data, temp_data_2[,1]);
  price_data = rbind( price_data, temp_data_2[,2] ) ;
 } 
 
reg_data = t(reg_data);
price_data = t( price_data ) ;


  optim_func = function( weights ){
    
    weights = weights/sum(weights);
    cor_2 = 0 ;  
    
    new_price_series = as.numeric( as.matrix ( price_data ) %*% weights );
    new_price_diff = rep(0, length(new_price_series)); 
    
    new_price_diff = as.matrix( reg_data ) %*% weights ;
    new_price_diff = as.data.frame(new_price_diff );    
    cor_2 = as.numeric( cor( new_price_diff, price_data - new_price_series ) ) ;
    cor_2 = mean(cor_2^2)
    
    return(cor_2);
    
  }
  
  
  res = optim(  rep(1/length(price_types), length(price_types)), optim_func, method = "L-BFGS-B",
                lower = rep( 0, length( price_types ) ) ,upper = rep( 1, length(price_types)  )  );
  
  if( res$convergence ==  1 ) {
    res = optim(  rep(1/length(price_types), length(price_types)), optim_func, method = "L-BFGS-B",
                  lower = rep( 0, length( price_types ) ) ,upper = rep( 1, length(price_types)  ) , control  = list( maxit = 2e4) );
  }  
  
  
  weights =  res$par;
  weights = weights/sum(weights);
  
  return(weights);
  
}
