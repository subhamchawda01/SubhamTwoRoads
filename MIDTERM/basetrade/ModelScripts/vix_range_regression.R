#!/usr/bin/env Rscript
#suppressPackageStartupMessages( library ( "numDeriv" , lib.loc="/apps/R/root/library/" ) );
.libPaths("/apps/R/root/library/")
suppressPackageStartupMessages( require('numDeriv') );

#################
#Algorithm
#y = Sum over i { Gi(Xi)  }
#Gi(X)=Bi * ( sigmoid ( Ai  * X ) - 0.5 )


#Reduces the ilist to "max_model_size" using lasso
GetDeltaProbsMatrix <- function ( ) {
  for ( i in seq( 1, num_instances ) ) {
    for ( j in seq( 1, num_regimes ) ) {
      if ( j==1 ) {
        if ( CY[i] <= regime_cutoffs[j] ) {
          Pmat[i,j] <<- 1;
        }
        else {
          Pmat[i,j] <<- 0;
        }
      }
      if ( j==(num_regimes) ) {
        if ( CY[i]>= regime_cutoffs[j] ) {
          Pmat[i,j] <<- 1;
        }
        else {
          Pmat[i,j] <<- 0;
        }
      }
      if ( j > 1 && j < ( num_regimes ) ) {
        if ( CY[i] < regime_cutoffs[j+1] && CY[i] >= regime_cutoffs[j-1] ) {
          Pmat[i,j] <<- 1;
        }
        else {
	  Pmat[i,j] <<- 0;          
        }
      }
    }
  }
}
GetProbsMatrix <- function ( ) {
  for ( i in seq( 1, num_instances ) ) { 
    for ( j in seq( 1, num_regimes ) ) {
      if ( j==1 ) {
	if ( CY[i] <= regime_cutoffs[j] ) {							                 
	  Pmat[i,j] <<- 1;
	}				
	else if ( CY[i] > regime_cutoffs[j] && CY[i] <= regime_cutoffs[j+1] ) {
          Pmat[i,j] <<- ( regime_cutoffs[j+1] - CY[i] ) / ( regime_cutoffs[j+1] - regime_cutoffs[j] ) ;
	}
        else {
	  Pmat[i,j] <<- 0;                    
        }
      }                                         
      if ( j==(num_regimes) ) {
        if ( CY[i]>= regime_cutoffs[j] ) {
          Pmat[i,j] <<- 1;
        }
        else if ( CY[i] >= regime_cutoffs[j-1] && CY[i] < regime_cutoffs[j] )  {  
          Pmat[i,j] <<- ( CY[i] - regime_cutoffs[j-1] ) / ( regime_cutoffs[j] - regime_cutoffs[j-1] ) ;
        }
        else {
          Pmat[i,j] <<- 0;                       
        }
      }
      if ( j > 1 && j < ( num_regimes ) ) {
        if ( CY[i] < regime_cutoffs[j] && CY[i] >= regime_cutoffs[j-1] ) {
	  Pmat[i,j] <<- ( CY[i] - regime_cutoffs[j-1] ) / ( regime_cutoffs[j] - regime_cutoffs[j-1] ) ;                               
        }
        else if ( CY[i] >= regime_cutoffs[j] && CY[i] < regime_cutoffs[j+1] ) {
          Pmat[i,j] <<- ( regime_cutoffs[j+1] - CY[i] ) / ( regime_cutoffs[j+1] - regime_cutoffs[j] );			                       					 
        }
        else {
	  Pmat[i,j] <<- 0;
	}
      }
    }
  }
}

