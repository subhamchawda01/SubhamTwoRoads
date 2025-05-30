.libPaths("/apps/R/root/library/")
suppressPackageStartupMessages( require('numDeriv') );

#=========================Compute the Probability matrix for Continuous regime Model ===

GetProbsMatrix <- function ( start_index, end_index, num_regimes, CY, regime_cutoffs ) {
	Pmat <- mat.or.vec ( ( end_index - start_index + 1 ) , num_regimes );
  r_index = 1;
  for ( i in seq(start_index,end_index) ) {
    for ( j in seq(1,num_regimes) ) {
    #cat("CY ", CY[i], " regime cutoff ", regime_cutoffs[j], "\n");
      if ( j==1 ) {
        if ( CY[i] <= regime_cutoffs[j] )  {                               
          Pmat[r_index,j] <- 1;              
        }                               
        else if ( CY[i] > regime_cutoffs[j] && CY[i] <= regime_cutoffs[j+1] ) { 
          Pmat[r_index,j] <- ( regime_cutoffs[j+1] - CY[i] ) / ( regime_cutoffs[j+1] - regime_cutoffs[j] ) ;
        }
        else { 
          Pmat[r_index,j] <- 0;  
        }                   
     }                      
     if ( j==(num_regimes) ) {
        if ( CY[i]>= regime_cutoffs[j] ) { 
          Pmat[r_index,j] <- 1;
        }
        else if ( CY[i] >= regime_cutoffs[j-1] && CY[i] < regime_cutoffs[j] ) {  
          Pmat[r_index,j] <- ( CY[i] - regime_cutoffs[j-1] ) / ( regime_cutoffs[j] - regime_cutoffs[j-1] ) ;
        }
        else { 
          Pmat[r_index,j] <- 0;
        }
      }
      if ( j > 1 && j < ( num_regimes ) ) {
        if ( CY[i] < regime_cutoffs[j] && CY[i] >= regime_cutoffs[j-1] ) {
          Pmat[r_index,j] <- ( CY[i] - regime_cutoffs[j-1] ) / ( regime_cutoffs[j] - regime_cutoffs[j-1] ) ;
        }
        else if ( CY[i] >= regime_cutoffs[j] && CY[i] < regime_cutoffs[j+1] ) {
          Pmat[r_index,j] <- ( regime_cutoffs[j+1] - CY[i] ) / ( regime_cutoffs[j+1] - regime_cutoffs[j] );
        }
				else {
					Pmat[r_index,j] <- 0;
				}
      }
    }
    r_index = r_index + 1;
  }
	return(Pmat);
}

#========= Selecting indicators based on FSRR =========================

ReduceIndicators <- function ( t_regdata_file, Continuous_Regime ) {
  regularization_coeff_ = 0.01;
  min_correlation_ = 0.01;
  first_indep_weight_ = 0;
  must_include_first_k_independants_ = 0;
  max_indep_correlation_ = 0.85;
  if( length(args) > ( 3 + num_regimes ) )
  {
    if( toString( args[ 4 + num_regimes ] ) == "FSRR"  ){
      start_index = num_regimes+5;
      end_index = length(args);
      if ( end_index - start_index >= 4 )
      {
        regularization_coeff_ = args[start_index];
        min_correlation_ = args[ start_index + 1 ];
        first_indep_weight_ = args[ start_index + 2 ];
        must_include_first_k_independants_ = args[ start_index + 3 ];
        max_indep_correlation_ = args[start_index + 4 ];

        if ( end_index - start_index >= 5 )
        { max_model_size = args[ start_index + 5 ]; }

      }
    }
  }
  system ( sprintf ('%s/callFSRR %s %s %s %s %s %s %s %s 1 &>/dev/null ', LIVE_BIN_DIR, t_regdata_file , regularization_coeff_, min_correlation_, first_indep_weight_, must_include_first_k_independants_, max_indep_correlation_, tmp_reg_output_file, max_model_size ) );
#system ( sprintf ('~/basetrade_install/bin/callFSRR %s %s %s %s %s %s %s %s 1 &>/dev/null ', t_regdata_file , 0.01, 0.01, 0, 0, 0.60, tmp_reg_output_file, max_model_size ) );
  system ( sprintf ('cat %s | grep OutCoeff | awk \'{print $2,$3}\' > %s', tmp_reg_output_file, tmp_reg_output_indicators ) );
  regout_text <- read.table (tmp_reg_output_indicators);
  #indicators_included <- c();
  #print(regout_text);
  if( Continuous_Regime ){
    regime_inds <<- c( regime_inds, nrow(regout_text ) );
    for ( i in c(1:nrow(regout_text)) ) {
      indicators_included <<- c( indicators_included, as.numeric( regout_text[i,1] )+1 );
    }
  } else {
    delta_regime_inds <<- c(delta_regime_inds, nrow(regout_text));
    for( i in c( 1:nrow(regout_text) ) ) {
      delta_indicators_included <<- c( delta_indicators_included, as.numeric( regout_text[i,1])+1 );
    }
  }
}

#======== Iteratively parsing all regimes for selecting indicators for every regime =======

