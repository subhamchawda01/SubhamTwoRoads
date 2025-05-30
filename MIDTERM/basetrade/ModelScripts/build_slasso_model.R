#!/usr/bin/env Rscript
suppressPackageStartupMessages(require(glmnet))
suppressPackageStartupMessages(require(lars))
suppressPackageStartupMessages(require(methods))
options(warn=-1)

"stabilityselection" <- function(x,y,nbootstrap=100,nsteps=50,alpha=0.2,plotme=FALSE) {

	# x is the n*p design matrix, y the n*1 variable to predict

	dimx <- dim(x)
	n <- dimx[1]
	p <- dimx[2]
	halfsize <- as.integer(n/2)
	freq <- matrix(0,nsteps+1,p)
	
	for (i in seq(nbootstrap)) {
		# randomly reweight each variable
		xs <- t(t(x)*runif(p,alpha,1))
		
		# randomly split the sample in two sets
		perm <- sample(dimx[1])
		i1 <- perm[1:halfsize]
		i2 <- perm[(halfsize+1):n]
		
		# run the randomized lasso on each sample and check which variables are selected
		r <- lars(xs[i1,],y[i1],max.steps=nsteps,normalize=FALSE)
                r_coeffs <- abs(sign(coef.lars(r)));
                r_steps <- nrow(r_coeffs) - 1;
                r_coeffs <- rbind(r_coeffs, r_coeffs[rep(r_steps+1, nsteps-r_steps),]);
		freq <- freq + r_coeffs;
		
                r <- lars(xs[i2,],y[i2],max.steps=nsteps,normalize=FALSE)
                r_coeffs <- abs(sign(coef.lars(r)));
                r_steps <- nrow(r_coeffs) - 1;
                r_coeffs <- rbind(r_coeffs, r_coeffs[rep(r_steps+1, nsteps-r_steps),]);
		freq <- freq + r_coeffs;
	}
		
	# normalize frequency in [0,1]
	freq <- freq/(2*nbootstrap)
	
	if (plotme) {
		matplot(freq,type='l',xlab="LARS iteration",ylab="Frequency")
	}
	
	# the final stability score is the maximum frequency over the steps
	result <- apply(freq,2,max)
}

args <- commandArgs(trailingOnly = TRUE);
data <- read.table(args[1]);
data <- as.matrix(data);
x <- data[,2:ncol(data)]; 
y <- data[,1];
dimx <- dim(x);
n <- dimx[1];
p <- dimx[2];
print(n);
print(p);
remove(data);

stdevs <- apply(x,2,sd);
zero_stdevs <- which(stdevs <= 0); 
if (length(zero_stdevs) > 0) {
    err <- paste("indices with zero stdevs: " , paste(zero_stdevs, collapse=","));
}

z = length(zero_stdevs);
print(z);
hash_map <- matrix(0,1,p-z);
x_copy = matrix(0,n,p-z);

idx <- 1;
match_idx <- 1;
for (i in seq(p)) {
	if (match_idx <= z && i == zero_stdevs[match_idx]) {
		match_idx = match_idx + 1;
	}
	else {
		x_copy[,idx] = x[,i] / stdevs[i];
		hash_map[idx] = i;
		idx = idx + 1;
	}
}

print(hash_map);
y_copy <- y;

stdevy <- sd(y, na.rm = FALSE);

if (stdevy > 0) {
	y_copy <- y / stdevy;
}

stability_scores <- stabilityselection(x_copy,y_copy,nbootstrap=50,nsteps=10);
print(stability_scores);

# Strategy1: first try the first threshold, if none of the predictors pass, 
# use the second threshold, if still none of the predictors pass,
# use normal lasso
# too selective, reducing threshold is one option, the other 
# option is to use strategy2

stability_threshold_1 = 0.5;
stability_threshold_2 = 0.4;

#low_stability_features <- which(stability_scores <= stability_threshold_1);
#if (length(low_stability_features) == length(stability_scores)) {
#	low_stability_features <- which(stability_scores <= stability_threshold_2);
#}

# Strategy2: select top k features
excl <- c();

print(as.numeric(args[2]))
num_features_exclude = (p-z-min(as.numeric(args[2]),p-z))
if (num_features_exclude > 0) {
	low_stability_features = order(stability_scores)[1:num_features_exclude]
	print(low_stability_features)

	for (i in seq(length(low_stability_features))) {
		low_stability_features[i] = hash_map[low_stability_features[i]];
	}
	if (length(low_stability_features) < length(stability_scores)) {
		excl <- append(excl,low_stability_features);
	}
}

# compute correlation on this data
initial_corrs <- suppressWarnings(cor(y,x));

initial_corrs[is.na(initial_corrs)] <- 0
names(initial_corrs) = NULL;

# exclude those indicators which have sign different to hash correlations
means <- colMeans(x);
means <- abs(means);
sharpe_check <- means - 0.22 *stdevs; # 0.22 threshold for sharpe of an indicator
if (length(args) > 3) {
	hash_corrs <- read.table(args[4]);
	hash_corrs <- hash_corrs[,1];
	corr_parity <- initial_corrs*hash_corrs;
	excl <- append(excl,which(corr_parity < 0));
}
inds_excluded_due_to_sharpe_check <- which(sharpe_check > 0);
excl <- append(excl,inds_excluded_due_to_sharpe_check);

# alpha=0 signifies ridge penalty(l2 norm), alpha=1 signifies lasso (l1 norm), alpha in (0,1) signifies a hybrid (elastic net)
# using lasso penalty

flag <- TRUE;

while (flag == TRUE) {
	fit <- cv.glmnet(x,y,family="gaussian",alpha=1,intercept=FALSE,exclude=excl); 
	df <- fit$glmnet.fit$df; 
	indx <- length(which(df<=as.numeric(args[2]))); 
	val_lambda <- fit$lambda[indx];

	# if a lambda value with lower number of indicators gives a better mse then take that value. Assumption here is that the
	# plot of the ift has only one minima

	if (val_lambda < fit$lambda.min) {
		val_lambda <- fit$lambda.min; 
		indx <- which(fit$lambda==fit$lambda.min);
	}
		
	coeffs <- coef(fit,s=val_lambda) ;
	coeffs <- as(coeffs,"vector");
	coeffs <- coeffs[-1];
    corr_weight_sign <- coeffs*initial_corrs;
	excl <- append(excl,which (corr_weight_sign < 0));
	error <-  fit$cvm[indx] ;
	selected_indicators <- which(coeffs!=0) ;
	selected_indicators_weights <- coeffs[which(coeffs!=0)] ;
	if (length(which(corr_weight_sign < 0)) <= 0) {
		flag = FALSE;
	}
}

# print classification error
print(error);
print(min(fit$cvm));
ind_n_weights <- cbind(selected_indicators,selected_indicators_weights);
write.table(ind_n_weights,args[3],col.names=F,row.names=F);
