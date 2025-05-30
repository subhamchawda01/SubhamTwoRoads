#!/usr/bin/env Rscript

get_moore_penrose_inv_matrix = function( num_outrights, weights ){
  
  weights = weights/sum(weights);
  
  num_spreads = num_outrights*(num_outrights-1)/2;
  
  num_products = num_outrights + num_spreads;
  
   
  
  pairs = data.frame();
  
  for( i in 1:(num_outrights-1) ){
    
    for( j in (i+1):num_outrights){
      pairs = rbind(pairs,c(i,j));
    }    
  }
  
  spread_pos = c();
  
  for( i in 1:nrow(pairs)){
    
     x = pairs[i,1];
     y = pairs[i,2];
    spread_pos[i] = num_outrights + ( (x-1)*(num_outrights - (x/2)) ) + (y-x);
  }
  
  keys_df = cbind(pairs, spread_pos);
  
   A = matrix(data = rep(0, num_spreads*num_products),nrow = num_spreads,ncol = num_products) ;
   
   for( i in 1:nrow(keys_df) ){
     keys = as.numeric(keys_df[i,]);
     A[i,keys[1]] = -1;
     A[i,keys[2]] = 1;
     A[i,keys[3]] = -1;
   }
  
  for( i in 1:ncol(A)){
    A[,i] = A[,i]/weights[i];
  }
    
 
  moore_penrose_matrix = -t(A)%*%solve(A%*%t(A));
  
  for( i in 1:nrow(moore_penrose_matrix)){
    cat(moore_penrose_matrix [i,]);
    cat("\n")
  }
  
  
}

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 2  ) {
        stop ("USAGE : <script> <num_ourights> <weights_file>  \n");
}

num_outrights = as.numeric( args[1]) ; 
weights = as.numeric( read.table(args[2]) [,1] ) ;
get_moore_penrose_inv_matrix( num_outrights, weights ) 

