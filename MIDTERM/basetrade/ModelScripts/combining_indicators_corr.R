.libPaths("/apps/R/root/library/")
suppressPackageStartupMessages( require('numDeriv') );

args <- commandArgs(trailingOnly = TRUE);
ilist_dir = toString(args[1]);
num_cv = as.integer(args[2]);
num_ind1 = as.integer(args[3]);
num_ind2 = as.integer(args[4]);
work_dir = toString(args[5]);
user_correlation_sign = as.integer(args[6]);
ind1_names = readLines(args[7]);
ind2_names = readLines(args[8]);
standardize = as.integer(args[9]);
#============================================== correlation of x,y ignoring their mean

correlation_nomean <- function( x,y ) {
  retval <- sum( x * y ) / ( sqrt( sum( x * x ) ) * sqrt( sum( y * y ) ) );
  return(retval);
 
}

#================================================== Find initial correlations for indicator 1
initial_corrs_ind1 <- function()
{ 
   for( i in 1:ncol( indicators1_test ) ){
    if (sd_indicators1_test[i] == 0) {
      indicators1_test[i] = indicators1_test[i] - mean(indicators1_test[i]); # Making indicator with 0 stdev to be all zeros
      corr_ind1_test[i] <<- 0;
    } else { 
      if(standardize == 1)
      {
        indicators1_test[i] = indicators1_test[i]/sd_indicators1_test[i];
      }
      corr_ind1_test[i] <<- roundoff (correlation_nomean( indicators1_test[,i],deltay_test ));
    }
    if( user_correlation_sign == 1){
      if( as.numeric(strsplit(ind1_names[i], ' ')[[1]][2]) > 0 ){
        initial_corr_ind1_test[i] <<- 1;
        initial_corr_ind1[i] <<- 1;
      }
      else{
        initial_corr_ind1_test[i] <<- -1;
	initial_corr_ind1[i] <<- -1;
      }
    }
    else{ 
      if( corr_ind1_test[i] > 0 ){
        initial_corr_ind1_test[i] <<- 1;
        initial_corr_ind1[i] <<- 1;
      }
      else{
        initial_corr_ind1_test[i] <<- -1;
	initial_corr_ind1[i] <<- -1;
      }
    }
  }
}

#=================================================== Find initial correlations for indicator 2
initial_corrs_ind2 <- function()
{
  for( i in 1:ncol( indicators2_test ) ){
    if (sd_indicators2_test[i] == 0) {
      indicators2_test[i] = indicators2_test[i] - mean(indicators2_test[i]); # Making indicator with 0 stdev to be all zeros
      corr_ind2_test[i] <<- 0;
    } else {
      if(standardize == 1)
      {
        indicators2_test[i] = indicators2_test[i]/sd_indicators2_test[i];
      }
      corr_ind2_test[i] <<- roundoff (correlation_nomean( indicators2_test[,i],deltay_test ));
    }
    if( user_correlation_sign == 1){
      if( as.numeric(strsplit(ind2_names[i], ' ')[[1]][2]) > 0) {
        initial_corr_ind2_test[i] <<- 1;
     	initial_corr_ind2[i] <<- 1;
       }
      else{
        initial_corr_ind2_test[i] <<- -1;
        initial_corr_ind2[i] <<- -1;
      }
    }
    else{
      if( corr_ind2_test[i] > 0 ){
        initial_corr_ind2_test[i] <<- 1;
        initial_corr_ind2[i] <<- 1;
      }
      else{
        initial_corr_ind2_test[i] <<- -1;
        initial_corr_ind2[i] <<- -1; 
     }
    }
  }
}

#=================================================
roundoff <- function(a)
{
	retval <- as.numeric ( format( round ( a, 3), nsmall = 3 ) );
}

#=================================================
sigmoid <- function ( a, b, x )
{
	retval <- 1 / ( 1 + exp( -1 * a * ( x + b ) ) );
}

#=================================================

exponent <- function ( a, b, x )
{
    retval <-  exp( -1 * a * ( x + b ) ) ;
}

#================================================= Benchmark correlation by linear regression
linear_regress <- function (b)
{

	correlation_nomean( b[1] * ind_1 + ( 1 - b[1] ) * ind_2 , deltay );
}

