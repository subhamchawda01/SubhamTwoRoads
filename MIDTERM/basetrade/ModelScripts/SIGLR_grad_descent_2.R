#!/usr/bin/env Rscript
#suppressPackageStartupMessages( library ( "numDeriv" , lib.loc="/apps/R/root/library/" ) );
.libPaths("/apps/R/root/library/")
suppressPackageStartupMessages( require('numDeriv') );

#################
#Algorithm
#y = Sum over i { Gi(Xi)  }
#Gi(X)=Bi * ( sigmoid ( Ai  * X ) - 0.5 )


#Reduces the ilist to "max_model_size" using lasso
ReduceIndicators <- function ()
{
	system (sprintf('~/basetrade/ModelScripts/call_slasso.pl %s %s %s 1>/dev/null ', args[1], max_model_size, tmp_reg_output_file ))
        regout_text <- readLines (tmp_reg_output_file);
        for ( i in c(1:length(regout_text)) )
        {
            t_regout_row <- unlist(strsplit(regout_text[i], "\\ "));
            indicators_included <<- c( indicators_included, as.numeric( t_regout_row[1] ) );
        }	
	X <<- X[,indicators_included];
}

#Computes the average no. of instances abs(indicator) exceeds the target bias
ComputeDominationMetric <- function ( B ) {
								#n = no. of observations. r = no. of indicators.
        alpha <- B[1:num_indicators];				#row vector of the parameters Ai of size r
        beta <- B[(num_indicators+1):(2*num_indicators)];      	#row vector of the parameters Bi of size r
        T <- X_train * t(replicate(nrow(X_train),alpha));	#T(i,j) = X_train(i,j)*alpha(j). T is an elementwise multiplication of X_train (n x r matrix) and Ai repicated n-times.
        H <- 1/(1 + exp(-T)) - 0.5;                             #H(i,j) = (1/(1+exp(-T(i,j))) - 0.5); H is an (n x r) matrix
	HH <- H * t(replicate(nrow(X_train),beta));		#HH(i,j) = H(i,j)*beta(j); HH is an (n x r) matrix; Contribution of jth indicator in the output of ith instance of Y_hat_train
        Y_hat_train <- H %*% beta;				#Y_hat_train(i,1) = sum(H(i,j)*beta(j)); Y_hat_train is an (n x 1) column matrix; Sum of contribution of all the indicators in Y_hat_train 
       	
        domination_metric <- sum ( ( abs(HH) - abs( replicate( num_indicators,as.vector( Y_hat_train ) ) ) ) > 0 );	#counts the no. of instances when contribution of one indicator exceeds the sum of contribution of all the indicators.
	retVal <- domination_metric / ( length(Y_hat_train) * num_indicators );
}

# penalty for indicators with different sign than original indicators
SignPenalty <- function ( B ) {
	t_large_penalty <- 1000; # a large number for penalizing strictly unwanted weights
	retVal <- 0;
        for ( i in c(1:num_indicators) )
        {
            if( B[ i + num_indicators ] * initial_corrs[i] < 0 )
            {
                 retVal <- retVal + ( B [ i + num_indicators ] * initial_corrs[i] ) * ( B [ i + num_indicators ] * initial_corrs[i] );
            }
        }
	retVal <- retVal * t_large_penalty;
}

AlphaPenalty <- function ( B ) {
	t_large_penalty <- 1000; # a large number for penalizing strictly unwanted weights
	retVal <- 0;
	for ( i in c(1:num_indicators ) )
	{
	    if ( B[i] < 0 )
	    {
		retVal <- retVal + B[i] * B[i] ;
	    }
	}
	retVal <- retVal * t_large_penalty;
}

DominationPenalty <- function ( B ) {
        domination_metric <- ComputeDominationMetric ( B );
        retVal <- lambda_domination * domination_metric;
}

DominationPenaltyGrad <- function ( B ) {
	retVal <- grad ( DominationPenalty,B,method="simple" );
}

SignPenaltyGrad <- function ( B ) { 
	retVal <- grad ( SignPenalty,B, method="simple");
}

SignPenaltyGradAnalytical <- function ( B ) {
        t_large_penalty <- 1000; # a large number for penalizing strictly unwanted weights
        grad_B_sign <- rep(0, 2*num_indicators);
        for ( i in c(1:num_indicators) )
        {
            if( B[ i + num_indicators ] * initial_corrs[i] < 0 )
            {
                 grad_B_sign[i+num_indicators] <- (2) * t_large_penalty * (B[ i + num_indicators ]) * (initial_corrs[i]) * (initial_corrs[i]) ;
            }
        }
        retVal <- grad_B_sign ;
}

AlphaPenaltyGrad <- function ( B ) {
	retVal <- grad ( AlphaPenalty, B, method="simple" );
}

