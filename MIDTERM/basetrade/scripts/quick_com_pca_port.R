#!/usr/bin/env Rscript

args <- commandArgs(trailingOnly = TRUE);
shortcode_file <- args[1];

numRowCmd <- paste("wc -l ",args[2], " | cut -d ' ' -f1 ");
numColCmd <- paste("head -n1 ", args[2], " | wc -w | cut -d ' ' -f1 ");

numRow <- as.numeric( system ( numRowCmd, intern=TRUE ) );
numCol <- as.numeric( system ( numColCmd, intern=TRUE ) );

data<-as.matrix ( read.table ( args[2], nrows=numRow, colClasses="numeric") ); 
data<- scale ( data ) 

outFile <- args[3];

fc <- file ( shortcode_file );
mylist <- strsplit(readLines(fc), " +");
close(fc);

for( i in 1:length(mylist)) { 
        shortcode <- mylist[[i]][1]; 
        colList <- lapply(mylist[[i]][2:length(mylist[[i]])], as.numeric) ;
        col <- c();
        for ( j in 1:length ( colList ) ) { col = c( col, colList[[j]][1] ); }

        pr <- prcomp ( data[,col] );
        eigen1 <- pr$rotation[,1];
        eigen2 <- pr$rotation[,2];
        eigen1 <- sign(eigen1[1]) * eigen1;
        eigen2 <- sign(eigen2[1]) * eigen2;
        if ( shortcode == "BRALL" || shortcode == "UEBE" || shortcode == "USALL2") {
          eigen1 <- -eigen1;
          eigen2 <- -eigen2;
        }
        sink(outFile, append=TRUE ) ;
        cat( "PORTFOLIO_STDEV", shortcode , attr(data,"scaled:scale")[col], "\n" )
        cat( "PORTFOLIO_EIGEN", shortcode, "1", pr$sdev[1]^2 / sum ( pr$sdev ^2 ), eigen1, "\n");
        cat( "PORTFOLIO_EIGEN", shortcode, "2", pr$sdev[2]^2 / sum ( pr$sdev ^2 ), eigen2, "\n");
        sink();
        gc();
}

