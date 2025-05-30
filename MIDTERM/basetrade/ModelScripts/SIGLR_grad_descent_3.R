#!/usr/bin/env Rscript
#suppressPackageStartupMessages( library ( "numDeriv" , lib.loc="/apps/R/root/library/" ) );
.libPaths("/apps/R/root/library/")
suppressPackageStartupMessages( require('numDeriv') );
suppressPackageStartupMessages( require('glmnet') );
suppressPackageStartupMessages( require('methods') );
options(warn=-1)
#===========================================================

cv_glmnet_regression = function( y,x, alpha, nfolds ){
  require("glmnet");
  
  #order = order(regdata);
  #regdata = regdata[order, ];
  
  #foldid  = rep( 1:nfolds, floor( nrow(regdata)/nfolds) );
  
  #regadata = regdata[1:length(foldid), ];
  
  #x = as.matrix(regdata[,-1]) ;l
  #y = as.matrix(regdata[,1] ) ;  
  
  cvfit = cv.glmnet(x = x,y = y,intercept = FALSE,nfolds = nfolds,type.measure = "mse",alpha = alpha);
  glm_obj = cvfit$glmnet.fit;
  lambda_min = cvfit$lambda.min;
  lambda_vec = cvfit$lambda;
  
  min_mse_ind = which(lambda_vec == lambda_min);
  
  mse = cvfit$cvm[min_mse_ind];
  weights = glm_obj$beta[,min_mse_ind];
  
  l = list ("weights" = weights, "mse" = mse, "lambda" = lambda_min);
  return(l)
}

#===================================================

H_grad = function ( i,alpha ) {

 grad = matrix( rep( 0, nrow(X_train)*ncol(X_train) ), nrow = nrow(X_train), ncol = ncol(X_train) ) ;
 grad = as.data.frame( grad ); 

 temp_vec = X_train[,i] *exp(-alpha[i]*X_train[,i] ) /  ( ( 1 + exp( -alpha[i]*X_train[,i] ) )^2 ) ;

 grad[,i] = temp_vec ;  
 return ( grad );

}

#===============================


beta_grad = function(H, alpha,i ) {

 H_mat = as.matrix( H ); 
 d_H = as.matrix( H_grad( i,alpha ) ) ;
  
 HT_H = t(H_mat)  %*%  H_mat  ; 
 d_HT_H = ( t(H_mat) %*% d_H )  + ( t(d_H ) %*% H_mat ) ;

 HT_H_inv = solve( t(H_mat) %*% H_mat ) 

 grad = ( ( -HT_H_inv %*% d_HT_H%*%HT_H_inv ) %*% t( H_mat ) ) + ( HT_H_inv %*%  t(d_H) )
 grad = grad %*% Y_train ;

 return ( grad ) ;

}
#======================================================================
SIGLR_new <- function ( alpha ) {
 
     
  T_optim <<- X_train * t(replicate(nrow(X_train),alpha));
  H_optim <<- 1/(1 + exp(-T_optim)) ;
  HH_optim <<- H_optim - 0.5;

  reg_df_optim <<- cbind( Y_train, HH_optim );
  reg_df_optim <<- as.data.frame(reg_df_optim);
  lm_optim <<- lm( Y_train ~ . +0, reg_df_optim);
  beta_optim <<- lm_optim$coefficients;
  alpha_optim <<- alpha  
  B <<- c( alpha_optim, beta_optim );
  Y_hat_train_optim <- HH_optim %*% beta_optim;

  retVal <- sum ( ( Y_train - Y_hat_train_optim ) * ( Y_train - Y_hat_train_optim ) ) / ( length(Y_hat_train_optim ) * 2 );   
  return(retVal)
}


#==============================================

alphagrad = function ( alpha ) {

   diff <- as.vector(Y_train - Y_hat_train_optim) ;
   t_grad_alpha <- -colSums(replicate(num_indicators,diff) * t(replicate (length(Y_train),beta_optim)) * H_optim * ( 1 - H_optim ) * X_train );       # d((y-y_hat).(y-y_hat))/d(alpha) 

   retVal <- ( t_grad_alpha/length(Y_train) ); 
   return(retVal);   
}


