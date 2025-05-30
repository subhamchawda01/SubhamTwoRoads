#!/usr/bin/env Rscript
#suppressPackageStartupMessages( library ( "numDeriv" , lib.loc="/apps/R/root/library/" ) );
.libPaths("/apps/R/root/library/")
suppressPackageStartupMessages( require('numDeriv') );

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

#Computes the average no. of instances abs(indicator) exceeds the target bias


sigmoid <- function ( x )
{
  retval <- 1/(1 + a*(x-c) );
}

sigma <- function ( a, c , x )
{
  retval <- sigmoid ( a*(x - c) );
}

ObjectiveFunction <- function ( B )
{
  Y_hat <- X %*% B;  
  objective_function <- 0.0;
  for ( i in c(1:num_classes) )
  {
    error_class <- 0.0;
    for ( j in c(1:num_classes) )
    {
      error_class <- error_class + E[i,j]*( sigma(a,c[j],Y_hat) - sigma(a,c[j+1],Y_hat) );
    }
    objective_function <- objective_function + error_class * ( sigma(a,c[i],Y) - sigma(a,c[i+1],Y) );
  }  
  retval <- sum( objective_function ) / nrow(X);
}

ObjectiveFunctionGrad <- function ( B )
{
  total_grad <- 0.0;

  Y_hat <- X %*% B;

  for ( i in c(1:num_classes) )
  {
    class_error_grad <- 0.0;
    for ( j in c(1:num_classes) )
    {
      class_error_grad <- class_error_grad + E[i,j] * a * (sigma(a,c[j],Y_hat) * ( 1 - sigma(a,c[j],Y_hat) ) - sigma(a,c[j+1],Y_hat)*( 1 - sigma(a,c[j+1],Y_hat) ) );
    }
    total_grad <- total_grad + t(X) %*% ( class_error_grad * (sigma(a,c[i],Y) - sigma(a,c[i+1],Y)) ) ;
  }
  retval <- total_grad / nrow(X);
}

L2Norm <- function ( B ) {
  retVal <- lambda * sum ( B * B ) ;
}

L1Norm <- function ( B ) {
  retVal <- lambda * sum ( abs ( B  ) ) ;
}

LpNorm <- function ( B ) {
  retVal <- lambda * ( sum ( abs( B ) ^ p_norm ) ) ; 
}

LpNormGrad <- function ( B ) {
  retVal <- grad (LpNorm,B,method="simple");
}

L1NormGrad <- function ( B ) {
  retVal <- grad (L1Norm,B,method="simple");
}

L2NormGrad <- function ( B ) {
  retVal <- grad (L2Norm,B,method="simple");
}

CReg <- function ( B ) {

  regularization_penalty <- 0;

  error_function <- ObjectiveFunction (B);

  if ( regularization_method == "L2" )
  {
    regularization_penalty <- L2Norm(B);
  }
  if ( regularization_method == "L1" )
  {
    regularization_penalty <- L1Norm(B);
  }
  if ( regularization_method == "LP" )
  {
    regularization_penalty <- LpNorm(B);	
  }	

  retVal <- regularization_penalty + error_function;

}

#Calculates the gradient for the learning parameters Ai and Bi analytically.
CRegGradAnalytical <- function ( B ) {
  objective_grad <- ObjectiveFunctionGrad ( B );	
  regularization_penalty_grad <- 0;

  if ( regularization_method == "L2" )
  {
    regularization_penalty_grad <- L2NormGrad ( B );
  }
  if ( regularization_method == "L1" )
  {
    regularization_penalty_grad <- L1NormGrad ( B );
  }
  if ( regularization_method == "LP" )
  {
    regularization_penalty_grad <- LpNormGrad ( B );
  }

  retVal <- regularization_penalty_grad + objective_grad ;	
}

#Calculates the gradient using inbuilt R function.
CRegGradNumerical <- function ( B ) {
  retVal <- grad (CReg,B,method="simple");
}

ComputeConfusionMatrix <- function ( Y, Y_hat ) 
{
  confusion_matrix <- matrix(0,num_classes, num_classes );	
  for ( i in c(1:num_classes) )
  {
    class_indices <- which (Y > c[i] & Y < c[i+1]);
    num_instances <- length(class_indices);
    Y_class <- Y_hat [ class_indices ];
    for ( j in c(1:num_classes) )
    {		  
      confusion_matrix[i,j] <- length(which( Y_class > c[j] & Y_class < c[j+1] )) / num_instances;
    }
  }   
  retval <- confusion_matrix;
}

final_correlation <- 0;
final_mse <- 0;
final_error_function <- 0;

r2_original <- 0;
r2_model <- 0;
r_squared <- 0;

sd_dep <- 0;
sd_model <- 0;
sd_residual <- 0;

a <- 100000;
num_classes <- 5;
c <- c(-1000000,-0.6,-0.2,0.2,0.6,100000);

r1 <- c(0,1,2,4,8);
r2 <- c(2,0,1,4,8);
r3 <- c(4,1,0,1,4);
r4 <- c(8,4,1,0,2);
r5 <- c(8,4,2,1,0); 

