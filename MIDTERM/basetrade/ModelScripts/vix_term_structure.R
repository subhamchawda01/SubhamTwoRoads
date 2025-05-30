#!/usr/bin/env Rscript
#suppressPackageStartupMessages( library ( "numDeriv" , lib.loc="/apps/R/root/library/" ) );
.libPaths("/apps/R/root/library/")
suppressPackageStartupMessages( require('numDeriv') );

#################

Prediction <- function ( W )
{
        v0 <- W[1];
        vl <- W[2];
        alpha <- W[3];
        v_hat <- seq(0,num_maturities-1,1);
        v_hat <- vl + ( v0 - vl ) * exp ( -alpha * v_hat );
        retval <- v_hat;
}


Objective <- function ( W )
{
        J = sum ( ( v_actual - Prediction ( W ) ) * ( v_actual - Prediction ( W ) ) );
        retval <- J;
}

GradNumerical <- function ( W )
{
        retVal <- grad (Objective,W,method="simple");        
}

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 1 ) {
        stop ("USAGE : <script> <data>\n");
}


datafile <- args[1];
data <- read.table(args[1]);
data <- as.matrix(data);

desired_size <- 700000000; # 700 MB
file_size <- file.info(datafile)$size;

downsample_ratio <- 1;

if ( file_size > desired_size )
{
	downsample_ratio <- ceiling ( file_size / desired_size );
	regdata <- regdata [ seq (1, nrow(regdata), by = downsample_ratio ), ]; 
}

num_maturities <- ncol(data);
num_instances <- nrow(data);

pred1_data <- data;
pred2_data <- data;

v_actual <- data[1,];

Weights <- matrix ( 0, num_instances, 3);

for ( i in seq(1,num_instances) )
{
        W <- c(1,1,1);
        v_actual <- data[i,];
        res <- optim(W, Objective, GradNumerical, method = "BFGS");
        W_o <- res$par;
        Weights[i,] <- W_o;
        pred1_data[i,] <- Prediction ( W_o );        
}

for ( j in seq(1,num_maturities) )
{
        error <- sum(abs(data[,j] - pred1_data[,j])) / num_instances;
        cat("Using 1st method error in maturity ", j, " is ", error, "\n" );    
}

for ( j in seq (1,num_maturities) )
{
        Y <- data[,j];
        X <- data[,-j];
        lmodel <- lm ( Y ~ X + 0 );
        coeffs <- lmodel$coefficients;
        Y_hat <- X%*%coeffs;
        pred2_data[,j] = Y_hat;
        error <- sum(abs(Y-Y_hat)) / num_instances;
        cat ("Using 2nd method error in maturity ", j, " is ", error, "\n" );
}


save ( Weights, pred1_data, pred2_data, data, file="~/vix_R_data");