#################
#Algorithm
#y = Sum over i { Gi(Xi)  }
#Gi(X)=Bi * ( sigmoid ( Ai  * X ) - 0.5 )


#Reduces the ilist to "max_model_size" using lasso
ReduceIndicators <- function ()
{
	system (sprintf('~/basetrade/ModelScripts/call_slasso.pl %s %s %s 1>/dev/null ', args[1], max_model_size, tmp_reg_output_file ))
        regout_text <- readLines (tmp_reg_output_file);
        for ( i in c(1:length(regout_text)) )
        {
            t_regout_row <- unlist(strsplit(regout_text[i], "\\ "));
            indicators_included <<- c( indicators_included, as.numeric( t_regout_row[1] ) );
        }	
	X <<- X[,indicators_included];
}

final_correlation <- 0;
final_mse <- 0;

r2_original <- 0;
r2_model <- 0;
r_squared <- 0;

sd_dep <- 0;
sd_model <- 0;
sd_residual <- 0;

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 3  ) {
        stop ("USAGE : <script> <regdata> <reg_output_file> <max_model_size> \n");
}

regdata_file <- args[1];
regdata <- read.table(args[1]);
regdata <- as.matrix(regdata);
num_rows_ <- dim ( regdata ) [1] 
num_cols_ <- dim ( regdata ) [2];
regdata <- as.numeric ( regdata ); #when numbers are too larget, R loads them as string, need to convert to numeric
regdata <- matrix ( regdata, nrow=num_rows_, ncol=num_cols_ ) ;

reg_output_file <- args[2];
tmp_reg_output_file <- paste (reg_output_file,"tmp",sep="_");
max_model_size <- as.numeric(args[3]);



desired_size <- 700000000; # 700 MB
file_size <- file.info(regdata_file)$size;

downsample_ratio <- 1;

if ( file_size > desired_size )
{
	downsample_ratio <- ceiling ( file_size / desired_size );
	regdata <- regdata [ seq (1, nrow(regdata), by = downsample_ratio ), ]; 
}

X <- regdata[,2:ncol(regdata)];
Y <- regdata[,1]

indicators_included <- c();
ReduceIndicators();
N <- dim(X)[1]

X_train <- X;
Y_train <- Y;

initial_corrs <- suppressWarnings(cor(Y_train,X_train));
initial_corrs[is.na(initial_corrs)] <- 0

num_indicators <- dim(X)[2];
sdx <- rep(0,num_indicators);

for ( i in c(1:num_indicators))
{
   sdx[i] <- sd ( X_train[,i] );
   X_train[,i] <- X_train[,i]/sdx[i];
}

     sdy <- sd(Y_train);
     Y_train <- Y_train / sdy;

     num_params <- num_indicators ;

     alpha <- rep (1, num_params );


     best_alpha <- rep(0,num_indicators);
     best_beta <- rep(0,num_indicators);


     T_optim <<- X_train * t(replicate(nrow(X_train),alpha));
     H_optim <<- 1/(1 + exp(-T_optim)) ;
     HH_optim <<- H_optim - 0.5;

     reg_df_optim <<- cbind( Y_train, HH_optim );
     reg_df_optim <<- as.data.frame(reg_df_optim);
     lm_optim <<- lm( Y_train ~ . +0, reg_df_optim);
     beta_optim <<- lm_optim$coefficients;
     alpha_optim <<- alpha

     B <<- c( alpha_optim, beta_optim );
     Y_hat_train_optim <<- HH_optim %*% beta_optim;

     h_bound = 4; # 25 % of sd
     l_bound = 0.1; # 1000 % of sd

     res <- optim( alpha, SIGLR_new, alphagrad, method = "L-BFGS-B",lower = rep( l_bound, length( alpha ) ) ,upper = rep( h_bound, length(alpha)  ) );
        
     alpha <- res$par;
 
     T <- X_train * t(replicate(nrow(X_train),alpha));
     H <- 1/(1 + exp(-T)) - 0.5;
        
     nfolds = 5;
     #alpha refers to elastic net alpha in (0,1)
     alpha_glm_vec = seq(from = 0.1, to = 0.9, by = 0.2 )
  
     optimal_alpha_glm = -1 ;
     optimal_lambda_glm = -1; 
     min_mse = 1e07;
     beta = c();

     for( i in 1:length(alpha_glm_vec ) ) {
        l = cv_glmnet_regression( Y_train, H,alpha_glm_vec[i],nfolds )  ;
        if( min_mse > l$mse ) {
            min_mse = l$mse;
            optimal_alpha_glm = alpha_glm_vec[i] ;
            optimal_lambda_glm = l$lambda; 
            beta = l$weights; 
         }
      } 

      for ( i in c(1:num_indicators)){
        alpha[i] <- alpha[i]/sdx[i];
      }       

      beta <- beta * sdy;
      best_alpha <<- alpha;
      best_beta <<- beta;


