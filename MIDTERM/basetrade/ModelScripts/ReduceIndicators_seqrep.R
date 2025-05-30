#!/usr/bin/env Rscript

library("leaps");
args <- commandArgs(trailingOnly = TRUE);

#regdata_file <- args[1];
regdata <- read.table(args[1]);
regdata <- as.matrix(regdata);
max_model_size = as.numeric( args[2] );
output_file = args[3];

if( max_model_size >= (  ncol(regdata) -1 ) )
{
 for ( i in 1:( ncol(regdata) -1 )  )
{

cat (  i-1,  file = output_file, sep= " ", append = TRUE );
cat ( "\n", file = output_file, append = TRUE ) ;


}

}else
{
X <- regdata[,2:ncol(regdata)];
Y <- regdata[,1]

indicators_included <- c();
fea_obj =  regsubsets( x = as.matrix( X ), y = Y, nbest = 1, nvmax = max_model_size, method = "seqrep" ) ;
 reg_col =   names(coef( fea_obj, max_model_size ))[-1] ;

for ( iii in 1:length(reg_col) )
{
 indicators_included[iii] = as.numeric ( strsplit( x = reg_col[iii], split = 'V',  ) [[1]] [2] ) - 2 ;
}


for ( i in 1:max_model_size )
{

cat (  indicators_included[i],  file = output_file, sep= " ", append = TRUE );
cat ( "\n", file = output_file, append = TRUE ) ;


}

}