AlphaPenaltyGradAnalytical <- function ( B ) {
        t_large_penalty <- 1000; # a large number for penalizing strictly unwanted weights
        grad_B_alpha <- rep(0, 2*num_indicators);
        for ( i in c(1:num_indicators ) )
        {
            if ( B[i] < 0 )
            {
                grad_B_alpha[i] <- (2) * t_large_penalty * B[i] ;
            }
        }
        retVal <- grad_B_alpha ;
}



L2Norm <- function ( B ) {
        retVal <- lambda_alpha * sum ( B [ 1 : num_indicators ] * B [ 1 : num_indicators ] ) + lambda_beta * sum (B[ ( num_indicators + 1) : (2 * num_indicators) ]*B[ ( num_indicators + 1 ) : ( 2 * num_indicators )]);
}

L1Norm <- function ( B ) {
        retVal <- lambda_alpha * sum ( abs ( B [ 1 : num_indicators ] ) ) + lambda_beta * sum ( abs ( B [ ( num_indicators + 1 ) : ( 2 * num_indicators ) ] ) );
}

LpNorm <- function ( B ) {
        retVal <- lambda_alpha * ( sum ( abs( B [ 1 : num_indicators ] ) ^ p_norm ) ) + lambda_beta * ( sum ( abs( B [ ( num_indicators + 1 ) : ( 2 * num_indicators ) ] ) ^ p_norm ) );
}

# not using L0
L0Norm <- function ( B ) {
	retVal <- lambda * ( sum ( which ( B !=0 ) ) );
}

#not using L0
L0NormGrad <- function ( B ) {
	retVal <- grad (L0Norm,B,method="simple");
}

LpNormGrad <- function ( B ) {
	retVal <- grad (LpNorm,B,method="simple");
}

L1NormGrad <- function ( B ) {
	retVal <- grad (L1Norm,B,method="simple");
}

L2NormGrad <- function ( B ) {
	retVal <- grad (L2Norm,B,method="simple");
}

SIGLR <- function ( B ) {
	sign_penalty <- SignPenalty ( B );
	alpha_penalty <- AlphaPenalty ( B );
#        sign_penalty <- 0;

	alpha <- B[1:num_indicators];
	beta <- B[(num_indicators+1):(2*num_indicators)];
	T <- X_train * t(replicate(nrow(X_train),alpha));
	H <- 1/(1 + exp(-T)) - 0.5;
	Y_hat_train <- H %*% beta;

	regularization_penalty <- 0;
	domination_penalty <- 0;

	if ( regularization_method == "L2" )
	{
	   regularization_penalty <- L2Norm(B);
	}
	if ( regularization_method == "L1" )
	{
	   regularization_penalty <- L1Norm(B);
	}
	if ( regularization_method == "LP" )
	{
	   regularization_penalty <- LpNorm(B);	
	}	
	if ( regularization_penalty == "L0" )
	{
	   regularization_penalty <- L0Norm(B);
	}
	if( domination_method == "Y" )
	{
	   domination_penalty <- DominationPenalty ( B );
	}	  	
	
	retVal <- domination_penalty + alpha_penalty + sign_penalty + regularization_penalty + sum ( ( Y_train - Y_hat_train ) * ( Y_train - Y_hat_train ) ) / ( length(Y_hat_train) * 2 );

}

#Calculates the gradient for the learning parameters Ai and Bi analytically.
SIGLRGradAnalytical <- function ( B ) {
        alpha <- B[1:num_indicators] ;
        beta <- B[(num_indicators+1):(2*num_indicators)] ;
        T <- X_train * t(replicate(nrow(X_train),alpha)) ;
        H <- 1/(1 + exp(-T)) ;
	HH <- H - 0.5 ;
        Y_hat_train <- HH %*% beta ;
	diff <- as.vector(Y_train - Y_hat_train) ;
	t_grad_alpha <- colSums(replicate(num_indicators,diff) * t(replicate (length(Y_train),beta)) * H * ( 1 - H ) * X_train );	# -d((y-y_hat).(y-y_hat))/d(alpha)
	t_grad_beta <- colSums(replicate(num_indicators,diff) * HH ) ;									# -d((y-y_hat).(y-y_hat)))/d(beta)
	
	regularization_penalty_grad <- 0;
	domination_penalty_grad <- 0;

#	sign_penalty_grad <- SignPenaltyGrad ( B );
# 	alpha_penalty_grad <- AlphaPenaltyGrad ( B );
        sign_penalty_grad <- SignPenaltyGradAnalytical ( B );
        alpha_penalty_grad <- AlphaPenaltyGradAnalytical ( B );
#	sign_penalty_grad <- 0;
	
	if ( regularization_method == "L2" )
	{
	   regularization_penalty_grad <- L2NormGrad ( B );
	}
	if ( regularization_method == "L1" )
	{
	   regularization_penalty_grad <- L1NormGrad ( B );
	}
	if ( regularization_method == "LP" )
	{
	   regularization_penalty_grad <- LpNormGrad ( B );
	}
	if ( regularization_method == "L0" )
	{
	   regularization_penalty_grad <- L0NormGrad ( B );
	}
	if ( domination_method == "Y" )
	{
	   domination_penalty_grad <- DominationPenaltyGrad ( B );
	}
	
	retVal <- -c(t_grad_alpha,t_grad_beta)/length(Y_train) + regularization_penalty_grad + domination_penalty_grad + sign_penalty_grad + alpha_penalty_grad ;	
}

