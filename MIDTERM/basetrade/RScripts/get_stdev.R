#!/usr/bin/env Rscript
args<-commandArgs(TRUE);

catted_file = args[1];
data = read.table(catted_file);

for ( j in 1:ncol(data ) )
#for ( j in 1:1) 
{
  cat((sd(data[,j])));
  cat( " " );
  
}
cat ( "\n" );