GetSelectedIndicators <- function ( Pmat, Continuous_Regime ) {
  if(Continuous_Regime){
    model_regimes = num_regimes;
  } else {
    model_regimes = num_regimes+1;
  }
  for ( i in seq (1,model_regimes) ) {
    Pmat_tmp <- matrix ( Pmat[,i], num_instances, num_indicators + 1, byrow = FALSE );
    t_regdata <- as.matrix( regdata * Pmat_tmp );
    #cat ("Dim t_regdata ", dim(t_regdata), "\n" );
    write.table(t_regdata, t_regdata_file, col.names=FALSE, row.names = FALSE);
    ReduceIndicators ( t_regdata_file, Continuous_Regime );
  }
}

#======== L1 penalty function ================================================
L1Norm <- function ( B ) {
  retVal <- lambda * sum ( abs ( B ) ) ;
}

#======== Gradient for Objective function ====================================
GradNumerical <- function ( B ) {
  retval <- grad (ObjectiveFunction, B,  method="simple");
}


#======== Objective Function for finding optimum weights =====================
ObjectiveFunction <- function (Weights) {
  Y_hat = matrix(0,num_instances,1);
	regularization_penalty <- L1Norm ( Weights );
  if ( Continuous_Regime ) {
    model_X = X_selected;
    model_regimes = num_regimes;
    modelPmat = Pmat;
    model_regime_inds = regime_inds;
  } else {
    model_X = delta_X_selected;
    model_regimes = num_regimes + 1;
    modelPmat = delta_Pmat;
    model_regime_inds = delta_regime_inds;
    #cat("Model regime: ", model_regimes, "model Pmat ", modelPmat[1,], "Indices ", model_regime_inds, "\n");
  }
  Y_hat <- GetYHatRegime (model_X, Weights, model_regimes, model_regime_inds, modelPmat, num_instances);
	error <- 0.5 * sum ( ( Y - Y_hat ) * ( Y - Y_hat ) ) / ( length ( Y ) ) ;
	retval <- error + regularization_penalty;
	return(retval);
}

#======== Construct Y_hat from X, W, regime details ==========================
GetYHatRegime <- function ( selected_X, Weights, model_regimes, model_regime_index, modelPmat, num_instances ) {
  start_index = 1;
  Y_hat = matrix( 0, num_instances, 1 );
  for( i in seq( 1 : model_regimes ) ) {
    end_index = start_index + model_regime_index[i] - 1;
    X_regime = selected_X[ , c(start_index : end_index) ];
    W_regime = Weights[c(start_index : end_index)];
    Y_regime = X_regime %*% W_regime;
    Y_regime_weighted = as.matrix( Y_regime * modelPmat[,i]);
    Y_hat = Y_hat + Y_regime_weighted;
    start_index = end_index + 1;
  }
  return (Y_hat);
}

#======= Initializing variables and reading arguments
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
max_model_size <- 10;
LIVE_BIN_DIR <- "/home/dvctrader/basetrade_install/bin";

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 4 ) {
  stop ("USAGE : <script> <regdata> <reg_output_file> <num_regimes> <regime_cutoff1> <regime_cutoff2> ... ");
}

regdata_file <- args[1];
whole_regdata <- read.table(args[1]);
train_regdata_num = as.integer(nrow(whole_regdata));
total_instances = nrow(whole_regdata);
regdata <- whole_regdata[c(1:train_regdata_num),];
regdata <- as.matrix(regdata);
#test_regdata <- whole_regdata[c( (train_regdata_num +1) : total_instances ),];
#test_regdata_size = total_instances - train_regdata_num;
#test_regdata <- as.matrix (test_regdata);
#dim(test_regdata);
t_regdata_file <- paste (regdata_file, "tmp", sep="_" );
reg_output_file <- args[2];
tmp_reg_output_file <- paste (reg_output_file,"tmp",sep="_");
tmp_reg_output_indicators <- paste(tmp_reg_output_file, "indicators", sep="_");
num_regimes <- as.numeric(args[3]);
if ( num_regimes < 2 ) {
 stop ("Number of regimes should atleast be 2 ");
}
if( length(args) < (3 + num_regimes ) )
{
 stop ("Number of params and num_params don't match ");   
}
regime_cutoffs <- c();

for ( i in c(1:(num_regimes) ))
{
	regime_cutoffs <- c(regime_cutoffs, as.numeric(args[3+i]));
}

desired_size <- 700000000; # 700 MB
file_size <- file.info(regdata_file)$size;
downsample_ratio <- 1;
sample_ratio <- 1;

if ( file_size > desired_size )
{
	print ("Downsizing due to large size");
  downsample_ratio <- ceiling ( file_size / desired_size );
  regdata <- regdata [ seq (1, nrow(regdata), by = downsample_ratio ), ];
}

X <- regdata[,3:ncol(regdata)];
Y <- regdata[,1];
#test_X <- test_regdata[,3:ncol(test_regdata)];
#test_Y <- test_regdata[,1];
CY <- whole_regdata[,2];
num_indicators <- dim(X)[2];
sdx <- rep(0,num_indicators);

