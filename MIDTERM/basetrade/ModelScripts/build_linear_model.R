#!/usr/bin/env Rscript


source ( "~/basetrade_install/ModelScripts/regression_utils.R" )

args <- commandArgs(trailingOnly = TRUE);
options(warn=-1)

if ( length ( args ) < 2 )
{
  stop ( " Usage: <script> <regdata> <regout> \n " ) ;
}

data <- read.table(args[1]);
data <- as.matrix(data);
num_rows_ <- dim ( data ) [1] 
num_cols_ <- dim ( data ) [2];
data <- as.numeric ( data ) ; #when numbers are too larget, R loads them as string, need to convert to numeric
data <- matrix ( data, nrow=num_rows_, ncol=num_cols_ ) ;
x <- data[,2:ncol(data)]; 
y <- data[,1];

remove(data);

#y_subset <- reducePredictors ( x , y , "AbSh" , 18 )

lmodel <- lm ( y ~ x + 0 );
coeffs <- coef (lmodel) ;
coeffs[which(is.na(coeffs))] <- 0;
selected_indicators <- which(coeffs!=0) ;
selected_indicators_weights <- coeffs[ which(coeffs!=0 ) ] ;
ind_n_weights <- cbind (selected_indicators,selected_indicators_weights) ;
write.table(ind_n_weights, args[2],col.names=F,row.names=F);
