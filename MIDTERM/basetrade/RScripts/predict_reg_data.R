#! /usr/bin/env Rscript
args = commandArgs( trailingOnly=TRUE )
#print(args)

GenerateYHatFile <- function (regdatafile, regoutputfile, yhatfile) {
	       	   
	       regdata <- read.delim ( file = regdatafile, header = F, sep = ' ' ) ;
	       regoutput <- read.delim ( file = regoutputfile, header = F, sep = ' ' ) ;

	       if ( is.null ( regdata ) || dim ( regdata )[1] == 0 || dim ( regdata )[2] == 0  ) {

	       	  print ( "regdata is empty \n" );

		  return ( NULL );

	       }

	       if ( is.null ( regoutput ) || dim ( regoutput )[1] == 0 || dim ( regoutput )[2] == 0  ) {

	       	  print ( "regoutput is empty \n" );

		  return ( NULL );

	       }

		  
	       coeff <- subset ( regoutput, regoutput$V1 == "OutCoeff" )$V3 ;
	       idx <- subset ( regoutput, regoutput$V1 == "OutCoeff" )$V2 ;

	       const <- subset ( regoutput, regoutput$V1 == "OutConst" )$V3 ;

	       if ( max ( idx + 1 ) > dim ( regdata )[2] ) {
	       	  
		  print ( "number of coefficents are greater than  number of independents\n" ) ;

	       }
	       

	       # 0 -> constant beta's coeff start from 1 #

	       idxVector <- vector ( "numeric" , dim ( regdata )[2]  ) ;
	       idxVector [ ( idx + 2 ) ] <- coeff ;
	       yhats <-  as.matrix ( regdata ) %*% idxVector  + const ;

	       retVal <-  ( cbind ( regdata$V1, yhats ) );
	       colnames ( retVal )  <- c( "Ys", "YHats" );

	       write.table ( retVal, file = yhatfile, sep = ' ', row.names=F,col.names=F );
}
if ( length ( args ) >= 3 )
{
GenerateYHatFile ( args[1], args[2], args[3] );
}
