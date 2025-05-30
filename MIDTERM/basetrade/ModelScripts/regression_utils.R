# file regression_utils.R  
# author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
# Address:
#	 Suite 217, Level 2, Prestige Omega,
#	 No 104, EPIP Zone, Whitefield,
#	 Bangalore - 560066, India
#	 +91 80 4060 0717
#
##############################################################################################

# y <- predictors set
reducePredictors <- function ( y , x , select_function = "TOP" , max_predictors = 18 )
{

	# add selec_functions here, if implemented AbVal AbMu AbMe AbSh MuSh AbMuSiSt

	nrows <- dim ( y ) [ 1 ]
	ncols <- dim ( y ) [ 2 ]

	min_statistical_data_lenth <- 5000
	
	if ( max_predictors > ncols )
	{
		max_predictors = ncols
	}

	if ( nrows < min_statistical_data_lenth ) 
	     folds <- 1 
	else 
	     folds <-  floor ( nrows / min_statistical_data_lenth )

	# correlations <- matrix ( 0 , folds , ncols )
	correlations <- c()

	strength_ord <- c( 1:ncols )

	# Absolute Value
	if ( folds == 1 || select_function == "AbVal" )
	{
		correlations  <- cbind ( correlations , apply ( y , 2 , function ( z ) { cor ( z , x ) } ) )
		cor_aval <- abs ( correlations )
		strength_ord <- order ( cor_aval , decreasing = T , na.last = T )
		
	}
	else
	{
		for ( i in 1 : folds )
		{
			start_index <-  ( i - 1 ) * min_statistical_data_lenth + 1
			end_index <- start_index + min_statistical_data_lenth - 1
			correlations  <- cbind ( correlations , apply ( y [ start_index:end_index ,  ] , 2 , function ( z ) { cor ( z , x [ start_index:end_index  ]  ) } ) )
		}
	# Absolute Mean of Fold Values
		if ( select_function == "AbMu" )
		{
			cat ( "select function is Absoluste Mean \n" );
			cor_amu <- apply ( correlations , 1 , function ( z ) { abs ( mean ( z ) ) } )
			strength_ord <- order ( cor_amu , decreasing = T , na.last = T )
		}
	# Absolute Median of Fold Values
		else if ( select_function == "AbMe" )
		{
			cat ( "select function is Absoluste Median \n" );
			cor_amed <- apply ( correlations , 1 , function ( z ) { abs ( median ( z ) ) } )
			strength_ord <- order ( cor_amed , decreasing = T , na.last = T )

		}
	# Absolute Sharpe of Fold Values
		else if ( select_function == "AbSh" )
		{
			cat ( "select function is Absoluste Sharpe \n" );
			cor_amu <- apply ( correlations , 1 , function ( z ) { abs ( mean ( z ) ) } )
			cor_asd <- apply ( correlations , 1 , function ( z ) { abs ( sd ( z ) ) } )
			cor_ash <- cor_amu / cor_asd 
			strength_ord <- order ( cor_ash , decreasing = T , na.last = T )

		}
	# Mean * Sharpe of Fold Values
		else if ( select_function == "MuSh" )
		{
			cat ( "select function is Mean*Sharpe \n" );
			cor_amu <- apply ( correlations , 1 , function ( z ) { abs ( mean ( z ) ) } )
			cor_asd <- apply ( correlations , 1 , function ( z ) { abs ( sd ( z ) ) } )
			cor_amash <- cor_amu * ( cor_amu / cor_asd )
			strength_ord <- order ( cor_amash , decreasing = T , na.last = T )
		}
	# Absolute Mean * Sign Strength of Fold Values
		else if ( select_function == "AbMuSiSt" )
		{
			cat ( "select function is Absolute * Mean * SignStrength \n" );
			cor_amu <- apply ( correlations , 1 , function ( z ) { abs ( mean ( z ) ) } )
			cor_asr <- apply ( correlations , 1 , function ( z ) { l <- sum ( sign ( z ) == 1 ) / length ( z ) ; return ( max ( l , 1 - l ) ) } )
			cor_amu_asr <- cor_amu * cor_asr 
			strength_ord <- order ( cor_amu_asr , decreasing = T , na.last = T )
		}
	}

	return ( strength_ord [ 1:max_predictors ] )
   }