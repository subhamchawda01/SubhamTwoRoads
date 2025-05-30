#!/usr/bin/env Rscript

suppressPackageStartupMessages( require(glmnet))
suppressPackageStartupMessages( require(methods))
options(warn=-1)
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

qr.X <- qr(x, tol=1e-9, LAPACK = FALSE)
cols_sel <- qr.X$pivot[seq_len(qr.X$rank)]

x <- as.matrix(x[,cols_sel]);
stdevs <- apply(x,2,sd);
cat ( stdevs, "\n" );

x <- sweep(x,2,stdevs,FUN='/');

df <- as.numeric(args[2]);

var_explained_mincap <- as.numeric(args[3]);

max_indicators <- min( 20, 1.5 * df ); # changed by aashay, @hrishav : look at this

if ( max_indicators < ncol(x) ) {
  selected_indicators_lasso <<- c();
  ReduceIndicatorsLasso ();
  x <- as.matrix(x[,selected_indicators_lasso]);
  stdevs <- stdevs[selected_indicators_lasso];
  cols_sel <- cols_sel[selected_indicators_lasso];
}

df <- min( ncol(x), df );

remove(data);

xtx <- t(x) %*% x;
eig <- eigen(xtx);
eig_values <- eig$values;

pc_var_explained <- cumsum(eig_values) / sum(eig_values);

df <- max(df, which( pc_var_explained >= var_explained_mincap )[1])
df <- min( ncol(x), df );

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
val_lambda <- res1$par[1];

coeffs_norm <- solve(xtx + val_lambda*diag(ncol(x))) %*% t(x) %*% y;
coeffs <- coeffs_norm / stdevs;

selected_indicators_weights <- coeffs[ which(coeffs!=0 ) ] ;
selected_indicators <- cols_sel[ which(coeffs!=0) ];
selected_indicators_correlations <- cor( y, x[, which(coeffs!=0) ] );

ind_n_weights <- cbind (selected_indicators-1,selected_indicators_weights) ;
#write.table(ind_n_weights, args[4],col.names=F,row.names=F);

pred_data <- as.matrix ( x  ) %*% as.matrix ( coeffs_norm ) ;

r2_original <- sum ( y ^ 2 ) ;
r2_residual <- sum ( (y - pred_data) ^ 2 ) ;

r_squared <- ( r2_original - r2_residual ) / r2_original;

cor_model <- cor ( y, pred_data );
y<-as.matrix(y);
sd_dep <- apply(y,2,sd) ;
sd_residual <- apply(y-pred_data,2,sd);
sd_model <- apply(pred_data,2,sd) ;
mse <- mean ( (y-pred_data)^2 );
abs_err <- mean ( abs ( y-pred_data ) ) ;

sink ( args[4] );
for ( i in 1:length(selected_indicators) ) { cat("OutCoeff", ind_n_weights[i,], "InitCorrelation", selected_indicators_correlations[i], "\n", sep=" "); }
cat ("RSquared "); cat (r_squared); cat ("\n");
cat ("Correlation "); cat (cor_model); cat ("\n");
cat ("StdevDependent "); cat (sd_dep); cat ("\n");
cat ("StdevResidual "); cat (sd_residual); cat ("\n");
cat ("StdevModel "); cat (sd_model); cat ("\n");
cat ("MSE "); cat (mse); cat ("\n");
cat ("MeanAbsError "); cat (abs_err); cat ("\n");

