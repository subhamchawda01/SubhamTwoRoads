#! /usr/bin/env Rscript
args = commandArgs( trailingOnly=TRUE )
print(args)

#first step read the input data
full_data = read.table( args[1] )
x = nrow ( full_data )
y = ncol ( full_data )

#cols will be labelled V1, ..., Vn with indep being V1
#make optional perhaps .. cap outliers .. based on %tage rather 
#than stdev to account for different var profiles

i = 1
while ( i <= y )
{
  sorted_temp = sort( full_data[ , i ] )
  min_ = sorted_temp[ floor( x*0.0005 ) ]
  max_ = sorted_temp[ ceiling( x*0.9995 ) ]
  full_data[ , i ] = pmax( min_, pmin( max_, full_data[ , i ] ) )
  i = i+1
}

library( earth )

#data set is filtered .. now get simple earth 
simple_lin_reg_out_filename = paste( args[1], "simple_linear_out", sep="." )
sink( simple_lin_reg_out_filename )
simple_lin_res = earth( V1 ~ ., degree = 1, data = full_data, linpreds = TRUE, nprune = 10, thresh = 0.0003, nk = 25 )
summary( simple_lin_res )
sink( )

#now get degree 1 with hinge
hinged_lin_reg_out_filename = paste( args[1], "hinged_linear_out", sep="." )
sink( hinged_lin_reg_out_filename )
hinged_lin_res = earth( V1 ~ ., degree = 1, data = full_data, nprune = 10, nk = 25, thresh = 0.0003 )
summary( hinged_lin_res )
sink( )

#now get degree 2 with hinge
hinged_deg2_reg_out_filename = paste( args[1], "hinged_deg2_out", sep="." )
sink( hinged_deg2_reg_out_filename )
hinged_deg2_res = earth ( V1 ~ ., degree = 2, data = full_data, nk = 15, nprune = 10, thresh = 0.0007 )
summary( hinged_deg2_res )
sink( )

#------------------------------------------------------------------------#
