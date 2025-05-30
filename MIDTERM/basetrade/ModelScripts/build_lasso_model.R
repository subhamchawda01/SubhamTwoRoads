#!/usr/bin/env Rscript
#suppressPackageStartupMessages( library(glmnet), lib.loc="/apps/R/R-2.15.2/library/" );
#.libPaths("/apps/R/R-2.15.2/library/")
suppressPackageStartupMessages( require(glmnet))
suppressPackageStartupMessages( require(methods))

# suppressPackageStartupMessages ( library(glmnet), lib.loc="/apps/R/root/library/" ) ;
#.libPaths("/apps/R/root/library/") ; # probably not needed after ~/.Renviron change
#suppressPackageStartupMessages ( require(glmnet) ) ; # probably not needed after ~/.Renviron change

args <- commandArgs(trailingOnly = TRUE);
data <- read.table(args[1]);
data <- as.matrix(data);
x <- data[,2:ncol(data)]; 
y <- data[,1];

remove(data);

stdevs <- apply(x,2,sd);
zero_stdevs <-  which (stdevs <=0 ) 
if (length ( zero_stdevs  ) > 0  )
{
    err <- paste ( "indices with zero stdevs : " , paste ( zero_stdevs, collapse="," ) );
#    tryCatch ({
#       full_cmd_str <- paste( "/home/dvctrader/basetrade_install/scripts/send_email.pl ", "To: nseall@tworoads.co.in __n__From: lasso@circulumvite.com __n__Subject: Indicators with zero stdev found __n____n__", err );
#       system ( full_cmd_str );
#     }, error = function ( e )
#     {})     
}
# compute correlation on this data
initial_corrs <- cor(y,x);

#

initial_corrs[is.na(initial_corrs)] <- 0
names(initial_corrs)=NULL;
#exclude those indicators which have sign different to hash correlations
means <- colMeans(x);


means <- abs(means);
sharpe_check <- means - 0.22 *stdevs; # 0.22 threshold for sharpe of an indicator
excl <- c();
if ( length(args) > 3)
{
	hash_corrs <- read.table(args[4]);
	hash_corrs <- hash_corrs[,1];
	corr_parity <- initial_corrs*hash_corrs;
	excl <- append( excl , which(corr_parity < 0 ));
}
inds_excluded_due_to_sharpe_check <- which(sharpe_check > 0);
excl <- append ( excl , inds_excluded_due_to_sharpe_check );
# alpha=0 signifies ridge penalty(l2 norm), alpha=1 signifies lasso (l1 norm), alpha in (0,1) signifies a hybrid (elastic net )
# using lasso penalty
flag <- TRUE;

while(flag==TRUE)
{
	fit <- cv.glmnet(x,y,family="gaussian",alpha=1,intercept=FALSE,exclude=excl); 
	df <- fit$glmnet.fit$df; 
	indx <- length(which(df<=as.numeric(args[2]))); 
	val_lambda <- fit$lambda[indx];

	#if a lambda value with lower number of indicators gives a better mse then take that value. Assumption here is that the
	#plot of the ift has only one minima

	if ( val_lambda < fit$lambda.min )
	{
		val_lambda <- fit$lambda.min; 
		indx <- which(fit$lambda==fit$lambda.min);
	}
		
	coeffs <- coef (fit,s=val_lambda) ;
	coeffs <- as(coeffs,"vector");
	coeffs <- coeffs[-1];
        corr_weight_sign <- coeffs*initial_corrs;
	excl <- append(excl,which (corr_weight_sign < 0 ));
	error <-  fit$cvm [indx] ;
	selected_indicators <- which(coeffs!=0) ;
	selected_indicators_weights <- coeffs[ which(coeffs!=0 ) ] ;
	if(length(which(corr_weight_sign < 0) ) <= 0)
	{
		flag = FALSE;
	}
}
# print classification error
print(error);
print(min(fit$cvm)) ;
ind_n_weights <- cbind (selected_indicators,selected_indicators_weights) ;
write.table(ind_n_weights, args[3],col.names=F,row.names=F);
