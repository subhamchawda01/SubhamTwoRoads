#!/usr/bin/env Rscript


source ( "~/basetrade_install/ModelScripts/regression_utils.R" )

args <- commandArgs(trailingOnly = TRUE);

if ( length ( args ) < 2 )
{
  stop ( " Usage: <script> <regdata> <regout> \n " ) ;
}

data <- read.table(args[1]);
data <- as.matrix(data);
x_ <- data[,2:ncol(data)]; 
y_ <- data[,1];

remove(data);

elem_wise_x_y_ = x_ * as.vector(y_);
ones_ = t(t(rep(1.0, length(y_))));
lmodel_ <- lm ( ones_ ~ 0 + elem_wise_x_y_ );
coeffs_ <- coef (lmodel_) ;
coeffs_[which(is.na(coeffs_))] <- 0;
#coeffs_ = coeffs_ / sd(x_ %*% t(t(coeffs_)) );
selected_indicators <- which(coeffs_!=0) ;
selected_indicators_weights <- coeffs_[ which(coeffs_!=0 ) ] ;
ind_n_weights_ <- cbind (selected_indicators,selected_indicators_weights) ;
write.table(ind_n_weights_, args[2],col.names=F,row.names=F);
