#!/usr/bin/env Rscript
#nonrobust
.libPaths(c("/usr/lib64/R/library","/home/apps/R/R-3.1.0/library","/home/apps/R/R-3.2.1/library" ))
require(nnls)

args = commandArgs( trailingOnly=TRUE )
#changing to static regression
if( length(args) < 1 )
{
  stop ("USAGE : <data_file> \n");
}

#File with n lines where each line has format stock_px predictor_px1 .. predictor_px2
data_file <- read.table(args[1])

num_cols <- length(data_file[1,]);

X <- as.matrix(data_file[,2:num_cols]);
Y <- as.numeric(data_file[,1]);

R <- nnls(X,Y);

cat(sprintf("%f\n",cor(Y,R$fitted)));
cat(sprintf("%s\n", R$x));
