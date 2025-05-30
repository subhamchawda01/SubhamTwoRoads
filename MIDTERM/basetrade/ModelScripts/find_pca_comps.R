#!/usr/bin/env Rscript
args <- commandArgs(trailingOnly = TRUE);
data <- read.table(args[1]);
data <- as.matrix(data);
tgt <- data[,1];
data <- data[,2:ncol(data)]

means <- colMeans(data);
absmeans <- abs(means);
stdevs <- apply(data,2,sd);

sharpe_check <- absmeans - 0.22*stdevs;

newdata <- cbind()
in_included <- cbind()

count_removed <- 0;
#remove indicators with significant non-zero bias
for ( i in cbind(1:ncol(data)) )
{
  if ( sharpe_check[i] < 0 && stdevs[i] > 0.00001 )
    {
      in_included <- cbind(in_included,1);
      newdata <- cbind(newdata,data[,i]);	
    }
  else
    {
      count_removed <- count_removed + 1;
      in_included <- cbind(in_included,0);
    }
}

stdevs <- apply(newdata,2,sd);

pca <- princomp(newdata); # get pca components
pca_comps <- with(pca,unclass(loadings));
sdevs <- pca$sdev;
pca_comps_final <- cbind();
j <- 1; 
for ( i in cbind(1:ncol(data)) )
{
  if(in_included[i]==0)
    {
      pca_comps_final <- rbind(pca_comps_final,t(rep(0,ncol(pca_comps))));
    }
  else
    {
      pca_comps_final <- rbind(pca_comps_final,pca_comps[j,]);
      j <- j + 1;
    }
}

remove(pca_comps);

for ( i in cbind(1:count_removed) )
{
  pca_comps_final <- cbind ( pca_comps_final,rep(0,nrow(pca_comps_final)) );
}

remove(newdata);
tr_regdata <- data %*% pca_comps_final; # matrix multiplication
remove(data);
tr_regdata <- cbind(tgt,tr_regdata);
write(tr_regdata,args[2],ncolumns=ncol(tr_regdata) );
write(pca_comps_final,args[3],ncolumns=ncol(pca_comps_final));