#Calculates the gradient using inbuilt R function.
SIGLRGradNumerical <- function ( B ) {
	retVal <- grad (SIGLR,B,method="simple");
}

#Computes and compares the performance of model for different domination and regularization steps.
ComputeTestStatistics <- function ( alpha, beta ) {
        T <- X_test * t(replicate(nrow(X_test),alpha));
        H <- 1/(1 + exp(-T)) - 0.5;
        Y_hat_test <- H %*% beta;
	retVal <- sum ( ( Y_test - Y_hat_test ) * ( Y_test - Y_hat_test ) ) / ( length(Y_hat_test) * 2 );
}

alpha_steps <- c(0.0001, 0.001);
beta_steps <- c(0.001, 0.01);
domination_steps <- c(0.001,0.01);

final_correlation <- 0;
final_mse <- 0;

r2_original <- 0;
r2_model <- 0;
r_squared <- 0;

sd_dep <- 0;
sd_model <- 0;
sd_residual <- 0;

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 5 ) {
        stop ("USAGE : <script> <regdata> <reg_output_file> <max_model_size> <domination_method(Y,N)> <penalty[N,L0,LP,L1,L2]> <optional: p in (0,1) for LP>[CVTest][max_allowed_error][statusfile]\n N = no regularization \n L0 = penalty on high L0 norm of coefficients i.e, number of non_zero indicators \n L1 = penalty on high L1 norm of coefficients \n L2 = penalty on high L2 norm of coefficients \n");
}


regdata_file <- args[1];
regdata <- read.table(args[1]);
regdata <- as.matrix(regdata);
num_rows_ <- dim ( regdata ) [1] 
num_cols_ <- dim ( regdata ) [2];
regdata <- as.numeric ( regdata ); #when numbers are too larget, R loads them as string, need to convert to numeric
regdata <- matrix ( regdata, nrow=num_rows_, ncol=num_cols_ ) ;

reg_output_file <- args[2];
tmp_reg_output_file <- paste (reg_output_file,"tmp",sep="_");
max_model_size <- as.numeric(args[3]);
regularization_method <- args[5];
domination_method <- args[4];


if ( domination_method == "N" )
{
     domination_steps <- 0;
}

desired_size <- 700000000; # 700 MB
file_size <- file.info(regdata_file)$size;

downsample_ratio <- 1;

if ( file_size > desired_size )
{
	downsample_ratio <- ceiling ( file_size / desired_size );
	regdata <- regdata [ seq (1, nrow(regdata), by = downsample_ratio ), ]; 
}

X <- regdata[,2:ncol(regdata)];
Y <- regdata[,1]

indicators_included <- c();
ReduceIndicators();
N <- dim(X)[1]

n_train <- floor(0.7*N);

n_test <- N - n_train;

X_train <- X[1:n_train,];
Y_train <- Y[1:n_train];

X_test <- X[(n_train+1):N,];
Y_test <- Y[(n_train+1):N];


if ( regularization_method == "N" )
{
        X_train <- X;
        Y_train <- Y;
}

initial_corrs <- cor(Y_train,X_train);
initial_corrs[is.na(initial_corrs)] <- 0

num_indicators <- dim(X)[2];
sdx <- rep(0,num_indicators);

for ( i in c(1:num_indicators))
{
   sdx[i] <- sd ( X_train[,i] );
   X_train[,i] <- X_train[,i]/sdx[i];
}

sdy <- sd(Y_train);
Y_train <- Y_train / sdy;

p_norm <- 0.5;

if ( length(args) > 5 )
{
   p_norm = as.numeric(args[6]);
}


num_params <- num_indicators + num_indicators;

B <- rep (1, num_params );

lambda_alpha <- 0;
lambda_beta <- 0;
lambda_domination <- 0;

best_alpha <- rep(0,num_indicators);
best_beta <- rep(0,num_indicators);

best_objective <- 1000000;


