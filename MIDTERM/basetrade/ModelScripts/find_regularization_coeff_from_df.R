#!/usr/bin/env Rscript

suppressPackageStartupMessages(require(glmnet))
suppressPackageStartupMessages(require(methods))

ReduceIndicatorsLasso <- function ()
{
#  initial_corrs <- cor(y,x);
#  initial_corrs[is.na(initial_corrs)] <- 0;
#  flag <- TRUE;
#  excl <- c();

#  while(flag==TRUE)
  {
#    fit <- cv.glmnet(x,y,family="gaussian",alpha=1,intercept=FALSE,exclude=excl);
    fit <- cv.glmnet(x,y,family="gaussian",alpha=1,intercept=FALSE); # changed by aashay, hrishav please look into this
    df_lasso <- fit$glmnet.fit$df;
    indx <- length(which(df_lasso<=as.numeric(max_indicators)));
    val_lambda <- fit$lambda[indx];

    coeffs <- coef (fit,s=val_lambda) ;
    coeffs <- as(coeffs,"vector");
    coeffs <- coeffs[-1];
    selected_indicators_lasso <<- which(coeffs!=0);
#    corr_weight_sign <- coeffs*initial_corrs;
#    excl <- append(excl,which (corr_weight_sign < 0 ));
#    if(length(which(corr_weight_sign < 0) ) <= 0) { flag = FALSE; }
  }
}

args <- commandArgs(trailingOnly = TRUE);
data <- read.table(args[1]);
data <- as.matrix(data);

y <- data[,1];

x <- as.matrix(data[,2:ncol(data)]);
stdevs <- apply(x,2,sd);

cols_sel <- which(stdevs!=0);
x <- as.matrix(x[,cols_sel]);
stdevs <- stdevs[cols_sel];
cat ( stdevs, "\n" );

x <- sweep(x,2,stdevs,FUN='/');

df <- as.numeric(args[2]);

var_explained_mincap <- as.numeric(args[3]);

max_indicators <- min( 25, 2 * df );

if ( length(args) > 3 ) {
  max_indicators <- as.numeric(args[4]);
}

if ( max_indicators < ncol(x) ) {
  selected_indicators_lasso <<- c();
  ReduceIndicatorsLasso ();
  x <- x[,selected_indicators_lasso];
  stdevs <- stdevs[selected_indicators_lasso];
  cols_sel <- cols_sel[selected_indicators_lasso];
}

df <- min( ncol(x), df );

remove(data);

xtx <- t(x) %*% x;
eig <- eigen(xtx);
eig_values <- eig$values;

pc_var_explained <- cumsum(eig_values) / sum(eig_values);

df <- max(df, which( pc_var_explained > var_explained_mincap )[1])

df_eigen_lambda_func <- function ( df, lambda ) {
  ret_val <- ( df - sum( eig_values / (eig_values + lambda) ) )^2;
  return (ret_val);
}

df_eigen_lambda_differential_func <- function ( df, lambda ) {
  ret_val <- 2 * ( df - sum( eig_values / (eig_values + lambda) ) ) * sum ( eig_values / (eig_values + lambda)^2 );
  return ( ret_val);
}

lambda_init <- mean( eig_values );

res1 <- optim( lambda_init, function(x) df_eigen_lambda_func(df, x), function(x) df_eigen_lambda_differential_func(df, x), method="L-BFGS-B", lower=0 );
val_lambda <- res1$par[1] / nrow(x);

cat ( "lambda: ", val_lambda, "\n", sep="" );

