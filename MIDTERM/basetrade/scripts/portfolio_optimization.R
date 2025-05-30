#! /usr/bin/env Rscript
args = commandArgs( trailingOnly=TRUE )
print(args)

library(fPortfolio)
#first step read the input data
ret_data = read.table( args[1] )
ret_data = t(ret_data)
Data = as.timeSeries( ret_data )
Spec = portfolioSpec()
#setType(Spec) = "CVaR"
##setType(Spec) = "MV"
Constraints = "LongOnly"
maxratioPortfolio(Data, Spec, Constraints)

##efficientPortfolio(Data, Spec, Constraints)
