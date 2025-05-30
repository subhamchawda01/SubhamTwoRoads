#!/usr/bin/env Rscript
#suppressPackageStartupMessages( library ( "numDeriv" , lib.loc="/apps/R/root/library/" ) );
.libPaths("/apps/R/root/library/")
suppressPackageStartupMessages( require('numDeriv') );

#################
#Algorithm
#to be written

#Reduces the ilist to "max_model_size" using lasso
ReduceIndicators <- function ()
{
	system (sprintf('~/basetrade/ModelScripts/call_lasso_mult.pl %s %s %s %s 1>/dev/null ', args[1], num_pred_dur, max_model_size, tmp_reg_output_file ))
        regout_text <- readLines (tmp_reg_output_file);
        for ( i in c(1:length(regout_text)) )
        {
            t_regout_row <- unlist(strsplit(regout_text[i], "\\ "));
            indicators_included <<- c( indicators_included, as.numeric( t_regout_row[1] ) );
        }	
	X <<- X[,indicators_included];
}


MULTLR <- function ( B ) {

	w <- B[1:num_indicators];
	#print("Weights : ");
	#print(w);
	H <- X_train;
	Y_hat_train <- H %*% w;
        tSum <- 0;
        for ( i in c(1:num_pred_dur) )
        {
            tSum <- tSum + sum ( ( Y_train[i] - Y_hat_train ) * ( Y_train[i] - Y_hat_train ) ) / ( length(Y_hat_train) * 2 ); 
        }
	retVal <- tSum;
	#print ("Error : ");
	#print (retVal);
}

#Compute Test Statisitics
ComputeTestStatistics <- function ( weights ) {
	w <- weights[1:num_indicators];
	H <- X_test;
	Y_hat_test <- H %*% w;
	tSum <- 0;
	for ( i in c(1:num_pred_dur) )
        {
            tSum <- tSum + sum ( ( Y_test[i] - Y_hat_test ) * ( Y_test[i] - Y_hat_test ) ) / ( length(Y_hat_test) * 2 ); 
        }
	retVal <- tSum;
}

#Calculates the gradient using inbuilt R function.
MULTLRGradNumerical <- function ( B ) {
	retVal <- grad (MULTLR,B,method="simple");
}

#Calculates the gradient using our own function.
MULTLRGradAnalytical <- function ( B ) {
	w <- B[1:num_indicators];
	H <- X_train;
	Y_hat_train <- H %*% w;
	Y_hat_train <- Y_hat_train[,rep(1,num_pred_dur)];
	temp_Y <- (Y_train - Y_hat_train)/ length(Y_hat_train);
	H <- t(H);
	temp_retVal <- H %*% temp_Y;
	#print(temp_retVal);
	retVal <- -rowSums ( temp_retVal );
	#print("Grads : ");
	#print(retVal);
	
}



args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 3 ) {
        stop ("USAGE : <script> <regdata> <reg_output_file> <num_dependents> <max_model_size> <target_stdev>\n");
}


regdata_file <- args[1];
regdata <- read.table(args[1]);
regdata <- as.matrix(regdata);
reg_output_file <- args[2];
tmp_reg_output_file <- paste (reg_output_file,"tmp",sep="_");
max_model_size <- as.numeric(args[4]);
target_stdev <- as.numeric(args[5]);

desired_size <- 700000000; # 700 MB
file_size <- file.info(regdata_file)$size;

downsample_ratio <- 1;

if ( file_size > desired_size )
{
	downsample_ratio <- ceiling ( file_size / desired_size );
	regdata <- regdata [ seq (1, nrow(regdata), by = downsample_ratio ), ]; 
}
num_pred_dur <- as.numeric(args[3]);
num_pred_dur_plus1 <- num_pred_dur + 1;

X <- regdata[,num_pred_dur_plus1:ncol(regdata)];
Y <- regdata[,1:num_pred_dur];

indicators_included <- c();
ReduceIndicators();

N <- dim(X)[1]

n_train <- floor(0.7*N);

n_test <- N - n_train;

X_train <- X[1:n_train,];
Y_train <- Y[1:n_train,];

X_test <- X[(n_train+1):N,];
Y_test <- Y[(n_train+1):N,];


num_indicators <- dim(X)[2];
sdx <- rep(0,num_indicators);

for ( i in c(1:num_indicators))
{
   sdx[i] <- sd ( X_train[,i] );
   X_train[,i] <- X_train[,i]/sdx[i];
}

sdy <- rep(0,num_pred_dur);
for ( i in c(1:num_pred_dur))
{
   sdy[i] <- sd ( Y_train[,i] );
   Y_train[,i] <- Y_train[,i]/sdy[i];
}

B <- rep (1, num_indicators );

best_w <- rep(0,num_indicators);

res <- optim(B, MULTLR, MULTLRGradAnalytical, method = "BFGS");
weights <- res$par;
for ( i in c(1:num_indicators))
{
	weights[i] <- weights[i]/sdx[i];
}
for ( i in c(1:num_indicators))
{
   X_train[,i] <- X_train[,i] * sdx[i];
}

X_temp <- X %*% weights;
tgtnums <- rowSums(X_temp);
tgtnums <- as.matrix(tgtnums);
target_stdev <- target_stdev/sd(tgtnums);

for ( i in c(1:num_pred_dur))
{
   Y_train[,i] <- Y_train[,i] * sdy[i];
}

for ( i in c(1:num_indicators))
{
        weights[i] <- weights[i] * target_stdev;
}


t_insample_result_new <- MULTLR ( weights );
t_outsample_result_new <- ComputeTestStatistics ( weights );
#cat ( "MSE for Training data: New ", t_insample_result_new, "\n", file= reg_output_file, append = TRUE );
#cat ( "MSE for Testing data : New ", t_outsample_result_new, "\n", file= reg_output_file, append = TRUE );
for ( i in c(1:num_indicators) )
{
        cat ( "OutCoeff", (indicators_included[i] -1), weights[i], file = reg_output_file, sep= " ", append = TRUE );
        cat ( "\n", file = reg_output_file, append = TRUE ) ;
}

#system ( sprintf("/home/dvctrader/place_coeffs_in_multlr_model.pl %s %s %s", final_model, orig_model, reg_output_file));
