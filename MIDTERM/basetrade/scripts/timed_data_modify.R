#!/usr/bin/env Rscript

HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");

REPO <- "basetrade";


args = commandArgs( trailingOnly=TRUE );

if ( length(args) < 2)
{
  stop("USAGE: <script> <filename> <flag> \n");
}

filename=args[1];
flag=args[2];

data<-read.table(filename); data<-as.matrix(data);

if(flag==2){

 data <- data[,-(3:4),drop=FALSE]

}
if(flag==1){

 data <- data[,-(3:4), drop=FALSE]
 data_start <- data[,c(1:3)]
 data_end  <- data[, c(3:dim(data)[2] ) ]
 data <- cbind(data_start,data_end)
}




write.table(data,filename,row.names=F,col.names=F);





