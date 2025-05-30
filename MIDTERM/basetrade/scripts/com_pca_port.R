#!/usr/bin/env Rscript

args <- commandArgs(trailingOnly = TRUE);
shortcode <- args[1];
data<-as.matrix ( read.table ( args[2] ) ) 
data<- scale ( data[,3:ncol(data)] ) 

pr <- prcomp ( data )
eigen1 <- pr$rotation[,1];
eigen2 <- pr$rotation[,2];
eigen1 <- sign(eigen1[1]) * eigen1;
eigen2 <- sign(eigen2[1]) * eigen2;
if ( shortcode == "BRALL" || shortcode == "UEBE" || shortcode == "USALL2" ) {
  eigen1 <- -eigen1;
  eigen2 <- -eigen2;
}

cat( "PORTFOLIO_STDEV", shortcode , attr(data,"scaled:scale"), "\n" ) 
cat( "PORTFOLIO_EIGEN", shortcode, "1", pr$sdev[1]^2 / sum ( pr$sdev ^2 ), eigen1, "\n");
cat( "PORTFOLIO_EIGEN", shortcode, "2", pr$sdev[2]^2 / sum ( pr$sdev ^2 ), eigen2, "\n");
