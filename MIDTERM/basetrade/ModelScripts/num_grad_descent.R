#! /usr/bin/env Rscript
.libPaths("/apps/R/root/library/") ; # probably not needed after ~/.Renviron change
suppressPackageStartupMessages ( require ( 'numDeriv' ) ) ;

use_L1_penalty = FALSE;
L1_lambda = 0.0;
use_L2_penalty = FALSE;
L2_lambda = 0.0;
use_L0_penalty = FALSE;
L0_lambda = 0.0;
use_autocorr_penalty = FALSE;
AutoCorr_lambda = 0.0;

window_size_ = 10000;

max_indep_correlation = 0.8;
max_model_size = 15;

final_correlation <- 0;
final_mse <- 0;

r2_original <- 0;
r2_model <- 0;
r_squared <- 0;

sd_dep <- 0;
sd_model <- 0;
sd_residual <- 0;

error_type = "L2";
args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 5 ) {
	stop ("USAGE : <script> <regdata> <error_type> <reg_output_file> <max_model_size> <max_indep_correlation>\n");	
}

regdata <- read.table(args[1]);
regdata <- as.matrix(regdata);

X <- regdata[,2:ncol(regdata)];
Y <- regdata[,1]

N <- dim(X)[1]

num_indicators <- dim(X)[2] ;

num_reduced_indicators <- 0;

B <- rep (0, num_indicators ) ;
new_X <- c( ); 
current_index <- 1;
ilist_mode <- TRUE;
indicators_included <- c( );

error_type <- args[2];
reg_output_file <- args[3];
max_model_size <- as.numeric(args[4]);
max_indep_correlation <- as.numeric(args[5]);

ReduceNumIndicators <- function ( )
{
	ind_obj_map = rep(0,num_indicators) ;

	for ( i in c(1:num_indicators) )
	{
		current_index <<- i;

		if(error_type=="L2") {
			res <- optim(0, L2, L2GradNumerical, method = "BFGS") ;
		}else if ( error_type=="L1") {
			res <- optim(0, L1, L1GradNumerical, method = "BFGS") ;
		}else if ( error_type=="L1L2") {
			res <- optim(0, L1L2, L1L2GradNumerical, method = "BFGS") ;
		}
			
		ind_obj_map [i] = res$value ;
	}
	
	sorted_indices <- order ( ind_obj_map ) ;
	
	for ( i in c(1:num_indicators) )
	{
		if ( num_reduced_indicators >= max_model_size )
		{
			break;		
		}
	
		if(i==1)
		{
			num_reduced_indicators <<- num_reduced_indicators + 1;
			indicators_included <<- append ( indicators_included, sorted_indices[i] );
			new_X <<- X[,sorted_indices[i]];
			new_X <<- as.matrix(new_X);
		}
		else
		{
			correlated = FALSE;

			for ( j in indicators_included ) 
			{
				if ( abs ( cor ( X [,j], X[,sorted_indices[i]] ) ) > max_indep_correlation ) 
				{
					correlated = TRUE;
					break;
				}
			}
	
			if (correlated==FALSE)
			{
				num_reduced_indicators <<- num_reduced_indicators + 1;
				indicators_included <<- append ( indicators_included, sorted_indices[i] );		
				new_X <<- cbind(new_X, X[,sorted_indices[i]]);
			}
		}			
	}
}

ComputeAutoCorr <- function ( Y, Y_hat, window_size )
{
	retVal <- 0; # TODO 	
}

L2 <- function ( B ) {

	if(ilist_mode)
	{
		tmp <- rep(0,num_indicators);
		tmp[current_index] <- B;		
		B <- tmp;
	}
	
	B <- as.matrix(B);
	Y_hat <- X %*% B ;
	retVal <- sum ( ( Y - Y_hat ) * ( Y - Y_hat ) ) / ( N * 2 ) ; 
	
	if ( use_L1_penalty )
	{
	    retVal <- retVal + L1_lambda * sum ( abs(B) / N );
	}
	else if ( use_L2_penalty )
	{
	    retVal <- retVal + L2_lambda * sum ( B * B / N );
	}
	else if ( use_L0_penalty )
	{
	    retVal <- retVal + L0_lambda * length(B);
	}
	else if ( use_autocorr_penalty )
	{
	    autoCorr <- ComputeAutoCorr ( Y,Y_hat,window_size );
	    retVal <- retVal + AutoCorr_lambda * autoCorr;
	}
	retVal
}

