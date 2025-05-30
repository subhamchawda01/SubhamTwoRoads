#!/usr/bin/env Rscript
#suppressPackageStartupMessages( library ( "numDeriv" , lib.loc="/apps/R/root/library/" ) );
.libPaths("/apps/R/root/library/")
suppressPackageStartupMessages( require('numDeriv') );
suppressPackageStartupMessages( require('nloptr') );
suppressPackageStartupMessages( require('MASS') );

Objective <- function ( alpha )
{
        maturities <- seq(0,num_maturities-1,1);

        exp_maturities <- exp ( - alpha * maturities );

        A <- sum ( exp_maturities );
        B <- sum ( exp ( - 2 * alpha * maturities ) );
        C <- rowSums ( data );
        E <- matrix ( exp_maturities, num_instances, num_maturities, byrow=TRUE);
        D <- rowSums ( data * E );

        AB1 <- c( B, A-B );
        AB2 <- c( A-B, num_maturities + B - 2*A );

        cat ( "alpha ", alpha, "\n" );
        cat ( "AB1 ", AB1, "\n" );
        cat ( "AB2 ", AB2, "\n" );

        AB <- rbind ( AB1, AB2 );
       
        ABinv <- ginv(AB);

        C1 <- D;
        C2 <- C - D;

        Cmat <- rbind ( C1, C2 );

        V <- ABinv %*% Cmat;

        V0 <- V[1,];
        Vl <- V[2,];

        pred_data <<- matrix ( Vl, num_instances, num_maturities, byrow=TRUE )  + ( V0 - Vl ) %*% t ( exp_maturities );

        J <- sum ( ( pred_data - data ) * ( pred_data - data ) ) / num_instances;
}


GradNumerical <- function ( alpha )
{
        retVal <- grad (Objective,alpha,method="simple");        
}

EqualityConstraint <- function ( B ) {
   retval <- B - 1;
}

EqualityConstraintGrad <- function ( B) {
   retval <- 1;
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

pred_data <- data;
pred2_data <- data;

v_actual <- data[1,];

Weights <- matrix ( 0, num_instances, 3);

alpha <- 0;

lb_constVec <- 0;
ub_constVec <- 100;

opts <- list("algorithm"="NLOPT_LD_SLSQP");

res <- suppressWarnings ( nloptr(x0=alpha, eval_f=Objective, eval_grad_f=GradNumerical, lb = lb_constVec, ub = ub_constVec, eval_g_eq = EqualityConstraint, eval_jac_g_eq = EqualityConstraintGrad, opts=opts) );
res<- rle(unlist(res[18]))$values;


res <- optim(alpha, Objective, GradNumerical, method = "L-BFGS-B", lower = 0.00, upper = 100.00 );
o_alpha <- res$par;

cat ( "Error for 1st method is ", Objective ( o_alpha ),"\n");

for ( j in seq(1,num_maturities) )
{
        error <- sum(abs(data[,j] - pred_data[,j])) / num_instances;
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
        error <- sum((Y-Y_hat)*(Y-Y_hat)) / num_instances;
        cat ("Using 2nd method error in maturity ", j, " is ", error, "\n" );
}


save ( pred_data, pred2_data, data, file="~/vix_R_data");