#============= Computing dependencies ================
for ( i in c(1:num_indicators) )
{
  sdx[i] <- sd ( X[,i] );
  X[,i] <- X[,i]/sdx[i];
}
sdy <- sd(Y);
Y <-  Y / sdy;

regdata <- cbind (Y,X);

num_instances <- nrow ( X );
Pmat <- matrix (1 , num_instances, num_regimes );
#test_Pmat <- matrix (1 , test_regdata_size, num_regimes );


#============= Finding Prob Matrix for all events =====
if( num_regimes > 1 ) {
	Pmat <- GetProbsMatrix( 1, num_instances, num_regimes, CY, regime_cutoffs );
  #test_Pmat <- GetProbsMatrix (num_instances+1, nrow(whole_regdata),num_regimes, CY, regime_cutoffs );
}
#============ Selecting Indicators and assigning weights for Continuous Regime Model =====
regime_inds <- c();
indicators_included <- c();
GetSelectedIndicators(Pmat,1);
#cat("Continuous Regime Indicators Included ", indicators_included, "\n");
#head(X);
X_selected = X[,indicators_included];
Continuous_Regime = 1;
B <- rep (1, length(indicators_included) );
res <- optim(B, ObjectiveFunction, GradNumerical, method = "BFGS");
Weights <- res$par;
#cat( "Weights for continous mode: ", Weights, "\n");

#============= Computing variables for Continuous Regime Model in non-normalized environment ===
#test_X_selected <- test_X[,indicators_included];
Y_hat = GetYHatRegime( X_selected, Weights, num_regimes, regime_inds, Pmat, num_instances );
non_normalized_mse <- sqrt(sum((Y_hat - Y )^2)/num_instances);
#test_Y_hat <- GetYHatRegime(test_X_selected, Weights, num_regimes, regime_inds, test_Pmat, test_regdata_size );
#test_non_normalized_mse <- sqrt(sum((test_Y_hat - test_Y)^2)/test_regdata_size);
#cat("Non normalized train mse ", non_normalized_mse, " test mse: ", test_non_normalized_mse, "\n");
W <- Weights*sdy;
Y <- Y*sdy;
for( i in c( 1:length(indicators_included ) ) ) {
 W[i] <- W[i]/sdx[indicators_included[i]];
 X_selected[,i] <- X_selected[,i]*sdx[indicators_included[i]];
}
#cat(Pmat);
Y_hat = matrix(0,num_instances,1);
Y_hat = GetYHatRegime( X_selected, W, num_regimes, regime_inds, Pmat, num_instances );
#dim(test_X_selected);
#length(W);
sd_dep <- sdy;
sd_model <- sd(as.vector(Y_hat));
sd_residual <- sd(as.vector(Y-Y_hat));
Y = as.matrix(Y);
final_correlation <- cor(Y,Y_hat);
#cat( "Dim Yhat ", dim(Y_hat), "\n" );
#cat( "dim Y ", dim(Y), "\n" )
final_mse <- sqrt( sum( (Y_hat - Y ) ^ 2)/num_instances );
#cat("M2 ", mse(Y_hat,Y), "\n");
#cat("Final mse ", final_mse, "\n");

#test_Y_hat <- matrix(0, nrow(whole_regdata) - num_instances,1);
#test_Y_hat <- GetYHatRegime( test_X_selected, W, num_regimes, regime_inds, test_Pmat, test_regdata_size );
#test_final_mse <- sqrt(sum((test_Y-test_Y_hat)^ 2 )/length(test_Y));
r2_residual <<- sum ( ( Y_hat - Y ) ^ 2 );
r2_original <<- sum ( (Y - mean(Y))^ 2 );
r_squared <<- 1 - r2_residual / r2_original;

#=========== Writing relevant information in the file to use ===================
cat ( "CREGIMECUT ", num_regimes, file = reg_output_file );
for ( i in c(1:(num_regimes) ))
{
	cat( " ", regime_cutoffs[i], file = reg_output_file, append = TRUE );
}
cat("\n", file = reg_output_file, append = TRUE );
cat ( "OutConst 0 0", file = reg_output_file, append = TRUE );
cat ( "\n" , file= reg_output_file, append=TRUE );

ind_index = 1;
for( i in c(1:num_regimes))
{
	cat ( "Factor ", i , "\n", file=reg_output_file, append=TRUE );
	for(j in c(1:regime_inds[i]))
	{
		cat("OutCoeff ", indicators_included[ind_index], " ", W[ind_index], "\n", file=reg_output_file, sep=" ", append=TRUE );
		ind_index = ind_index + 1;
	}
}

cat ( "RSquared ", r_squared, "\n", file = reg_output_file, sep = " ", append =TRUE);
cat ( "Correlation ", final_correlation ,"\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "StdevDependent ", sd_dep, "\n", file = reg_output_file, sep = " ", append=TRUE );
cat ( "StdevResidual ", sd_residual, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "StdevModel ", sd_model, "\n", file = reg_output_file, sep = " ", append = TRUE );
cat ( "MSE ", final_mse ,"\n", file = reg_output_file, sep = " ", append = TRUE );
