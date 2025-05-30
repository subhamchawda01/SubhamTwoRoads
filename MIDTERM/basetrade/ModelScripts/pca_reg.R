#!/usr/bin/env Rscript

args <- commandArgs(trailingOnly = TRUE);
data <- read.table(args[1]);
data <- as.matrix(data);
tgt <- data[,1];
data <- data[,2:ncol(data)]

nc <- as.numeric(args[2]); # nc - num_components
max_model_size <- as.numeric(args[3]);
method <- as.numeric(args[4]); 
means <- colMeans(data);
means <- abs(means);
stdevs <- apply(data,2,sd);

sharpe_check <- means - 0.22*stdevs;

newdata <- cbind()
indicators_included <- cbind()

#remove indicators with significant non-zero bias
for ( i in cbind(1:ncol(data)) )
{
	if(sharpe_check[i] < 0 )
	{
		indicators_included <- cbind(indicators_included,i);
		newdata <- cbind(newdata,data[,i]);	
	}
}

stdevs <- apply(newdata,2,sd);

for ( i in cbind(1:ncol(newdata)) )
{
	if(stdevs[i]==0)
	{
		newdata[,i] <- newdata[,i]*0;
	} else
	{
		newdata[,i] <- newdata[,i]*(1/stdevs[i]);
	}
}

pca <- princomp(newdata); # get pca components
pca_comps <- with(pca,unclass(loadings));
pca_comps_reduced <- pca_comps[,1:nc];
sdevs <- pca$sdev;
sdevs <- sdevs[1:nc];
n <- nrow(pca_comps_reduced);

num_ind_vec <- rep(0,nc);

nifc <-floor(max_model_size/nc + (nc-1)/2)

if ( method < 0 )
{
	for( i in cbind(1:nc) )
	{
		num_ind_vec[i] <- nifc - i;
	}		
} else
{
	for ( i in cbind(1:nc) )	
	{
		num_ind_vec[i] <-floor(sdevs[i]*max_model_size/sum(sdevs))-1;
	}
}

for ( i in cbind(1:ncol(pca_comps_reduced)) )
{
	comp <- pca_comps_reduced[,i];
	thresh <- sort( abs(comp),partial=(n-num_ind_vec[i]))[n-num_ind_vec[i]];
	comp[abs(comp)< thresh] <- 0;
	pca_comps_reduced[,i] <- comp;
}

for ( i in cbind(1:nrow(pca_comps_reduced)) )
{
	pca_comps_reduced[i,] <- pca_comps_reduced[i,] * stdevs[i];
}

tr_regdata <- newdata %*% pca_comps_reduced; # matrix multiplication
tr_regdata <- cbind(tgt,tr_regdata);
pca_indices_weights <- cbind(t(indicators_included),pca_comps_reduced);
write.table(tr_regdata,args[5]);
write.table(pca_indices_weights,args[6]);
