#!/usr/bin/env Rscript
#nonrobust
.libPaths(c("/usr/lib64/R/library","/home/apps/R/R-3.1.0/library" ))

args = commandArgs( trailingOnly=TRUE )
#changing to static regression
if( length(args) < 1 )
{
  stop ("USAGE : <data_file> \n");
}

#File with n lines where each line has format stock_px predictor_px1 .. predictor_px2
data_file <- read.table(args[1])

#number of predictors
num_predictors <- length(data_file[1,]) - 1;

#do regression against all others 
Z <- as.data.frame(data_file);
reg_othrs <- lm( as.formula(paste(colnames(Z)[1], "~ 0+", paste(colnames(Z)[c(2:(num_predictors+1))], collapse = "+" ), sep = "" )), data = Z)
cat(sprintf("%f\n",cor(Z[,1], fitted(reg_othrs))));
cat(sprintf("%s\n",summary(reg_othrs)$coefficients[1:num_predictors]));
