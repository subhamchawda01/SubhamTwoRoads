#!/usr/bin/env Rscript
suppressPackageStartupMessages(require(glmnet))
suppressPackageStartupMessages(require(methods))

args <- commandArgs(trailingOnly = TRUE);
data <- read.table(args[1]);
data <- as.matrix(data);
x <- data[,2:ncol(data)]; 
y <- data[,1];

lasso_or_ridge <- as.numeric(args[2]);

remove(data);

stdevs <- apply(x,2,sd);
# alpha=0 signifies ridge penalty(l2 norm), alpha=1 signifies lasso (l1 norm), alpha in (0,1) signifies a hybrid (elastic net )
# using lasso penalty
if ( lasso_or_ridge == 1 )
{
	fit <- cv.glmnet(x,y,family="gaussian",alpha=0,intercept=FALSE); 
} else 
{
        fit <- cv.glmnet(x,y,family="gaussian",alpha=1,intercept=FALSE);
}

val_lambda <- fit$lambda.min; 
indx <- which(fit$lambda==fit$lambda.min);
		
coeffs <- coef (fit,s=val_lambda) ;
coeffs <- as(coeffs,"vector");
coeffs <- coeffs[-1];
error <-  fit$cvm [indx] ;
selected_indicators <- which(coeffs!=0) ;
selected_indicators_weights <- coeffs[ which(coeffs!=0 ) ] ;
# print classification error
print(error);
print(min(fit$cvm)) ;
ind_n_weights <- cbind (selected_indicators,selected_indicators_weights) ;
write.table(ind_n_weights, args[3],col.names=F,row.names=F);
