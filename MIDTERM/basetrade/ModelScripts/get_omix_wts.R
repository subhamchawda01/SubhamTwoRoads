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
	stop ("USAGE : <script> <timed_data> <dur_msecs_> \n");
}
 
data<-read.table(args[1]);
data<-as.matrix(data);

dur_msecs_ <- as.numeric(args[2]);
curr_idx_ <- 1;
num_data_ <- dim(data)[1];
num_pxs_ <- dim(data)[2]-2;
#adv_data_ <- c();
U <- c();
V <- c();

while(curr_idx_ < num_data_)
{
    for(adv_idx_ in (curr_idx_+1):num_data_)
    {
		if(data[adv_idx_,1] >= data[curr_idx_,1] + dur_msecs_)
		{
		    #adv_data_ <- rbind(adv_data_, c(data[curr_idx_,], data[adv_idx_,]));
		    U <- rbind(U, c(data[curr_idx_,3:(num_pxs_+2)]));
		    V <- rbind(V, c(data[adv_idx_,3:(num_pxs_+2)] - data[curr_idx_,3:(num_pxs_+2)]));
		    break;
		}
		else if(data[adv_idx_,1] <= data[curr_idx_,1])
		{
		    curr_idx_=adv_idx_-1;
		    break;
		}
    }
    curr_idx_ <- curr_idx_+1;
}

#U<-adv_data_[,3:6];
#V<-(adv_data_[,10:13] - U);
x0 <- solve( t(U) %*% V ) %*% matrix(1,num_pxs_);
qx0 <- t(x0) %*% t(V) %*% U %*% x0;
x <- x0/qx0[1];

print (x);
