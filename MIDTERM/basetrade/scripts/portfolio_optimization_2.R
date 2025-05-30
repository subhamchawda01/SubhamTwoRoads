#! /usr/bin/env Rscript

# R script to compute max-sharpe portfolio
# Input a filename which has a matrix where a row represents returns of a strat

args = commandArgs( trailingOnly=TRUE )
print(args)

library(quadprog)
#first step read the input data
ret_data = read.table( args[1] )
ret_data = t(ret_data)
size = dim ( ret_data ) #size[1]=no_of_days size[2]=no of strats or assets

D = cov(ret_data);
d = array(0, dim = c(size[2],1));
A = array(0, dim = c(size[2]+1,size[2]));
for ( i in 1:size[2] )
	A[1,i] = mean(ret_data[,i]);
for ( i in 1:size[2] )
	A[i+1,i] = 1;
b = array(0, dim = c(size[2]+1,1));
b[1] = 1;

solQP <- solve.QP( D, d, t(A) , b , meq = size[2]+1 ) ;

# rescale variables to obtain weights
w <- as.matrix(solQP$solution/sum(solQP$solution));