# Subset Selection using a linear regression method.
# Ideally, we need to come up with a better way to do this. 
ReduceIndicators <- function ( t_regdata_file )
{
  system ( sprintf ('~/basetrade_install/bin/callFSRR %s %s %s %s %s %s %s %s 1>/dev/null ', t_regdata_file , 0.01, 0.01, 0, 0, 0.60, tmp_reg_output_file, max_model_size ) );
  system ( sprintf ('cat %s | grep OutCoeff | awk \'{print $2,$3}\' > %s', tmp_reg_output_file, tmp_reg_output_indicators ) );
  regout_text <- read.table (tmp_reg_output_indicators);
  print(regout_text);
  indicators_included <- c();
  inds <- rep ( 0, num_indicators );
  for ( i in c(1:nrow(regout_text)) ) {
    #t_regout_row <- unlist(strsplit(regout_text[i], "\\ "));
    #print(t_regout_row);
    #indicators_included <- c( indicators_included, as.numeric( regout_text[i][1] ) );
    inds [ as.numeric(regout_text[i,1]) ] <- 1;
  }
  retval <- inds; 
}

GetSelectedIndicators <- function ( ) 
{
  for ( i in seq (1,num_regimes) )
  {
    Pmat_tmp <- matrix ( Pmat[,i], num_instances, num_indicators + 1, byrow = FALSE );                 
    #write.table(Pmat_tmp,"Pmat_tmp_file");
    #t_regdata <- regdata * Pmat_tmp ;    
    t_regdata <- regdata ;
    write.table(t_regdata, t_regdata_file,col.names=FALSE, row.names = FALSE);
    selected_indicators[,i] <<- ReduceIndicators ( t_regdata_file );    
  }
}

ObjectiveFunction <- function (Weights)
{
  #regularization_penalty <- L1Norm ( Weights );       
  #regularization_penalty <- L2Norm ( Weights );       
  W <- matrix ( Weights, nrow=num_indicators, byrow=TRUE);
  #W <- as.matrix(Weights);
  Y_hat_all <- X %*% W;
  selection_penalty <- sum ( abs ( ( W *(  1 - selected_indicators ) ) * ( W *( 1 - selected_indicators ) ) ) ) * beta;
  Y_hat_all_weighted <- Y_hat_all * Pmat;
  Y_hat <- rowSums ( Y_hat_all_weighted  );
  error <- 0.5 * sum ( ( Y - Y_hat ) * ( Y - Y_hat ) ) / ( length ( Y ) ) ;
  #retval <- error + regularization_penalty + selection_penalty;
  retval <- error + selection_penalty ;
}

L2Norm <- function ( B ) {
  retVal <- lambda * sum ( B * B ) ;
}

L1Norm <- function ( B ) {
  retVal <- lambda * sum ( abs ( B ) ) ;
}

L1NormGrad <- function ( B ) {
	retVal <- grad (L1Norm,B,method="simple");
}

L2NormGrad <- function ( B ) {
	retVal <- grad (L2Norm,B,method="simple");
}

GradNumerical <- function ( B ) {
	retval <- grad (ObjectiveFunction, B,  method="simple");
}

lambda <- 0.00001;

beta <- 1000000000;
final_correlation <- 0;
final_mse <- 0;

r2_original <- 0;
r2_model <- 0;
r_squared <- 0;

sd_dep <- 0;
sd_model <- 0;
sd_residual <- 0;

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 4 ) {
  stop ("USAGE : <script> <regdata> <reg_output_file> <1 for delta regime 0 for continuous regime> <num_regimes> <regime_cutoff1> <regime_cutoff2> ... ");
}

max_model_size <- 10;

regdata_file <- args[1];
regdata <- read.table(args[1]);
regdata <- as.matrix(regdata);
t_regdata_file <- paste (regdata_file, "tmp", sep="_" );
reg_output_file <- args[2];
tmp_reg_output_file <- paste (reg_output_file,"tmp",sep="_");
tmp_reg_output_indicators <- paste(tmp_reg_output_file, "indicators", sep="_");
delta_model <- as.numeric(args[3]);
num_regimes <- as.numeric(args[4]);
regime_cutoffs <- c();

for ( i in c(1:(num_regimes) ))
{
  regime_cutoffs <- c(regime_cutoffs, as.numeric(args[4+i]));
}

