#!/usr/bin/env Rscript
#suppressPackageStartupMessages( library ( "numDeriv" , lib.loc="/apps/R/root/library/" ) );

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 3  ) {
        stop ("USAGE : <script> <regdata_filename> <ilist_signs_filename> <new_regdata_filename>\n");
}

regdata_filename <- args[1];
regdata <- read.table(args[1]);
regdata <- as.matrix(regdata);
num_rows_ <- dim ( regdata ) [1] 
num_cols_ <- dim ( regdata ) [2];
regdata <- as.numeric ( regdata ); #when numbers are too larget, R loads them as string, need to convert to numeric
regdata <- matrix ( regdata, nrow=num_rows_, ncol=num_cols_ ) ;

new_regdata_filename <- args[3];

ilist_signs_filename_ <- args[2];
ilist_signs <- as.numeric(read.table(ilist_signs_filename));
ilist_signs <- as.vector(ilist_sings);

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

X_train <- X;
Y_train <- Y;

initial_corrs <- cor(Y_train,X_train);
initial_corrs[is.na(initial_corrs)] <- 0

indicators_included <- c();

for ( i in c(1:num_indicators) )
{
  if ( initial_corrs[i] * ilist_signs[i] >= 0 )
  {
    indicators_included <- c(indicators_included, i );
  }
}

X <- X_train[,indicators_included];

for ( i in c(1:nrow(X)) )
{
  cat (Y[i], X[i,], "\n", file = new_regdata_filename, append=TRUE );
}
