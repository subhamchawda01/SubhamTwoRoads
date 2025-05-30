#! /usr/bin/env Rscript
args = commandArgs( trailingOnly=TRUE )
print(args)

library( slam , lib.loc="/home/apps/R/R-3.0.1/library/" );
library( Rglpk , lib.loc="/home/apps/R/R-3.0.1/library/" );
#first step read the input data
obj = read.table( args[1] )
mat = read.table( args[2] )
dir = read.table( args[3] , colClasses='character' )
rhs = read.table( args[4] )
max = TRUE

Rglpk_solve_LP(obj, mat, dir, rhs, max = max)