#================================================ correlation for constrained beta
optim_f_constrained <- function (b) 
{
	alpha <- sigmoid( b[1], b[2], cv ); 
	y_hat <- alpha * ind_1 + ( 1 - alpha ) * ind_2;
  error <- deltay - y_hat;
#retval <- correlation_nomean( ( alpha * ind_1 + ( 1 - alpha ) * ind_2 ) , deltay ); ## f(x) = sigmoid(a*cv + b) * I1 + (1-sigmoid(a*cv+b))*I2
  retval <- sum(error * error);
  return(retval);
}

#================================================ correlation for unconstrained beta
optim_f_unconstrained <-  function (b) 
{
	alpha			<- 	sigmoid( b[1], b[2], cv); 
	regress_Y 		<-	deltay - ( 1 - alpha ) * ind_2;
	regress_X		<-	alpha * ind_1;
	regress_data		<- 	data.frame( cbind(regress_Y, regress_X) );
	unconstrained_beta_val  <<-     summary(lm( regress_Y ~ 0 + regress_X, data = regress_data ) )$coefficients[1,1]; ##given the value of sig_a, sig_b find beta
  y_hat <- alpha * ind_1 * unconstrained_beta_val + ( 1 - alpha ) * ind_2;
  error <- deltay - y_hat;
	retval <-	mean( error * error ); 
	return (retval);
}

#================================================ gradient for constrained beta optim_f
AnalyticalGradNumerical_constrained <- function ( b ) {
#retval <- grad (optim_f_constrained, b,  method="simple");
#return(retval);
  grad_arr = array();
  alpha <- sigmoid( b[1], b[2], cv );
  exp_factor <- exponent( b[1], b[2], cv );
  exp_factor[which(exp_factor > 10000)] <- 10000
  grad_alpha_a <- -1*exp_factor * ( cv + b[2] ) * alpha * alpha ;
  grad_alpha_b <- -1*exp_factor * b[1] * alpha * alpha ;
  y_hat <- alpha * ind_1 + ( 1 - alpha ) * ind_2;
  grad_arr[1] =  2 * sum ( ( deltay - y_hat ) * ( grad_alpha_a * ind_1 - grad_alpha_a * ind_2 ) ) ;
  grad_arr[2] =  2 * sum ( ( deltay - y_hat ) * ( grad_alpha_b * ind_1 - grad_alpha_b * ind_2 ) ) ;

  return ( grad_arr );
}

#================================================ gradient for unconstrained beta optim_f
AnalyticalGradNumerical_unconstrained <- function ( b ) {
#retval <- grad (optim_f_unconstrained, b,  method="simple");
  grad_arr = array();
  alpha <- sigmoid( b[1], b[2], cv );
  exp_factor <- exponent( b[1], b[2], cv );
  grad_alpha_a <- -1*exp_factor * ( cv + b[2] ) * alpha * alpha ;
  grad_alpha_b <- -1*exp_factor * b[1] * alpha * alpha ;
  y_hat <- alpha * ind_1 * unconstrained_beta_val + ( 1 - alpha ) * ind_2;
  grad_arr[1] = 2 * mean ( ( deltay - y_hat ) * ( grad_alpha_a * ind_1 * unconstrained_beta_val - grad_alpha_a * ind_2 )  ) ;
  grad_arr[2] = 2 * mean ( ( deltay - y_hat ) * ( grad_alpha_b * ind_1 * unconstrained_beta_val - grad_alpha_b * ind_2 ) ) ;
  return ( grad_arr );
}

#================================================
ComputeBeta <- function ()
{
  all_alpha = roundoff( sigmoid( sig_a, sig_b, cv * sd_cv + mean_cv ) ); 
  data_all = cbind(all_alpha, ind1, ind2);
  data_g = subset( data_all, all_alpha >= 0.5 );
  if( dim(data_g)[1] > 1 )
  { 
    sd_r2 = apply( data_g, 2, sd );
    beta_r2 = paste( "r1: ", toString( roundoff( sd_r2[3] / sd_r2[2] ) ), sep="");
  }
  else{
    beta_r2 = "r1: 0" ;
	}
  data_l = subset( data_all, all_alpha < 0.5 );
  if( dim(data_l)[1] > 1 ) {
  	sd_r1 = apply( data_l, 2, sd );
		beta_r1 = paste("r2: ", toString( roundoff( sd_r1[3] / sd_r1[2] ) ), sep = "");
  }
	else{		
		beta_r1 = "r2: 0";
  }
  if( dim(data_l)[1] > 1 && dim(data_g)[1] > 1 ) {
    new_beta = paste("nb: ", toString( roundoff( sd_r1[3] / sd_r2[2] ) ), sep="");
  }
  else{
    new_beta = "nb: 0";
  }
  beta_detailed = paste( beta_r2, beta_r1, new_beta, "ob:", roundoff (beta_param2 / beta_param1), sep = " " );
  return( beta_detailed );
}

