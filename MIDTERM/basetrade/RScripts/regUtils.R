#######################################################################
# This file contains functions helpful for data regression studies
#######################################################################

generateYYHatsFile <- function (regdatafile, regoutputfile, yhatfile) {
	       	   
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

	       if ( max ( idx +1 ) > dim ( regdata )[2] ) {
	       	  
		  print ( "number of coefficents are greater than  number of independents\n" ) ;

	       }
	       

	       # 0 -> coeff start from 0 #

	       idxVector <- vector ( "numeric" , dim ( regdata )[2]  ) ;
	       idxVector [ idx + 2 ] <- coeff ;
	       yhats <-  as.matrix ( regdata ) %*% idxVector  + const ;

	       retVal <-  ( cbind ( regdata$V1, yhats ) );
	       colnames ( retVal )  <- c( "Ys", "YHats" );

	       write.table ( retVal, file = yhatfile, sep = ' ' );
}


computeMultipleStats <- function ( regdatafile ) {
		     data <-  read.delim ( regdatafile , sep = " ", header = F ) ;

		     q_beta <- matrix ( 0, length (  seq ( 0.05 , 1 , 0.05 ) ) , dim ( data )[2] - 1 ) ;
		     q_rho <- matrix ( 0, length (  seq ( 0.05 , 1 , 0.05 ) ) , dim ( data )[2] - 1 ) ;

		     q_mu <- matrix ( 0, length (  seq ( 0.05 , 1 , 0.05 ) ) , dim ( data )[2] - 1 ) ;
		     q_sig <- matrix ( 0, length (  seq ( 0.05 , 1 , 0.05 ) ) , dim ( data )[2] - 1 ) ;
		     
		     dep_idx <- 1 
		     indep_idx <- 2

		     for ( indep_idx in 2 : dim ( data ) [ 2 ] ) {

			quantile_based <- quantile ( data [ ,indep_idx ] , seq ( 0.1 , 1 , 0.05 ) ) ;

		     	for ( i in 1 : length ( quantile_based ) ) {	        

			    if ( i == 1 ) { 

			       range_idx <- which ( data [ , indep_idx ]  <= quantile_based [ i ] ) ;

			    } else {
			     
			       range_idx <- which ( data [ , indep_idx ]  <= quantile_based [ i ] & data [ , indep_idx ] > quantile_based [ i - 1 ] ) ;
			       
			    }
										
			    q_beta [ i , indep_idx - 1 ] <- cov ( data [ range_idx , dep_idx ] , data [ range_idx , indep_idx ] ) / var ( data [ range_idx , indep_idx ] ) ;
			    q_rho [ i , indep_idx - 1 ] <- cor ( data [ range_idx, dep_idx ] , data [ range_idx , indep_idx ] ) ;
			    q_mu [ i , indep_idx - 1 ] <- mean ( data [ range_idx , dep_idx ] ) ;
			    q_sig [ i , indep_idx - 1 ] <- sd ( data [ range_idx , dep_idx ] ) ;
			}

}


