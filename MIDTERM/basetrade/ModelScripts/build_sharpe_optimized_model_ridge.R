#!/usr/bin/env Rscript

suppressPackageStartupMessages ( require(glmnet) ) ; 
suppressPackageStartupMessages ( require(methods) ) ;

args <- commandArgs(trailingOnly = TRUE);
data <- read.table(args[1]);
data <- as.matrix(data);
x_ <- data[,2:ncol(data)]; 
y_ <- data[,1];

remove(data);

elem_wise_x_y_ = x_ * as.vector(y_);
ones_ = t(t(rep(1.0, length(y_))));

ridgefit_ <- cv.glmnet( elem_wise_x_y_,ones_, family="gaussian", alpha=0, intercept=FALSE ) ;

val_lambda <- ridgefit_$lambda.min; 
#indx <- which(ridgefit_$lambda==ridgefit_$lambda.min);
		
coeffs_ <- coef (ridgefit_,s=val_lambda) ;
coeffs_ <- as(coeffs_,"vector");
coeffs_ <- coeffs_[-1];
coeffs_[which(is.na(coeffs_))] <- 0;

coeffs_ = coeffs_ / sd(x_ %*% t(t(coeffs_)) ); # this makes stdev of x * coeffs_ to be 1
selected_indicators <- which(coeffs_!=0) ;
selected_indicators_weights <- coeffs_[ which(coeffs_!=0 ) ] ;
ind_n_weights_ <- cbind (selected_indicators,selected_indicators_weights) ;
write.table(ind_n_weights_, args[2],col.names=F,row.names=F);