#===================================================
GetNumLinesFile <- function (file)
{
  console.output = system( sprintf("wc -l %s", file), intern = TRUE);
  console.array = strsplit (console.output, " ");
  num.lines = as.integer (console.array[[1]][1]);
  return (num.lines);
}

### Reading conditional variables and indicators and scaling indicators ###
regdata_files <- list.files( ilist_dir, pattern="filtered_regdata_filename*", full.names=TRUE);
for ( file in regdata_files){
  if (GetNumLinesFile (file) == 0){
    cat("Error: Regdata not created");
    stop ();
  }
  data = read.table(file);
  is.na(data) = sapply(data, is.infinite);
  data = data[complete.cases(data),];
  dataTrain=matrix(0,0,ncol(data))
  dataTest=matrix(0,0,ncol(data))
  prob=0.7
  split <-sample(c(TRUE, FALSE), nrow(data), replace=TRUE, prob=c(0.7, 0.3))
  dataTrain <-data[split, ]
  dataTest <- data[!split,]
  	
		
   deltay = as.matrix( dataTrain[,1] );
   conditional_variables = as.matrix( dataTrain[,2:(num_cv+1)] );
   indicators1 = as.matrix( dataTrain[ ,(num_cv+2):(num_cv+num_ind1+1) ] );
   indicators2 = as.matrix( dataTrain[ ,(num_cv+num_ind1+2):(num_cv+num_ind1+num_ind2+1) ] );
   sd_indicators1 = apply(indicators1, 2, sd); ### find sd of indicators in file 1
   sd_indicators2 = apply(indicators2, 2, sd); ### find sd of indicators in file 2
   corr_ind1 = matrix(0, ncol( indicators1 ), 1 );
   corr_ind2 = matrix( 0, ncol( indicators2 ), 1 );
   initial_corr_ind1 = matrix( 0, ncol( indicators1 ), 1 );
   initial_corr_ind2 = matrix( 0, ncol( indicators2 ), 1 );
   i1 = indicators1;
   i2 = indicators2; 
        
   deltay_test = as.matrix( dataTest[,1] );
   conditional_variables_test = as.matrix( dataTest[,2:(num_cv+1)] );
   indicators1_test = as.matrix( dataTest[ ,(num_cv+2):(num_cv+num_ind1+1) ] );
   indicators2_test = as.matrix( dataTest[ ,(num_cv+num_ind1+2):(num_cv+num_ind1+num_ind2+1) ] );
   sd_indicators1_test = apply(indicators1_test, 2, sd); ### find sd of indicators in file 1
   sd_indicators2_test = apply(indicators2_test, 2, sd); ### find sd of indicators in file 2
   corr_ind1_test = matrix(0, ncol( indicators1_test ), 1 );
   corr_ind2_test = matrix( 0, ncol( indicators2_test ), 1 );
   initial_corr_ind1_test = matrix( 0, ncol( indicators1_test ), 1 );
   initial_corr_ind2_test = matrix( 0, ncol( indicators2_test ), 1 );
   i1_test = indicators1_test;
   i2_test = indicators2_test;

### Standardizing and finding correlation for indicator 1 and inital correlations ###
   initial_corrs_ind1();
   file = gsub("//dir_ilist","", file);
   file_ind1_ = paste( file, "_corr_ind1", sep='' );
   write.table(matrix(corr_ind1_test), file_ind1_, row.names=FALSE, col.names = FALSE);

### Standardizing and finding correlation for indicator 2 and inital correlations ###
   initial_corrs_ind2();
   file_ind2_ = paste(file,"_corr_ind2",sep='');
   write.table(matrix(corr_ind2_test), file_ind2_, row.names = FALSE, col.names=FALSE);

### Choosing conditional variables one by one and finding the correlation for every pair ###
  for( k in c( 1:ncol( conditional_variables ) ) ){
    attributes = matrix(, nrow = ncol( indicators1 ), ncol( indicators2 ) );
    for( i in c( 1:( ncol( indicators1 ) ) ) ){
      for( j in c( ( 1 ):ncol( indicators2 ) ) ){
        combination_corr =  0;
        comb_increase  = 0;
        unconstrained_beta_val  = 0;
        ignore_combination = 0;

	ind_1 	= as.matrix( indicators1[,i]*initial_corr_ind1[i]); ## if negative correlation multiply by -1 to make corr +ve
	ind_2 	= as.matrix( indicators2[,j]*initial_corr_ind2[j] ); ## if negative correlaion multiply by -1 to make corr +ve
        ind1 = i1[,i];
	ind2 = i2[,j];
	cv = as.matrix( conditional_variables[,k] );
	#benchmark = optim( c(0.5), linear_regress, control = list( fnscale = -1 ) , method = "Brent", lower = 0, upper = 1 );
	mean_cv = mean( cv ); 
	sd_cv = sd( cv );
	cv = ( cv - mean_cv ) / sd_cv; ### to have reasonable constraints on cv for optim_f
				
	ind_1_test = as.matrix( indicators1_test[,i]*initial_corr_ind1_test[i]); ## if negative correlation multiply by -1 to make corr +ve
        ind_2_test = as.matrix( indicators2_test[,j]*initial_corr_ind2_test[j] ); ## if negative correlaion multiply by -1 to make corr +ve
        ind1_test = i1_test[,i];
        ind2_test = i2_test[,j];
	cv_test  = as.matrix( conditional_variables_test[,k] );
        benchmark_test = optim( c(0.5), linear_regress, control = list( fnscale = -1 ) , method = "Brent", lower = 0, upper = 1 );
	static_alpha=benchmark_test$par[1];
        linear_combination_expr=static_alpha*ind_1_test+(1-static_alpha)*ind_2_test;
	benchmark_test$value=roundoff(correlation_nomean(linear_combination_expr,deltay_test) );
	mean_cv_test = mean( cv_test );
        sd_cv_test = sd( cv_test );
        cv_test = ( cv_test - mean_cv_test ) / sd_cv_test; ### to have reasonable constraints on cv for optim_f
	
	if( standardize == 0 ){
	    p_optim = optim( c(1,0), optim_f_unconstrained,AnalyticalGradNumerical_unconstrained,method="L-BFGS-B", lower = c(0.2,-2.5), upper = c( 2.5, 2.5) );
            ### optimizing with sig_a for the limits (0.2,2.5) for thr slope of sigmoid function to avoid the value of slope as 0
	    if(p_optim$convergence == 52)    ## 52 is the code when optim function does not converge
            {
              cat("Warning: p_optim for does not converge", p_optim$message, "\n");
              cat("Combination - cv_index: ", k, " ind1_index: ", i, " ind2_index: ", j, "\n" );
              ignore_combination = 1;
            }
            alpha= sigmoid( p_optim$par[1], p_optim$par[2], cv_test);
	    p_beta = roundoff( unconstrained_beta_val ); ## beta for publishing
	    p_combination_expr = alpha * unconstrained_beta_val * ind_1_test + ( 1 - alpha ) * ind_2_test;
	    p_combination_corr	= 	roundoff( correlation_nomean( p_combination_expr, deltay_test ) );
	    n_optim = optim( c(-1,0), optim_f_unconstrained,AnalyticalGradNumerical_unconstrained,method="L-BFGS-B", lower = c(-2.5,-2.5), upper = c(-0.2,2.5) );
	    ### optimizing with sig_a for the limits (-2.5, 0.2) because of discontinuity and maintaining flexibilty of switching from any indicator
            if(n_optim$convergence == 52)  ## 52 is the code when optim function does not converge
            {
              cat("Warning: n_optim for does not converge", n_optim$message, "\n");
              cat("Combination - cv_index: ", k, " ind1_index: ", i, " ind2_index: ", j, "\n" );
              ignore_combination = 1;
            }
	    alpha =  sigmoid( n_optim$par[1], n_optim$par[2], cv_test);
            n_beta =  roundoff( unconstrained_beta_val ); ### beta for publishing
	    n_combination_expr = alpha * unconstrained_beta_val * ind_1_test + ( 1 - alpha ) * ind_2_test;
	    n_combination_corr = roundoff( correlation_nomean( n_combination_expr, deltay_test ) );
	    n_unconstrained_beta = unconstrained_beta_val;
	    if( abs(p_combination_corr) > abs(n_combination_corr) ){ ### comparing which range gives better beta
		sig_a = roundoff( p_optim$par[1] );
		sig_b = roundoff( p_optim$par[2] * p_optim$par[1] );
		beta_param1 = 1;
		beta_param2 = roundoff( p_beta );
		combination_corr = abs(p_combination_corr);
	     }
	     else {
		sig_a = roundoff( n_optim$par[1] );
		sig_b = roundoff( n_optim$par[2] * n_optim$par[1]);
		beta_param1 = 1;
		beta_param2 = roundoff(n_beta);
		combination_corr = abs(n_combination_corr);
	     }
	}
	else {  ### for standardized indicators
		p_optim	=  optim( c(1,0), optim_f_constrained,AnalyticalGradNumerical_constrained,method = "L-BFGS-B", lower = c(0.2,-2.5), upper = c(2.5,2.5) ); 

            	if(p_optim$convergence == 52)            ## 52 is the code when optim function does not converge
            	{
              	   cat("Warning: p_optim for does not converge", p_optim$message, "\n");
              	   cat("Combination - cv_index: ", k, " ind1_index: ", i, " ind2_index: ", j, "\n" );
              	   ignore_combination = 1;
            	}
            	p_alpha <- sigmoid( p_optim$par[1], p_optim$par[2], cv_test); 
	    	p_combination_corr <- roundoff(correlation_nomean( ( p_alpha * ind_1_test + ( 1 - p_alpha ) * ind_2_test ) , deltay_test ));
		n_optim	=  optim( c(-1,0), optim_f_constrained,AnalyticalGradNumerical_constrained,method = "L-BFGS-B", lower = c(-2.5,-2.5), upper = c(-0.2,2.5) ); 
            	if(n_optim$convergence == 52)            ## 52 is the code when optim function does not converge
            	{
              	  cat("Warning: n_optim for does not converge", n_optim$message, "\n");
              	  cat("Combination - cv_index: ", k, " ind1_index: ", i, " ind2_index: ", j, "\n" );
              	  ignore_combination = 1;
            	}
	  	  n_alpha <- sigmoid( n_optim$par[1], n_optim$par[2], cv_test);
          	  n_combination_corr <- roundoff(correlation_nomean( ( n_alpha * ind_1_test + ( 1 - n_alpha ) * ind_2_test ) , deltay_test ));
		  if( abs(p_combination_corr) > abs(n_combination_corr) ){
			sig_a = roundoff( p_optim$par[1] );
			sig_b = roundoff( p_optim$par[2] * p_optim$par[1]);
			combination_corr=abs(p_combination_corr);
		 }
		else {
		        sig_a = roundoff( n_optim$par[1] );
			sig_b = roundoff( n_optim$par[2] * n_optim$par[1] );
			combination_corr = abs(n_combination_corr);
		}
		beta_param1 = roundoff( sd_indicators1[i] );
		beta_param2 = roundoff( sd_indicators2[j] );
	}
	quantiles = quantile((cv*sd_cv+mean_cv),c(0.1,0.5,0.9)); #calc 10,50,90 percentiles of cv for computing alpha at these points
	alpha_val = roundoff( sigmoid( sig_a, sig_b / sig_a, quantiles ) );
	#beta_detailed = ComputeBeta();  ## could be of use later
        if(ignore_combination == 1 ){
          	comb_increase = -1;
        }
        else {
		comb_increase = roundoff ( ( ( combination_corr - abs(benchmark_test$value) ) / abs(benchmark_test$value) ) );   ### increase in corr 
        }
	mean_cv_test = roundoff(mean_cv_test);
	sd_cv_test = roundoff(sd_cv_test);
	attributes[i,j] = paste( sig_a, sig_b, mean_cv, sd_cv, beta_param1, beta_param2, comb_increase, combination_corr, alpha_val[1], alpha_val[2], alpha_val[3], sep="," );
      }
    }
		write.table( attributes, file = paste(file, "_cv_corr_", toString(k), sep=''),  row.names = FALSE, col.names=FALSE);
 }
}

