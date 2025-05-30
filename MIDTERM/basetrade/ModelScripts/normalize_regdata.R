#!/usr/bin/env Rscript


args <- commandArgs(trailingOnly = TRUE);

if ( length ( args ) < 1 )
{
  stop ( " Usage:  <reg_data_file> \n " ) ;
}

reg_data_file = args[1];

normalize_reg_data = function( reg_data_file ){
  reg_data = read.table(reg_data_file);
  stdev_val = as.numeric( reg_data[,ncol(reg_data)] );
  mean = mean(stdev_val);
  sd = sd(stdev_val);

#  upper_cap = mean + 1.5*sd ;
  lower_cap = max( 0.1*mean ); # mean - 0.5*sd );
  
#  stdev_val = min( stdev_val, upper_cap );
  stdev_val = max( stdev_val, lower_cap ) ;

  reg_data = reg_data/stdev_val;
  reg_data = reg_data[,-ncol(reg_data)];
  write.table(reg_data,file = reg_data_file,append = FALSE,row.names = FALSE,col.names = FALSE );
  
}

normalize_reg_data ( reg_data_file );