if ( regularization_method == "N" ){
        
        res <- optim(B, SIGLR, SIGLRGradAnalytical, method = "BFGS" );
        Beta <- res$par;
        alpha <- Beta[1:num_indicators];
        beta <- Beta[(num_indicators+1):(2*num_indicators)];
        for ( i in c(1:num_indicators))
        {
            alpha[i] <- alpha[i]/sdx[i];
            if(abs(beta[i]) < 0.001)
            {
                beta[i] <- 0;
                alpha[i] <- 0;
            }
        }       
        beta <- beta * sdy;
	best_alpha <<- alpha;
	best_beta <<- beta;
}else {
        for ( lambda_alpha in alpha_steps )
        {
         for ( lambda_beta in beta_steps )
	  {
	    for ( lambda_domination in domination_steps )
	    {
		res <- optim(B, SIGLR, SIGLRGradAnalytical, method = "BFGS");
		Beta <- res$par;
		alpha <- Beta[1:num_indicators];
		beta <- Beta[(num_indicators+1):(2*num_indicators)];
		for ( i in c(1:num_indicators))
		{
	    	    alpha[i] <- alpha[i]/sdx[i];
	    	    if(abs(beta[i]) < 0.001)
	    	    {
        		beta[i] <- 0;
	        	alpha[i] <- 0;
	  	    }
		}   	
		beta <- beta * sdy;
		t_outsample_result <- ComputeTestStatistics (alpha, beta );
		if ( best_objective > t_outsample_result )
		{
		    best_alpha <<- alpha;
		    best_beta <<- beta;
		    best_objective <- t_outsample_result;	    
		}
		#cat ("Alpha Lambda :",lambda_alpha,"Beta Lambda :",lambda_beta,"DominationLambda :",lambda_domination,"Objective :",t_outsample_result,"\n",sep=" " );
	    }
          }
	}
}

for ( i in c(1:num_indicators))
{
    X_train[,i] <- X_train[,i] * sdx[i];
}

Y_train <- Y_train * sdy;

alpha <- best_alpha ;
beta <- best_beta ;

T <- X_train * t(replicate(nrow(X_train),alpha)) ;
H <- 1/(1 + exp(-T)) - 0.5;
HH <- H * t(replicate(nrow(X_train),beta));
Y_hat_train <- H %*% beta;

sd_dep <- sdy;
sd_model <- sd(as.vector(Y_hat_train));
sd_residual <- sd(as.vector( Y_train - Y_hat_train ));
final_correlation <- cor(Y_train,Y_hat_train);
final_mse <- sqrt(sum((Y_train - Y_hat_train) * (Y_train - Y_hat_train))/(length(Y_train)));

domination <- 0;
domination_metric <- sum ( ( abs(HH) - abs( replicate( num_indicators,as.vector( Y_hat_train ) ) ) ) > 0 )

domination <- domination / ( length ( Y_hat_train ) * num_indicators );

r2_residual <<- sum ( ( Y_hat_train - Y_train ) ^ 2 );
r2_original <<- sum ( (Y_train - mean(Y_train))^ 2 );

r_squared <<- 1 - r2_residual / r2_original;

cat ( "OutConst 0 0", file = reg_output_file );
cat ( "\n" , file= reg_output_file, append=TRUE );

for ( i in c(1:num_indicators) )
{
        cat ( "OutCoeff", (indicators_included[i] -1), alpha[i],beta[i], file = reg_output_file, sep= " ", append = TRUE );
        cat ( "\n", file = reg_output_file, append = TRUE ) ;
}

cat ( "RSquared ", r_squared, "\n", file = reg_output_file, sep = " ", append =TRUE);
cat ( "Correlation ", final_correlation ,"\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "StdevDependent ", sd_dep, "\n", file = reg_output_file, sep = " ", append=TRUE );
cat ( "StdevResidual ", sd_residual, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "StdevModel ", sd_model, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "MSE ", final_mse ,"\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "Domination ", domination , "\n", file =  reg_output_file, sep = " ", append = TRUE );

if ( length(args) > 6)
{
	testregdata <- as.matrix(read.table(args[7]));
	X_cv_test <- testregdata[,2:ncol(testregdata)];
	X_cv_test <- X_cv_test[,indicators_included];
	Y_cv_test <- testregdata[,1];
	T <- X_cv_test * t(replicate(nrow(X_cv_test),alpha));
        H <- 1/(1 + exp(-T)) - 0.5;
        Ybar_test <- H %*% beta;
	
	error <- sum ( ( Ybar_test - Y_cv_test) ^ 2)/sum( (Y_cv_test - mean(Y_cv_test))^2); 
        if (error < as.numeric(args[8])) 
        { 
                cat("1\n",file = args[9]); 
        } 
        else 
        { 
                cat("0\n",file = args[9]); 
        } 
	cat("Test error ",error,"\n",file = args[9],append=TRUE);
}