E <- rbind(r1,r2,r3,r4,r5);

args = commandArgs( trailingOnly=TRUE )
  if ( length(args) < 4 ) {
    stop ("USAGE : <script> <regdata> <reg_output_file> <max_model_size> <penalty[N,L0,LP,L1,L2]> <optional: p in (0,1) for LP>\n N = no regularization \n L0 = penalty on high L0 norm of coefficients i.e, number of non_zero indicators \n L1 = penalty on high L1 norm of coefficients \n L2 = penalty on high L2 norm of coefficients \n");
  }

regdata_file <- args[1];
regdata <- read.table(args[1]);
regdata <- as.matrix(regdata);
reg_output_file <- args[2];
tmp_reg_output_file <- paste (reg_output_file,"tmp",sep="_");
max_model_size <- as.numeric(args[3]);
regularization_method <- args[4];

desired_size <- 700000000; # 700 MB
file_size <- file.info(regdata_file)$size;

downsample_ratio <- 1;

if ( file_size > desired_size )
{
  downsample_ratio <- ceiling ( file_size / desired_size );
  regdata <- regdata [ seq (1, nrow(regdata), by = downsample_ratio ), ]; 
}

num_rows_ <- dim ( regdata ) [1] 
num_cols_ <- dim ( regdata ) [2];
regdata <- as.numeric ( regdata ) ; #when numbers are too larget, R loads them as string, need to convert to numeric
regdata <- matrix ( regdata, nrow=num_rows_, ncol=num_cols_ ) ;

X <- regdata[,2:ncol(regdata)];
Y <- regdata[,1]

indicators_included <- c();
#ReduceIndicators();
N <- dim(X)[1]

initial_corrs <- cor(Y,X);
initial_corrs[is.na(initial_corrs)] <- 0

num_indicators <- dim(X)[2];
sdx <- rep(0,num_indicators);

for ( i in c(1:num_indicators))
{
  sdx[i] <- sd ( X[,i] );
  X[,i] <- X[,i]/sdx[i];
}

sdy <- sd(Y);
Y <- Y / sdy;

p_norm <- 0.5;

if ( length(args) > 4 )
{
  p_norm = as.numeric(args[5]);
}

B <- rep (1, num_indicators );

lambda <- 0.01;

best_objective <- 1000000;

res <- optim(B, CReg, CRegGradAnalytical, method = "BFGS");
Beta <- res$par;
final_objective <- res$value;
for ( i in c(1:num_indicators))
{
  if(abs(Beta[i]) < 0.001)
  {
    Beta[i] <- 0;
  }
  Beta[i] <- Beta[i]/sdx[i];
}       
Beta <- Beta * sdy;

for ( i in c(1:num_indicators))
{
  X[,i] <- X[,i] * sdx[i];
}

Y <- Y * sdy;

Y_hat <- X %*% Beta;

model_confusion_matrix <- ComputeConfusionMatrix ( Y, Y_hat );

sd_dep <- sdy;
sd_model <- sd(as.vector(Y_hat));
sd_residual <- sd(as.vector( Y - Y_hat ));
final_correlation <- cor(Y,Y_hat);
final_mse <- sqrt(sum((Y - Y_hat) * (Y - Y_hat))/(length(Y)));

r2_residual <<- sum ( ( Y_hat - Y ) ^ 2 );
r2_original <<- sum ( Y ^ 2 );

r_squared <<- 1 - r2_residual / r2_original;

lmodel <- lm ( Y ~ X + 0 );
Y_hat <- lmodel$fitted.values ;
ols_confusion_matrix <- ComputeConfusionMatrix ( Y, Y_hat );

ols_beta <- lmodel$coefficients;
ols_final_objective <- ObjectiveFunction ( ols_beta );

cat ( "OutConst 0 0", file = reg_output_file );
cat ( "\n" , file= reg_output_file, append=TRUE );

for ( i in c(1:num_indicators) )
{
  cat ( "OutCoeff", (indicators_included[i] -1), Beta[i], file = reg_output_file, sep= " ", append = TRUE );
  cat ( "\n", file = reg_output_file, append = TRUE ) ;
}

cat ( "RSquared ", r_squared, "\n", file = reg_output_file, sep = " ", append =TRUE);
cat ( "Correlation ", final_correlation ,"\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "StdevDependent ", sd_dep, "\n", file = reg_output_file, sep = " ", append=TRUE );
cat ( "StdevResidual ", sd_residual, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "StdevModel ", sd_model, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "MSE ", final_mse ,"\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "Objective ", final_objective, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "OLS Objective ", ols_final_objective, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "Confusion Matrix Model", "\n", file = reg_output_file, sep = " ", append = TRUE );
for ( i in c(1:num_classes) )
{
  cat ( model_confusion_matrix[i,], "\n", file = reg_output_file, sep = " ", append = TRUE );
}

cat ( "\n", "Confusion Matrix OLS", "\n", file = reg_output_file, sep = " ", append = TRUE );
for ( i in c(1:num_classes) )
{
  cat ( ols_confusion_matrix[i,], "\n", file = reg_output_file, sep = " ", append = TRUE );
}
