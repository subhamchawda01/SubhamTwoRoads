#!/usr/bin/env Rscript

library("leaps");

args <- commandArgs(trailingOnly = TRUE);

if ( length ( args ) < 3 )
{
  stop ( " Usage: <script> <regdata> <max_ind> <regout> \n " ) ;
}

data <- read.table(args[1]);
max_ind = as.numeric(args[2]);
output_file = args[3];
#data <- as.matrix(data);
#print ( output_file ); 

x <- data[,2:ncol(data)];
y <- data[,1];

if( max_ind >= ncol(x) )
{
  reg_data = data;
}else
{
fea_obj =  regsubsets( x=as.matrix(x), y = y, nbest = 1, nvmax = max_ind, method = "seqrep" ) ;
reg_col =  c("V1", names(coef(fea_obj, max_ind))[-1] );
reg_data = data[reg_col];
}
lm = lm(formula = V1~.+0, data = reg_data );
coef = coef(lm) ;
names = names(coef);

cat ( "OutConst",0, 0, file = output_file, sep= " ", append = TRUE );
cat ( "\n", file = output_file, append = TRUE ) ;

s = summary(lm); 

for ( i in 1:length( coef ) )
{
  if ( coef[i] != 0 )
  {
    index =  as.numeric ( strsplit( x = names[i], split = 'V',  ) [[1]] [2] ) ;
    cor = cor( reg_data[,1], reg_data[,i+1] ); 
    t_stat = s$coefficients[i,3]; 
    cat ( "OutCoeff", (index - 2 ), coef[i],"InitCorrelation", cor, "Tstat", t_stat, file = output_file, sep= " ", append = TRUE );
    cat ( "\n", file = output_file, append = TRUE ) ;
  }
  
}


 cat ( "RSquared", s$r.squared, file = output_file, sep= " ", append = TRUE );
 cat ( "\n", file = output_file, append = TRUE ) ;
 
 model_cor = cor( reg_data[,1], as.matrix(reg_data[,-1])%*% coef ); 
 cat ( "Correlation", model_cor, file = output_file, sep= " ", append = TRUE );
 cat ( "\n", file = output_file, append = TRUE ) ;

