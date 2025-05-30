#!/usr/bin/env Rscript

.libPaths("/apps/R/root/library/") # probably not needed

if ( exists( "verbose" ) ) {
  library('numDeriv');
  library('nloptr');
} else {
  msg.trap <- capture.output ( suppressPackageStartupMessages ( require('numDeriv') ) ) ;
  msg.trap <- capture.output ( suppressPackageStartupMessages ( require('nloptr') ) ) ;
}


RewardToRisk <- function ( B ) {
   WPNL <- PNL %*% B ;
   VAR <- quantile(WPNL,(0:20)/20)[2]; # VAR at 95 %tile
   #VAR <- sd(WPNL); 
   retval <- -sum(WPNL)/( length(WPNL) * abs(VAR) );
}

#Calculates the gradient using inbuilt R function.
GradNumerical <- function ( B ) {
   retVal <- grad (RewardToRisk,B,method="simple");
}

EqualityConstraint <- function ( B ) {
   allones <- rep(1,num_strats);
   retval <- allones %*% B - 1;
}

EqualityConstraintGrad <- function ( B) {
   retval <- rep(1,num_strats);
}

#Computes and compares the performance of model for different domination and regularization steps.
args = commandArgs( trailingOnly=TRUE );
if ( length(args) < 1 ) {
   stop ("USAGE : <script> <space delimited pnls [days,strats]>\n");
}

pnl_file <- args[1];
PNL <- as.matrix(read.table(args[1]));

num_strats <- dim(PNL)[2];

B <- rep(1,num_strats) * ( 1 / num_strats );

best_objective <- 1000000;

constraintMatrix <- diag(num_strats);
allones <- rep(1,num_strats);
constraintMatrix <- rbind(allones, -allones, constraintMatrix );

constVec <- rep (0, num_strats);

opts <- list("algorithm"="NLOPT_LD_SLSQP");

res <- suppressWarnings ( nloptr(x0=B, eval_f=RewardToRisk, eval_grad_f=GradNumerical, lb = constVec, eval_g_eq = EqualityConstraint, eval_jac_g_eq = EqualityConstraintGrad, opts=opts) );
res<- rle(unlist(res[18]))$values;

WPNL <- PNL %*% res;
VAR <- ceiling(quantile(WPNL,(0:20)/20)[2]);

avgpnl <- ceiling(sum(WPNL)/length(WPNL));

cat ( "Weights ", res , "\n" );
cat ( "WeightedStats Pnl: ", avgpnl, " Var: " , VAR , "\n" );

for ( i in c(1:num_strats) )
{
    var <- quantile(PNL[,i],(0:20)/20)[2]    
    cat("InputStats " , i , " Pnl:  " , ceiling(sum(PNL[,i])/length(PNL[,i])), " Var: " , var,  "\n" );
}
