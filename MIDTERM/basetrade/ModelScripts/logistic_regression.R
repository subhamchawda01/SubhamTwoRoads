#!/usr/bin/env Rscript
#suppressPackageStartupMessages( library ( "numDeriv" , lib.loc="/apps/R/root/library/" ) );
.libPaths("/apps/R/root/library/")
suppressPackageStartupMessages( require('numDeriv') );

#################
#Algorithm - SOFTMAX regression

#Reduces the ilist to "max_model_size" using lasso
ReduceIndicators <- function ()
{
	system (sprintf('~/basetrade/ModelScripts/call_slasso.pl %s %s %s 1>/dev/null ', args[4], max_model_size, tmp_reg_output_file ))
        regout_text <- readLines (tmp_reg_output_file);
        for ( i in c(1:length(regout_text)) )
        {
            t_regout_row <- unlist(strsplit(regout_text[i], "\\ "));
            indicators_included <<- c( indicators_included, as.numeric( t_regout_row[1] ) );
        }	
	X <<- X[,indicators_included];
}


SOFTMAX <- function(B)
{
    print ("B :")
    print (B)
    dim(B) <- c(num_class, num_indicators)
#    print (dim(B));
    L <- X %*% t(B);    #N*num_ind X num_ind*num_class = N*num_class
    EXPL <- exp(L); #N*num_class
    SUMEXPL <- rowSums(EXPL);
    P <- EXPL/SUMEXPL;  #N*num_class
    cost <- (-1/N) * sum(CHECK*log((P+cost_concession)/(cost_concession + 1)));
    penalty <- lambda * sum (B*B);
    cat ("PENALTY : ", penalty, " COST : ", cost, "\n"); 
    retVal <- cost + penalty;
}

SOFTMAXGradAnalytical <- function ( B )
{
    dim(B) <- c(num_class, num_indicators)
#    print (dim(B));
    L <- X %*% t(B);    #N*num_class
    EXPL <- exp(L); #N*num_class
    SUMEXPL <- rowSums(EXPL);
    P <- EXPL/SUMEXPL;  #N*num_class
    PN <- (P+cost_concession)/(cost_concession + 1)
    costgrad <- (-1/N) * (t(t(X) %*% (CHECK - PN))); #t(num_ind*N X N*num_class) = num_class*num_ind
    penaltygrad <- 2*lambda*B;
    cat ("PENALTYGRAD : \n");
    print (penaltygrad);
    cat ("COSTGRAD : \n");
    print (costgrad);
    retVal <- costgrad + penaltygrad;
}

SOFTMAXGradNumerical <- function ( B ) 
{
	retVal <- grad (SOFTMAX,B,method="simple");
}

args = commandArgs( trailingOnly=TRUE )
print (length(args))
if ( length(args) < 4 ) {
        #stop ("USAGE : <script> <sampled_reg_data_> <mlogit_reg_data_> <max_model_size> <penalty[N,L0,LP,L1,L2]> <optional: p in (0,1) for LP>\n N = no regularization \n L0 = penalty on high L0 norm of coefficients i.e, number of non_zero indicators \n L1 = penalty on high L1 norm of coefficients \n L2 = penalty on high L2 norm of coefficients \n");
        stop ("USAGE : <script> <mlogit_reg_data_> <max_model_size> <regression_output_filename_> <reg_data_> [L2_penalty_coeff] [cost_concession] [use_numeric_grad(0|1)]\n")
}

mlogit_regdata_file <- args[1];
mlogit_regdata <- read.table(mlogit_regdata_file);
mlogit_regdata <- as.matrix(mlogit_regdata);
regdata_file <- args[4];
regdata <- read.table(regdata_file);
regdata <- as.matrix(regdata);
max_model_size <- as.numeric(args[2]);
reg_output_file <- args[3];
tmp_reg_output_file <- paste (reg_output_file,"tmp",sep="_");

X <- regdata[,2:ncol(regdata)];
Y <- mlogit_regdata[,1];


indicators_included <- c();
ReduceIndicators();

N <- dim(X)[1]
num_indicators <- dim(X)[2];
num_class <- 3
num_params <- (num_indicators)*num_class;
CHECK <- c((Y==0), (Y==1), (Y==2))*1
dim(CHECK) <- c(N, num_class)

sdx <- rep(0,num_indicators);
for ( i in c(1:num_indicators))
{
   sdx[i] <- sd ( X[,i] );
   X[,i] <- X[,i]/sdx[i];   	
}

X = c(X,rep(1,nrow(X)));
num_indicators <- num_indicators + 1;
dim(X) <- c(N, num_indicators);

B <- matrix(0, num_class, num_indicators);
lambda <- 0.01;
if(length(args)>=5)
{
    lambda <- as.numeric(args[5]);
}

cost_concession <- 0.0
if(length(args) >= 6)
{
    cost_concession <- as.numeric(args[6]);
}


use_num <- 0;
if(length(args)>=7)
{
    use_num <- as.numeric(args[7]);
}

if(use_num == 1) 
{
    res <<- optim(B, SOFTMAX, SOFTMAXGradNumerical, method = "BFGS");
} else 
{
res <<- optim(B, SOFTMAX, SOFTMAXGradAnalytical, method = "BFGS");
}

theta <- res$par;
cat ("theta\n");
print (theta);
cat ("resvalue\n");
print (res$value);

dim(theta) <- c(num_class, num_indicators)
print (theta);
L <- X %*% t(theta);    #N*num_class
EXPL <- exp(L); #N*num_class
SUMEXPL <- rowSums(EXPL);
P <- EXPL/SUMEXPL;  #N*num_class
Y_hat <- apply(P, 1, which.max) - 1;
cat ("class distribution in original data : ", sum((Y==0)*1), sum((Y==1)*1), sum((Y==2)*1),"\n")
cat ("class distribution in predictions : ", sum((Y_hat==0)*1), sum((Y_hat==1)*1), sum((Y_hat==2)*1),"\n");
cat ("class distribution of correct predictions  : ", sum(((Y_hat==0)*1) * ((Y_hat == Y)*1)), sum(((Y_hat==1)*1) * ((Y_hat == Y)*1)), sum(((Y_hat==2)*1) * ((Y_hat == Y)*1)),"\n");
correct_pred_ <- sum((Y_hat == Y)*1);
cat ("%age of correct_pred_ : ", correct_pred_/N, "\n");
cat ("correlations of class predictions : ", cor(P[,1],regdata[,1]), cor(P[,2],regdata[,1]), cor(P[,3],regdata[,1]),"\n")

cat ( "OutConst", theta[1,num_indicators],theta[2,num_indicators],theta[3,num_indicators], file = reg_output_file );
cat ( "\n" , file= reg_output_file, append=TRUE );

for ( i in c(1:(num_indicators-1)) )
{
        cat ( "OutCoeff", (indicators_included[i] -1), theta[1,i],theta[2,i],theta[3,i], file = reg_output_file, sep= " ", append = TRUE );
        cat ( "\n", file = reg_output_file, append = TRUE ) ;
}
