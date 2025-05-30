#!/usr/bin/env Rscript

# A brief description of this project was :
# Since the first step of model-building in genstrats is to see by l2-norm or regression what weights to assign to diffpricetype of different prices ( if they are in the ilist )
# Here we are doing that once for the product.
# We are trying to look at the data of just the product and find omix weights = the best weighted combination of the different prices such that no price after that has a significant positive correlation with the future price change of this omix price.
# 
#​Future work:
# Since regression will only summarize the data we present to it, the following are valid avenues of work :
#​- figure out the right duration to predict for each product
# - use a filter like isolating bunches of 10 minute periods where the number of trades of the product is high and hence these are the periods we should focus on in modeling.
# - < anything else > ?
# 
# Logic :

.libPaths("/apps/R/root/library/")
suppressPackageStartupMessages( require('numDeriv') );

#solution used from http://stats.stackexchange.com/questions/13740/finding-weights-that-remove-residual-dot-product

args = commandArgs( trailingOnly=TRUE );
if ( length(args) < 2 ) 
{        
    stop ( paste("USAGE : <script> <timed_data> <dur_msecs_>",
        "Note: for timed_data, it expects:",
        "\tbasepx and futpx as the baseprice for the GeneralPrice indicator",
        "\tthe indicators as possible candidates for the bias for the GeneralPrice", sep="\n") );
}
 
data<-read.table(args[1]);
data<-as.matrix(data);

dur_msecs_ <- as.numeric(args[2]);

num_data_ <- dim(data)[1];
midpx_idx_ <- 4;
ninds <- ncol(data) - 4;

rows_buf <- floor(5 * dur_msecs_ / mean(diff(data[,1])));

curr_idc_ <- 1:(num_data_-rows_buf);
adv_idc_ <- sapply(curr_idc_, function(x) (x + min(which(data[(x+1):(x+rows_buf),1] - data[x,1] > dur_msecs_))));
val_indc_ <- is.finite(adv_idc_);
curr_idc_ <- curr_idc_[val_indc_];
adv_idc_ <- adv_idc_[val_indc_];
#cat("rows reduced from",num_data_," to", length(curr_idc_), "\n");

for (indc_ in 1:ninds) {
  pxcols <- c(midpx_idx_, midpx_idx_ + indc_);
  pxs <- data[,pxcols];
  pxs[,2] <- pxs[,1] + pxs[,2];

  U <- pxs[curr_idc_,];
  V <- pxs[adv_idc_,] - U;

  cat("\n");
  cat("Indc ", indc_, ": Predictive_Corr with selfprice_change: ", cor(U[,2]-U[,1],V[,2]),"\n", sep="");
  cat("Indc ", indc_, ": Predictive_Corr with baseprice_change: ", cor(U[,2]-U[,1],V[,1]),"\n", sep="");

  x0 <- solve( t(U) %*% V ) %*% matrix(1,2);
  qx0 <- t(x0) %*% t(V) %*% U %*% x0;
  x <- x0/qx0[1];
  cat("Indc ", indc_, ": Bias Weight: ", x[2], "\n", sep="");
}
