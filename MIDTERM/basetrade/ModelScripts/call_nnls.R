#!/usr/bin/env Rscript


suppressMessages(require(data.table))
suppressMessages(require(VIF))
suppressMessages(require(nnls))
options(warn=-1)


### this script accepts regdata and orders indicator by correlation, takes top N indicators and fits non negative least squares
### using R package nnls

args <- commandArgs(trailingOnly = TRUE);
if ( length ( args ) < 2 )
{
  stop ( " Usage: <script> <regdata> <outputfile>\n " );
}



d <- as.matrix(read.table(args[1], sep=" "));

d_x <- (d[,3]);
d_y <- (d[,4:ncol(d)]);
cc <- nnls(as.matrix(d_y),d_x);
write.table (t(cc$x), args[2], col.names=F, row.names=F);
