#!/usr/bin/env Rscript
#suppressPackageStartupMessages( library ( "numDeriv" , lib.loc="/apps/R/root/library/" ) );
.libPaths("/apps/R/root/library/")
suppressPackageStartupMessages( require('numDeriv') );

#################
#Algorithm
#y = Sum over i { Gi(Xi)  }
#Gi(X)=Bi * ( sigmoid ( Ai  * X ) - 0.5 )

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
	alpha_penalty <- AlphaPenalty ( B );

	alpha <- B[1:num_indicators];
	beta <- B[(num_indicators+1):(2*num_indicators)];
	T <- X_train * t(replicate(nrow(X_train),alpha));
	H <- 1/(1 + exp(-T)) - 0.5;
	Y_hat_train <- H %*% beta;

	regularization_penalty <- 0;

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
	
	retVal <- alpha_penalty + regularization_penalty + sum ( ( Y_train - Y_hat_train ) * ( Y_train - Y_hat_train ) ) / ( length(Y_hat_train) * 2 );

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

        alpha_penalty_grad <- AlphaPenaltyGradAnalytical ( B );
	
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
	
	retVal <- -c(t_grad_alpha,t_grad_beta)/length(Y_train) + regularization_penalty_grad + alpha_penalty_grad ;	
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

final_correlation <- 0;
final_mse <- 0;

r2_original <- 0;
r2_model <- 0;
r_squared <- 0;

sd_dep <- 0;
sd_model <- 0;
sd_residual <- 0;

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 3 ) {
        stop ("USAGE : <script> <regdata> <reg_output_file> <penalty[N,L0,LP,L1,L2]> <optional: p in (0,1) for LP> [CVtest][max_allowed_error][statusfile]\n N = no regularization \n L0 = penalty on high L0 norm of coefficients i.e, number of non_zero indicators \n L1 = penalty on high L1 norm of coefficients \n L2 = penalty on high L2 norm of coefficients \n");
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
regularization_method <- args[3];

desired_size <- 700000000; # 700 MB
file_size <- file.info(regdata_file)$size;

downsample_ratio <- 1;

if ( file_size > desired_size )
{
	downsample_ratio <- ceiling ( file_size / desired_size );
	regdata <- regdata [ seq (1, nrow(regdata), by = downsample_ratio ) ]; 
}

X <- regdata[,2:ncol(regdata)];
Y <- regdata[,1]

N <- dim(X)[1]

n_train <- floor(0.7*N);

n_test <- N - n_train;

X_train <- X[1:n_train,];
Y_train <- Y[1:n_train];

X_test <- X[(n_train+1):N,];
Y_test <- Y[(n_train+1):N];

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

if ( length(args) > 3 )
{
   p_norm = as.numeric(args[4]);
}


num_params <- num_indicators + num_indicators;

B <- rep (1, num_params );

lambda_alpha <- 0;
lambda_beta <- 0;

best_alpha <- rep(0,num_indicators);
best_beta <- rep(0,num_indicators);

best_objective <- 1000000;

{
    for ( lambda_alpha in alpha_steps )
    {
       for ( lambda_beta in beta_steps )
       {
  	  res <- optim(B, SIGLR, SIGLRGradAnalytical, method = "BFGS");
	  Beta <- res$par;
	  alpha <- Beta[1:num_indicators];
	  beta <- Beta[(num_indicators+1):(2*num_indicators)];
	  for ( i in c(1:num_indicators))
	  {
	     alpha[i] <- alpha[i]/sdx[i];
	  }   	
	  beta <- beta * sdy;
	  t_outsample_result <- ComputeTestStatistics (alpha, beta );
	  if ( best_objective > t_outsample_result )
	  {
	     best_alpha <<- alpha;
	     best_beta <<- beta;
	     best_objective <- t_outsample_result;	    
	  }
	  #cat ("Alpha Lambda :",lambda_alpha,"Beta Lambda :",lambda_beta,"Objective :",t_outsample_result,"\n",sep=" " );
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

r2_residual <<- sum ( ( Y_hat_train - Y_train ) ^ 2 );
r2_original <<- sum ( (Y_train - mean(Y_train))^ 2 );

r_squared <<- 1 - r2_residual / r2_original;

cat ( "OutConst 0 0", file = reg_output_file );
cat ( "\n" , file= reg_output_file, append=TRUE );

for ( i in c(1:num_indicators) )
{
        cat ( "OutCoeff", (i-1), alpha[i],beta[i], file = reg_output_file, sep= " ", append = TRUE );
        cat ( "\n", file = reg_output_file, append = TRUE ) ;
}

cat ( "RSquared ", r_squared, "\n", file = reg_output_file, sep = " ", append =TRUE);
cat ( "Correlation ", final_correlation ,"\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "StdevDependent ", sd_dep, "\n", file = reg_output_file, sep = " ", append=TRUE );
cat ( "StdevResidual ", sd_residual, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "StdevModel ", sd_model, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "MSE ", final_mse ,"\n", file = reg_output_file, sep = " ", append = TRUE );

if ( length(args) > 4)
{
	testregdata <- as.matrix(read.table(args[5]));
	X_test <- testregdata[,2:ncol(testregdata)];
	Y_test <- testregdata[,1];
	
        T <- X_test * t(replicate(nrow(X_test),best_alpha));
        H <- 1/(1 + exp(-T)) - 0.5;
        Ybar_test <- H %*% best_beta;

	error <- sum ( ( Ybar_test - Y_test) ^ 2)/sum( (Y_test - mean(Y_test))^2); 
        
	if (error < as.numeric(args[6])) 
        { 
                cat("1\n",file = args[7]); 
        } 
        else 
        { 
                cat("0\n",file = args[7]); 
        } 
	cat("Test error ",error,"\n",file = args[7],append=TRUE);
}