L1 <- function ( B ) {

        if(ilist_mode)
        {
                tmp <- rep(0,num_indicators);
                tmp[current_index] <- B;   
                B <- tmp;
        }

        Y_hat <- X %*% B ;
	retVal <- sum ( abs ( Y - Y_hat ) ) / N ;

        if ( use_L1_penalty )
        {
            retVal <- retVal + L1_lambda * sum ( abs(B) / N );
        }
        else if ( use_L2_penalty )
        {
            retVal <- retVal + L2_lambda * sum ( B * B / N );
        }
        else if ( use_L0_penalty )
        {
            retVal <- retVal + L0_lambda * length(B);
        }
        else if ( use_autocorr_penalty )
        {
            autoCorr <- ComputeAutoCorr ( Y,Y_hat,window_size );
            retVal <- retVal + AutoCorr_lambda * autoCorr;
        }
        retVal
	
}

L1L2 <- function ( B ) {

        if(ilist_mode)
        {
                tmp <- rep(0,num_indicators);
                tmp[current_index] <- B;
                B <- tmp;
        }

        Y_hat <- X %*% B ;	
	diff <- Y - Y_hat ;
	retVal <- sum ( 2 * ( sqrt ( 1 + diff * diff / 2 ) - 1 ) ) / N ;

        if ( use_L1_penalty )
        {
            retVal <- retVal + L1_lambda * sum ( abs(B) / N );
        }
        else if ( use_L2_penalty )
        {
            retVal <- retVal + L2_lambda * sum ( B * B / N );
        }
        else if ( use_L0_penalty )
        {
            retVal <- retVal + L0_lambda * length(B);
        }
        else if ( use_autocorr_penalty )
        {
            autoCorr <- ComputeAutoCorr ( Y,Y_hat,window_size );
            retVal <- retVal + AutoCorr_lambda * autoCorr;
        }
        retVal

}

L2GradAnalytical <- function ( B ) {
	Y_hat <- X %*% B ;
	diff  <- Y_hat - Y ;
	diff <- as.matrix(diff);
	retVal <- ( t(diff) %*% X ) / N ;	
}

L1L2GradAnalytical <- function ( B ) {

        if(ilist_mode)
        {
                tmp <- rep(0,num_indicators);
                tmp[current_index] <- B;
                B <- tmp;
        }
	
        Y_hat <- X %*% B ;
        diff  <- Y_hat - Y ;
	tmp <- diff / sqrt ( 1 + diff * diff / 2 );
	diff <- as.matrix(diff);
	retVal <- ( t(diff) %*% X ) / N ;
}

L2GradNumerical <- function ( B ) {
	retVal <- grad (L2,B,method="simple");
}

L1GradNumerical<- function ( B ) {
	retVal <- grad (L2,B,method="simple");
}

L1L2GradNumerical <- function ( B ) {
	retVal <- grad (L1L2,B,method="simple");
}

ComputeModelStatistics <- function ( ) {
	Y_hat = X %*% Beta;  	# this needs to change if we are using a non-linear/different model
	final_mse <<- sum ( ( Y_hat - Y ) * ( Y_hat - Y ) ) / ( 2 * length ( Y ) );
	final_correlation <<- cor ( Y_hat, Y );
	r2_original <<- sum ( Y ^ 2 );
	r2_model <<- sum ( Y_hat ^ 2 );
	r2_residual <<- sum ( ( Y_hat - Y ) ^ 2 );
	r_squared <<- 1 - r2_residual / r2_original;
	
	sd_dep <<- sd (Y);
	sd_model <<- sd (Y_hat);
	sd_residual <<- sd ( Y_hat - Y );
}


if ( num_indicators > max_model_size ) {
	ReduceNumIndicators();
} else {
	indicators_included <- c(1:num_indicators);
	num_reduced_indicators <- num_indicators;
	new_X <- X;
}

ilist_mode = FALSE;
X <- new_X;
B <- rep (0, num_reduced_indicators ) ;
if(error_type == "L2" ) {
	res <- optim(B, L2, L2GradNumerical, method = "BFGS");
} else if ( error_type == "L1" ) {
	res <- optim(B, L1, L1GradNumerical, method = "BFGS");
} else if ( error_type == "L1L2" ) {
	res <- optim(B, L1L2, L1L2GradNumerical, method = "BFGS");
}

Beta <- res$par ;
FinalObjectiveValue <- res$value ;
ComputeModelStatistics();

cat ( "OutConst 0 0", file = reg_output_file );
cat ( "\n" , file= reg_output_file, append=TRUE ); 

for ( i in c(1:num_reduced_indicators) )
{
	cat ( "OutCoeff", (indicators_included[i]-1), Beta[i], file = reg_output_file, sep= " ", append = TRUE ) ;
	cat ( "\n", file = reg_output_file, append = TRUE ) ;
}

cat ( "ErrorFunction ",FinalObjectiveValue,"\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "RSquared ", r_squared, "\n", file = reg_output_file, sep = " ", append =TRUE);
cat ( "Correlation ", final_correlation ,"\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "StdevDependent ", sd_dep, "\n", file = reg_output_file, sep = " ", append=TRUE );
cat ( "StdevResidual ", sd_residual, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "StdevModel ", sd_model, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "MSE ", final_mse , file = reg_output_file, sep = " ", append = TRUE );
