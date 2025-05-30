#!/usr/bin/env Rscript

args <- commandArgs(trailingOnly = TRUE);

if ( length ( args ) < 10 )
{
  stop ( " Usage: <dep_code> <indep_code>  <duration> <price_type> <smoothness_factor> <begin_date> <end_date> <begin_time> <end_time> <pred_dur> \n " ) ;
}

i=1;
dep_code = args[i];
i = i+1;
indep_code = args[i];
i = i+1
dur = as.numeric( args[i] );
i = i +1;
price_type = args[i];
i = i +1;
smf = as.numeric(args[i]);
i= i+1;
begin_date = args[i];
i = i+1;
end_date = args[i];
i = i+1;
begin_time = args[i];
i = i +1;
end_time = args[i];
i = i+1
pred_dur = as.numeric(args[i]);
i= i+1;

ilist = tempfile(,tmpdir= "/media/ephemeral2/");

line = paste( "MODELINIT", "DEPBASE", dep_code, price_type, price_type ) ;
write(line, ilist, append = TRUE ) ;
line = "MODELMATH LINEAR CHANGE"
write(line, ilist, append = TRUE ) ;

line = "INDICATORSTART"	
write(line, ilist, append = TRUE ) ;

line = paste ( "INDICATOR", 1, "SimpleReturns", dep_code, dur, price_type , sep = " " );
write(line, ilist, append = TRUE ) ;

line = paste ( "INDICATOR", 1, "SimpleReturns", indep_code, dur ,price_type , sep = " " );
write(line, ilist, append = TRUE ) ;


line = "INDICATOREND";
write(line, ilist, append = TRUE ) ;


#cat(ilist)
source("~/basetrade/scripts/r_utils.R" )

data_file = tempfile(  tmpdir = "/media/ephemeral2/" );
generate_timed_data_dates( ilist, data_file, begin_date, end_date, begin_time, end_time , time = 1000, l1_events = 0, trade_events = 0, dir = "/media/ephemeral2/", is_reg_data = TRUE, pred_dur = pred_dur );

lm_data = read.table(data_file )[,-(1) ] ;
sd = sd(lm_data[,1]);
temp_ind = which( abs(lm_data[,1]) > 0.1*sd ) ;
lm_data = lm_data[temp_ind,]; 

#train_data = lm_data[ 1:floor(nrow(lm_data)/2), ] ;
#test_data = lm_data[ -(1:floor(nrow(lm_data)/2)), ] ;

train_data = lm_data;
test_data = lm_data

indicator_dynamic_weights_cor = function ( train_df, test_df, y_ind, indicator_ind,  c_fac = 1, v_fac = 1 ,w_fac =1, sd_target = 1 ) {  
  require("dlm")
  require("stats")
  names(train_df)[y_ind] = "y";
  names(train_df)[indicator_ind] = "x";
  
  weights_vec =c();
  for ( i in 1:50 )
  { 
    temp_data = train_df[  floor( (i-1)*nrow(train_df)/50 ):floor(i*nrow(train_df)/50 ), ] ; 
    lm = lm( y ~ x+0, temp_data ); weights_vec = c(weights_vec, lm$coefficients) ; 
  }
  
  
  obv_var = var ( train_df[,y_ind] - ( mean(weights_vec)*train_df[,indicator_ind ] ) );
  weights_var = var(weights_vec)*c_fac;
  weights_diff_var = var( diff(weights_vec, lag = 1  ) )*w_fac ; 
  weights_mean = mean(weights_vec )

  opt_w_fac = function(param){
    dlm = dlmModReg(X = as.matrix(test_df[,indicator_ind]),addInt = FALSE,dV = obv_var*param[1],dW = weights_diff_var, m0 = weights_mean,
                    C0 = diag ( weights_var, 1)  );
    dlmfilter = dlmFilter(y = test_df[,y_ind],mod = dlm);
    a = dlmfilter$a;
    dlm_cor = cor(  test_df[,indicator_ind]*a , test_df[,y_ind] );
    
    penalty =  sd_target*var(diff(a,lag = 1) ) /( weights_diff_var );    
    #dlm_var = var(diff(a,lag = 1) );
    
    #obj = ( dlm_var - ( sd_target*weights_diff_var ) )^2;
    obj = dlm_cor - penalty; 
                
    return( -obj )  ;
    
  }
 
  indicator_ind = 3;
  y_ind = 2;
  
  res = optim(par = 1,fn = opt_w_fac,method = "L-BFGS-B",lower = c(0));
  w_fac_opt = res$par; 
  obv_var = obv_var*w_fac_opt;  
 
    dlm = dlmModReg(X = as.matrix(test_df[,indicator_ind]),addInt = FALSE,dV = obv_var*w_fac_opt, dW = weights_diff_var, m0 = weights_mean,
                    C0 = diag ( weights_var, 1)  );  

    dlmfilter = dlmFilter(y = test_df[,y_ind],mod = dlm);
    a = dlmfilter$a;
    dlm_cor = cor(test_df[,1] , test_df[,indicator_ind]*a - test_df[,y_ind] );

    #print(c("cor", dlm_cor ) );

   output = c( dep_code, indep_code, weights_var,  weights_diff_var, obv_var );

  cat( output )
  
}

  indicator_ind = 3;
  y_ind = 2;


indicator_dynamic_weights_cor ( train_data, test_data, y_ind, indicator_ind, 1,1,1, smf );
file.remove(ilist);
file.remove(data_file);






