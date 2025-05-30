#!/usr/bin/env Rscript


source ( "~/basetrade_install/ModelScripts/regression_utils.R" )

args <- commandArgs(trailingOnly = TRUE);

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
library (MASS);

l_vec_ <- 0.1 ^ (1:5) * 10;
lmodel <- lm.ridge ( y ~ x + 0, lambda = l_vec_ );
l_vec_ <- seq(0,1,0.1);
l_vec_
lmodel$GCV
index_ <- which ( lmodel$GCV==min ( lmodel$GCV ) ); 
lam_ <- l_vec_ [index_ ];
index_
lam_
lmodel <- lm.ridge ( y ~ x + 0, lambda = lam_ );

coeffs <- coef (lmodel) ;
coeffs[which(is.na(coeffs))] <- 0;
selected_indicators <- which(coeffs!=0) ;
selected_indicators_weights <- coeffs[ which(coeffs!=0 ) ] ;
ind_n_weights <- cbind (selected_indicators,selected_indicators_weights) ;

write.table(ind_n_weights, args[2],col.names=F,row.names=F);

#stats 
pred_data <- as.matrix ( x  ) %*% as.matrix ( coeffs ) ;

r2_original <- sum ( y ^ 2 ) ;
r2_residual <- sum ( (y-pred_data) ^ 2 ) ;
r_squared <- ( r2_original - r2_residual ) / r2_original;

cor_model <- cor ( y, pred_data );
y<-as.matrix(y);
sd_dep <- apply(y,2,sd) ;
sd_residual <- apply(y-pred_data,2,sd);
sd_model <- apply(pred_data,2,sd) ;
mse <- mean ( (y-pred_data)^2 );
abs_err <- mean ( abs ( y-pred_data ) ) ;

for ( i in 1:length(coeffs) ) { cat("OutCoeff "); cat (coeffs[i]); cat ("\n"); }
cat ("RSquared "); cat (r_squared); cat ("\n");
cat ("Correlation "); cat (cor_model); cat ("\n");
cat ("StdevDependent "); cat (sd_dep); cat ("\n");
cat ("StdevResidual "); cat (sd_residual); cat ("\n");
cat ("StdevModel "); cat (sd_model); cat ("\n");
cat ("MSE "); cat (mse); cat ("\n");
cat ("MeanAbsError "); cat (abs_err); cat ("\n");

