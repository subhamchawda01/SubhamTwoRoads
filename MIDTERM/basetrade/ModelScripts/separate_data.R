#!/usr/bin/env Rscript
#suppressPackageStartupMessages( library(glmnet), lib.loc="/apps/R/R-2.15.2/library/" );
#.libPaths("/apps/R/R-2.15.2/library/")
suppressPackageStartupMessages( require(glmnet))

args <- commandArgs(trailingOnly = TRUE);
if ( length ( args)  < 6 )
{
  stop ( "USAGE: <script> <common_ilist> <reg_data_file> <total_prods> <num_common_inds> <num_template_inds_> <outfiles_base> " ) ; 
} 
model_name_ <- args [1] ;
data <- read.table(args[2]);
data <- as.matrix(data);  
total_prods_ <- as.numeric ( args [3] );
common_indocators_ <- as.numeric ( args [4] ) ;
template_indicators_ <- as.numeric(args [5] ) ;
base_out_filename_ <- args [6];
num_prices_ <- 2 ;
if ( length ( args ) >= 7 ) { num_prices_ <- as.numeric ( args [ 7 ] ) ; }  
for ( i in cbind ( 1:total_prods_ ) )
{
  price_start_ <-  num_prices_* ( i -1 ) + 5  ;
  price_end_ <- num_prices_ * ( i -1 ) +  6 ;
  if (num_prices_==1)
  {
    price_start_ <- num_prices_* ( i -1 ) + 5  ;
    price_end_ <- num_prices_ * ( i -1 ) + 5 ;
  }
  else
  {
#==2 
    price_start_ <-  num_prices_* ( i -1 ) + 5  ;
    price_end_ <- num_prices_ * ( i -1 ) +  6 ;
  }
  cols <- c(1, 2,  price_start_, price_end_ , (4 + num_prices_*total_prods_ + 1 ):(4+num_prices_*total_prods_+common_indocators_ ),
      ( 4 + num_prices_ * total_prods_ + common_indocators_ + ( i - 1 )   * template_indicators_ + 1 ) :( 4 + num_prices_ * total_prods_ + common_indocators_ + i * template_indicators_   ) );
  this_data_ <- data [, cols];
  out_filename_ <- paste ( base_out_filename_ , "_", toString ( i ), sep = "") ;
  colum <- 4+common_indocators_ + template_indicators_ ;
  write.table ( this_data_, file = out_filename_,row.names=F,col.names=F) ; 
}
