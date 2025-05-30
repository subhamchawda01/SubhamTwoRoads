#! /usr/bin/env Rscript

ReduceIndicators <- function ()
{
	system (sprintf('~/basetrade/ModelScripts/call_slasso.pl %s %s %s 1>/dev/null ', regdata_file, max_model_size, tmp_reg_output_file ))
        regout_text <- readLines (tmp_reg_output_file);
        for ( i in c(1:length(regout_text)) )
        {
            t_regout_row <- unlist(strsplit(regout_text[i], "\\ "));
            indicators_included <<- c( indicators_included, as.numeric( t_regout_row[1] ) );
        }	
	X <<- X[,indicators_included];
}

GenerateQuantiledData <- function ()
{
	system ( sprintf ( 	'export NEW_GCC_LIB=/usr/local/lib ; 
				export NEW_GCC_LIB64=/usr/local/lib64; 
				export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH ; 
				export GCC_4_9_ROOT=/apps/gcc_versions/gcc-4_9_install ; 
				if [ -d $GCC_4_9_ROOT ] ; then 
					export PATH=$GCC_4_9_ROOT/bin:$PATH; 
					export LD_LIBRARY_PATH=$GCC_4_9_ROOT/lib64:$LD_LIBRARY_PATH ;
				fi;
			 ~/basetrade_install/bin/realign_to_quantiles %s %s %s',regdata_file, regdata_file_quantiled, num_quantiles) )
}

ComputeTestStatistics <- function ( alpha, beta ) {
        T <- X_test * t(replicate(nrow(X_test),alpha));
        H <- 1/(1 + exp(-T)) - 0.5;
        Y_hat_test <- H %*% beta;
	retVal <- sum ( ( Y_test - Y_hat_test ) * ( Y_test - Y_hat_test ) ) / ( length(Y_hat_test) * 2 );
}

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 6 ) {
        stop ("USAGE : <script> <regdata> <reg_output_file> <max_model_size> <num_iters> <ntree> <max_nodes> < optional if using quantiled_reg_data: num_quantiles > [testCV][max_allowed_error][statusfile]");
}

#suppressPackageStartupMessages( library ( "randomForest", lib.loc="/apps/R/R-2.15.2/library/" ) );
require("randomForest");

regdata_file <- args[1];
regdata_file_quantiled <- paste (regdata_file,"quantiled",sep="_");
regdata <- as.matrix(read.table(args[1]));
reg_output_file <- args[2];
tmp_reg_output_file <- paste (reg_output_file,"tmp",sep="_");
max_model_size <- as.numeric(args[3]);
num_iters <- as.numeric(args[4]);
num_trees <- as.numeric(args[5]);
max_nodes_per_tree <- as.numeric(args[6]);
indicators_included <- c();

if ( length(args) > 6 && as.numeric(args[7]) > 0 )
{
	num_quantiles = as.numeric(args[7]);
	GenerateQuantiledData();
	regdata <- as.matrix(read.table(regdata_file_quantiled));
	regdata_file <- regdata_file_quantiled;	
}

X <- regdata[,2:ncol(regdata)];
Y <- regdata[,1]

if ( ncol(X) > max_model_size )
{
	ReduceIndicators();
} else {
	indicators_included <- c(1:ncol(X));
}

N <- dim(X)[1]

#n_train <- floor(0.7*N);

#n_test <- N - n_train;

sampling_block = N/10
row_ids = 1:nrow(X)

X_train <- X[ row_ids%%sampling_block < 0.7*sampling_block, ];
Y_train <- Y[ row_ids%%sampling_block < 0.7*sampling_block ];

X_test <- X[ row_ids%%sampling_block >= 0.7*sampling_block, ];
Y_test <- Y[ row_ids%%sampling_block >= 0.7*sampling_block ];

initial_corrs <- cor(Y_train,X_train);
initial_corrs[is.na(initial_corrs)] <- 0

num_indicators <- dim(X)[2];

rf <- randomForest ( X_train, Y_train, X_test, Y_test, ntree=num_trees, maxnodes=max_nodes_per_tree, keep.forest = TRUE )
max_rf <- rf;
max_test_rsq <- rf$test$rsq[num_trees];

for ( i in c(1:num_iters) )
{
	rf <- randomForest ( X_train, Y_train, X_test, Y_test, ntree=num_trees, maxnodes=max_nodes_per_tree, keep.forest = TRUE )
	if ( max_test_rsq < rf$test$rsq[num_trees] )
	{
		max_rf <- rf
		max_test_rsq <- rf$test$rsq[num_trees]
	}
}


cat ( "MODELARGS ",num_trees," ", max_nodes_per_tree,"\n",file = reg_output_file);

for ( i in indicators_included ) {
	cat ("INDICATOR ",(i-1),"\n",file = reg_output_file, append = TRUE );
}

for ( i in c(1:num_trees) ){
	cat ("TREESTART ", i ,"\n", file = reg_output_file ,  append = TRUE );
	tr <- getTree (rf, k=i);
	
	for ( i in c(1:nrow(tr))){
		tr_left <- tr[i,1] - 1;
		tr_right <- tr[i,2] - 1;
		tr_splitvar <- tr[i,3] - 1;
		tr_point <- tr[i,4];
		
		if ( tr[i,5] == -1 ){
			tr_status = 'Y';
		} else {
			tr_status = 'N';
		}		
		
		tr_pred <- tr[i,6];

		cat ( "TREELINE ",tr_left," ",tr_right," ",tr_splitvar," ",tr_point," ",tr_status," ",tr_pred, file = reg_output_file, "\n" , append = TRUE );
	}
}
#for cross validation,pass -1 in the optional argument
cat ( "RSquared ",max_test_rsq,"\n",file = reg_output_file,append=TRUE);

if ( length(args) > 7 )
{
	testregdata <- as.matrix(read.table(args[8]));
	
	X_test <- testregdata[,2:ncol(testregdata)];
	Y_test <- testregdata[,1];
	X_test <- X_test[,indicators_included];	
	Ybar_test <- predict(max_rf,X_test);
	error <- sum ( ( Ybar_test - Y_test) ^ 2)/sum( (Y_test - mean(Y_test))^2);
	
	if (error < as.numeric(args[9]))
	{
		cat("1\n",file = args[10]);
	}
	else
	{
		cat("0\n",file = args[10]);
	}
	cat("test error ",error,"\n",file=args[10],append=TRUE);
}
