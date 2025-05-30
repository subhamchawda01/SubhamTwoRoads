#!/usr/bin/env Rscript

# suppressPackageStartupMessages ( library(glmnet), lib.loc="/apps/R/root/library/" ) ;
.libPaths("/apps/R/root/library/") ; # probably not needed after ~/.Renviron change
suppressPackageStartupMessages ( require(glmnet) ) ;
suppressPackageStartupMessages ( require(methods) ) ; 

#require(lattice,quietly=TRUE);
#require(methods,quietly=TRUE);
#require(Matrix,quietly=TRUE);
#require(glmnet,quietly=TRUE);
#msg.trap <- capture.output( suppressMessages( library(glmnet) ) ) ;

args <- commandArgs(trailingOnly = TRUE);
print(args);
raw_data <- read.table(args[1]);
raw_data <- as.matrix(raw_data);
tgt <- raw_data[,1];  # dependent
data<- read.table(args[2]);
data<- as.matrix(data); 
x <- raw_data[,2:ncol(raw_data)]; 
y <- data[,1];

stdevs <- apply(x,2,sd);
zero_stdevs <- which (stdevs <=0 ) 
if (length ( zero_stdevs  ) > 0 )
{
    err <- paste ( "indices with zero stdevs : " , paste ( zero_stdevs, collapse="," ) );
#    tryCatch ({
#       full_cmd_str <- paste( "/home/dvctrader/basetrade_install/scripts/send_email.pl ", "To: nseall@tworoads.co.in __n__From: mlogit@circulumvite.com __n__Subject: Indicators with zero stdev found __n____n__", err );
#       system ( full_cmd_str );
#     }, error = function ( e )
#     {})
}
initial_corrs <- cor(tgt,x);


# log 
#

initial_corrs <- cor(tgt,x);
initial_corrs[is.na(initial_corrs)] <- 0

# alpha=0 signifies ridge penalty(l2 norm), alpha=1 signifies lasso (l1 norm), alpha in (0,1) signifies a hybrid
# currently using ridge penalty
flag <- TRUE;
excl <-c();

while(flag==TRUE)
{
	fit <- cv.glmnet(x,y,nfolds=3,family="multinomial",type ="class",alpha=1,exclude=excl,dfmax=as.numeric(args[3])); 
	df <- fit$glmnet.fit[4]$df; 
	indx <- length(which(df<=as.numeric(args[3]))); 
	val_lambda <- fit$lambda[indx];
	coeffs <- coef(fit,s=val_lambda);

	label0_weights <- as(coeffs$`0`,"matrix");
	label1_weights <- as(coeffs$`1`,"matrix");
	label2_weights <- as(coeffs$`2`,"matrix");

	corr_weight_sign_nve <- label0_weights[-1]*initial_corrs;
	corr_weight_sign_pve <- label2_weights[-1]*initial_corrs;

#sanity check for sign of correlation and weights, and exclude that indicator for the next run
	if( length ( which( corr_weight_sign_nve > 0 ) > 0 )  |  length ( which( corr_weight_sign_pve < 0 ) > 0 )  )
	{
		excl <- append(excl,which (corr_weight_sign_nve > 0 ));	
		excl <- append(excl,which (corr_weight_sign_pve < 0 ));	
		next;	
	}
	else
	{
		flag <- FALSE;
	}
}

# compute class probabilities 
probs <- predict(fit,x,type="response",s=val_lambda)
probs <- probs[,,1];
# print correlations with each label's probability vector
print(cor(probs[,1],tgt));
print(cor(probs[,2],tgt));
print(cor(probs[,3],tgt));
coeffs <- coef(fit,s=val_lambda);
error <-  fit$cvm[indx];
# print classification error
print(error);
print(min(fit$cvm));
weights <- cbind(label1_weights,label2_weights);
weights <- cbind(label0_weights,weights); 
write.table(weights,args[4]);