print(regime_cutoffs);
#GetFactorsFromFile ( )
#model_text <- readLines (model_filename);

desired_size <- 700000000; # 700 MB
file_size <- file.info(regdata_file)$size;

downsample_ratio <- 1;

if ( file_size > desired_size )
{
	print ("Downsizing due to large size");
	downsample_ratio <- ceiling ( file_size / desired_size );
	regdata <- regdata [ seq (1, nrow(regdata), by = downsample_ratio ), ]; 
}

X <- regdata[,3:ncol(regdata)];
Y <- regdata[,1];
# conditioning variable
CY <- regdata[,2];
#CY <- scale(CY, scale = TRUE, center= TRUE);


num_indicators <- dim(X)[2];
sdx <- rep(0,num_indicators);
for ( i in c(1:num_indicators) )
{
  sdx[i] <- sd ( X[,i] );
  X[,i] <- X[,i]/sdx[i];
}
print(sdx);
sdy <- sd(Y);
Y <-  Y / sdy;
selected_indicators <- matrix ( 0, num_indicators, num_regimes );
num_params <- num_indicators * num_regimes;

regdata <- cbind ( Y, X );

B <- rep (1, num_params );
num_instances <- nrow ( X );
Pmat <- matrix ( 1, num_instances, num_regimes );

#print(num_instances);
#head(CY);


if( delta_model == 0 && num_regimes > 1 ){
  GetProbsMatrix ( );
}
if( delta_model == 1 && num_regimes > 1 ) {
 GetDeltaProbsMatrix ( );
}

initial_corrs <- cor(Y,X);
initial_corrs[is.na(initial_corrs)] <- 0
print (initial_corrs );
print(rowSums(Pmat));
GetSelectedIndicators ( );
#print(selected_indicators);
res <- optim(B, ObjectiveFunction, GradNumerical, method = "BFGS");
Weights <- res$par;
print(Weights);
#Weights <- selected_indicators;
cat("Correlation check: ", cor(Y, X %*% Weights));
W <- matrix ( Weights, nrow=num_indicators, byrow=TRUE );
W <- W * sdy;

for ( i in c(1:num_indicators))
{
  X[,i] <- X[,i] * sdx[i];
  W[i,] <- W[i,] / sdx[i];
}
#print(W);
Y <- Y * sdy;

Y_hat_all <- X %*% W;
Y_hat_all_weighted <- Y_hat_all * Pmat;
Y_hat <- rowSums ( Y_hat_all_weighted  );

sd_dep <- sdy;
sd_model <- sd(as.vector(Y_hat));
sd_residual <- sd(as.vector( Y - Y_hat ));
final_correlation <- cor(Y,Y_hat);
final_mse <- sqrt(sum((Y - Y_hat) * (Y - Y_hat))/(length(Y)));

r2_residual <<- sum ( ( Y_hat - Y ) ^ 2 );
r2_original <<- sum ( (Y - mean(Y))^ 2 );
r_squared <<- 1 - r2_residual / r2_original;

cat ( "OutConst 0 0", file = reg_output_file );
cat ( "\n" , file= reg_output_file, append=TRUE );

for ( i in c(1:num_regimes) )
{
  cat ( "Factor ", i , "\n", file=reg_output_file, append=TRUE );
  for ( j in c(1:num_indicators) )
  {       
	  cat ( "OutCoeff", W[j,i], "\n", file=reg_output_file, sep=" ", append=TRUE );
  }     
}

cat ( "RSquared ", r_squared, "\n", file = reg_output_file, sep = " ", append =TRUE);
cat ( "Correlation ", final_correlation ,"\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "StdevDependent ", sd_dep, "\n", file = reg_output_file, sep = " ", append=TRUE );
cat ( "StdevResidual ", sd_residual, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "StdevModel ", sd_model, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "MSE ", final_mse ,"\n", file = reg_output_file, sep = " ", append = TRUE );