for ( i in c(1:num_indicators))
{
    X_train[,i] <- X_train[,i] * sdx[i];
}

Y_train <- Y_train * sdy;

alpha <- best_alpha ;
beta <- best_beta ;

T <- X_train * t(replicate(nrow(X_train),alpha)) ;
H <- 1/(1 + exp(-T)) - 0.5;
HH <- H * t(replicate(nrow(X_train),beta));
Y_hat_train <- H %*% beta;

sd_dep <- sdy;
sd_model <- sd(as.vector(Y_hat_train));
sd_residual <- sd(as.vector( Y_train - Y_hat_train ));
final_correlation <- suppressWarnings(cor(Y_train,Y_hat_train));
final_mse <- sqrt(sum((Y_train - Y_hat_train) * (Y_train - Y_hat_train))/(length(Y_train)));

domination <- 0;
domination_metric <- sum ( ( abs(HH) - abs( replicate( num_indicators,as.vector( Y_hat_train ) ) ) ) > 0 )

domination <- domination / ( length ( Y_hat_train ) * num_indicators );

r2_residual <<- sum ( ( Y_hat_train - Y_train ) ^ 2 );
r2_original <<- sum ( (Y_train - mean(Y_train))^ 2 );

r_squared <<- 1 - r2_residual / r2_original;

cat ( "OutConst 0 0", file = reg_output_file );
cat ( "\n" , file= reg_output_file, append=TRUE );

for ( i in c(1:num_indicators) )
{
        cat ( "OutCoeff", (indicators_included[i] -1), alpha[i],beta[i], file = reg_output_file, sep= " ", append = TRUE );
        cat ( "\n", file = reg_output_file, append = TRUE ) ;
}

cat ( "RSquared ", r_squared, "\n", file = reg_output_file, sep = " ", append =TRUE);
cat ( "Correlation ", final_correlation ,"\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "StdevDependent ", sd_dep, "\n", file = reg_output_file, sep = " ", append=TRUE );
cat ( "StdevResidual ", sd_residual, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "StdevModel ", sd_model, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "MSE ", final_mse ,"\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "Domination ", domination , "\n", file =  reg_output_file, sep = " ", append = TRUE );

if ( length(args) > 6)
{
	testregdata <- as.matrix(read.table(args[7]));
	X_cv_test <- testregdata[,2:ncol(testregdata)];
	X_cv_test <- X_cv_test[,indicators_included];
	Y_cv_test <- testregdata[,1];
	T <- X_cv_test * t(replicate(nrow(X_cv_test),alpha));
        H <- 1/(1 + exp(-T)) - 0.5;
        Ybar_test <- H %*% beta;
	
	error <- sum ( ( Ybar_test - Y_cv_test) ^ 2)/sum( (Y_cv_test - mean(Y_cv_test))^2); 
        if (error < as.numeric(args[8])) 
        { 
                cat("1\n",file = args[9]); 
        } 
        else 
        { 
                cat("0\n",file = args[9]); 
        } 
	cat("Test error ",error,"\n",file = args[9],append=TRUE);
}


