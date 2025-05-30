#!/usr/bin/env Rscript

args = commandArgs( trailingOnly=TRUE );
if ( length(args) < 1) 
{
  stop("USAGE: <script> <filename> col_indices_pairs=(1 2)\nExample <script> datafile 1 2\n");
}
data<-read.table(args[1]); data<-as.matrix(data); 
col_ind_pairs = c(1,2);
if ( length(args) >= 3 )
{
  col_ind_pairs = c();
  col_ind_pairs <- as.numeric(args[2:length(args)])
}

tot_cols = dim(data)[2]
for ( i in seq(2, length(col_ind_pairs), by=2 ) )
{
  idx_x <- col_ind_pairs[i-1];
  idx_y <- col_ind_pairs[i];
  if ( idx_x <= tot_cols && idx_y <= tot_cols )
  {
    cat(sprintf ( "%.4f ", cor(as.numeric(data[,idx_x]), as.numeric(data[,idx_y])) ))
  }
  else
  {
    cat ( "0.0 " )
  }
}

