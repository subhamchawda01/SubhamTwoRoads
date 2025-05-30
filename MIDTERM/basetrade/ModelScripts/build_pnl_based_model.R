#!/usr/bin/env Rscript

GetListOfDates <- function ( )
{
        dates <- system ( sprintf("~/basetrade/scripts/get_list_of_dates_for_shortcode.pl %s %s %s 2>/dev/null", shortcode, start_date, num_days_lookback),intern=TRUE );
        t_list_of_dates <- unlist(strsplit(dates, "\\ "));
	
	for ( i in c(1:num_days_lookback) )
	{
		list_of_dates <<- append( list_of_dates, t_list_of_dates[i] );
	}
}

WriteOutModel <- function ( ) 
{
	t_model_size <- as.numeric ( system ( sprintf ( "cat %s | grep \"INDICATOR\" | wc -l", model_filename ), intern=TRUE ) );
	system ( sprintf ( "cat %s | awk '{if(NR==%s){$2=%s} print $0}' > %s", model_filename, t_model_size + 1, weight_new, new_model_filename ) );
}

AddNextIndicator <- function ()
{
	num_models_created <- 0;
	indicators_considered <- c();
	t_weights <- c();
	for ( i in c(1:num_indicators) )
	{
		if ( i %in% indicators_included )
		{
			next;
		}
		else
		{
			t_X <- X[,i];
			
			for ( j in ncol(OX) )
			{
				t_X <- t_X - (sum(t_X * OX[,j]))/(sum(OX[,j]*OX[,j]))*OX[,j];
			}
	
			if ( cor(t_X,Y_res) * initial_corrs[i] < 0 )
			{
			  	next;
			}	
		
			max_indep_corr_hit <- FALSE;
			for ( j in ncol(OX) )
			{
				if ( cor(t_X, OX[,j]) > max_indep_correlation )
				{					
					max_indep_corr_hit <- TRUE;
					break;
				}
			}						
			
			OX <<- cbind(OX,t_X);

			if ( max_indep_corr_hit )
			{
				next;
			}
			indicators_considered <- append ( indicators_considered, i );
			t_weight <- (sum(Y_res*t_X))/(sum(t_X*t_X));
			t_weights <- append ( t_weights, t_weight );	
			num_models_created <- num_models_created + 1;
		        t_new_model_filename <- paste ( work_dir, "/tmp_model_", num_models_created, sep="" );		
			BuildModelFile ( i, t_weight, t_new_model_filename );
		}		
	}
	if ( num_models_created <= 0 )
	{
		retVal <- -1;
	}
	else
	{
		BuildCattedStratFiles ( num_models_created );	
		best_index <- GetBestIndicatorIndex ( test_strat_filename, indicators_considered );
		if ( best_index < 0 )
		{
			t_strat_filebase <- paste ( work_dir, "/tmp_strat*",sep="");
			t_model_filebase <- paste ( work_dir, "/tmp_model*",sep="");
	#		system ( sprintf ( "rm -f %s %s", t_strat_filebase, t_model_filebase ) ); 
			retVal <- -1;	
		} else 
		{
			best_model <- paste ( work_dir, "/tmp_model_", best_index, sep = "" )
			system ( sprintf ( "cp %s %s", best_model, current_model ) );
			indicators_included <<- append ( indicators_included, indicators_considered [ best_index ]) ;
			current_model_weights <<- append ( current_model_weights, t_weights [ best_index ] );
			current_model_size <<- current_model_size + 1;
			retVal <- 1;
		}
	}
}

GetBestIndicatorIndex <- function ( test_catted_strat_filebase, indicators_considered )
{
	num_strats <- length(indicators_considered);
        sum_pnl <- rep(0,num_strats);
        sum_vol <- rep(0,num_strats);

        avg_pnl <- rep(0,num_strats);
        avg_vol <- rep(0,num_strats);

	num_strat_files <- ceiling ( num_strats / max_sim_strats_per_run ) ;

        for ( trading_date in list_of_dates )
        {
            for ( i in c(1:num_strat_files) )
            {
                t_strat_filename <- paste ( test_catted_strat_filebase, "_", current_model_size, "_", i, sep="" );
                offset <- (i-1)*max_sim_strats_per_run;
                num_remaining <- num_strats - offset;
                if ( num_remaining < max_sim_strats_per_run )
                {
                        num_in_the_lot <- num_remaining;
                } else {
                        num_in_the_lot <- max_sim_strats_per_run;
                }

                sim_result_lines <- system (sprintf('~/basetrade_install/bin/sim_strategy SIM %s 22222 %s ADD_DBG_CODE -1 2>/dev/null',t_strat_filename,trading_date), intern = TRUE );
                for ( strat in c(1:num_in_the_lot) )
                {
                     sim_result_words <- unlist(strsplit(sim_result_lines[ strat ], "\\ " ));
                     sum_pnl[ strat + offset ] <- sum_pnl[ strat + offset ] + as.numeric(sim_result_words[2]);
                     sum_vol[ strat + offset ] <- sum_vol[ strat + offset ] + as.numeric(sim_result_words[3]);
                }
            }
        }

        for ( i in c(1:num_strats) )
        {
                avg_pnl[i] <- sum_pnl[i]/length(list_of_dates);
                avg_vol[i] <- sum_vol[i]/length(list_of_dates);
        }

        max_score_pnl <- -100000;
        max_vol <- 0;
        max_score <- -1000000000;
	max_score_index <- -1;
	max_score_vol <- -1;

	for ( i in c(1:num_strats) )
	{
		if ( avg_pnl[i] > 0 )
		{
			t_score <- avg_pnl[i] * sqrt ( avg_vol[i] );
		} else 
		{
			t_score <- avg_pnl[i] / sqrt ( avg_vol[i] );
		}

		if ( max_score < t_score && avg_vol[i] > volume_cutoff )
		{
			max_score <- t_score;
			max_score_index <- i;
			max_score_pnl <- avg_pnl[i];
			max_score_vol <- avg_vol[i];
		}
	}		
	
	cat ( "Current Model Size : ", current_model_size, "\nMax Score : ", max_score, "\nMax pnl :", max_score_pnl, "\nMax vol :", max_score_vol, "\n" )

# Adding new indicators till min_model_size...after that if adding new indicator doesn't add to pnl then dont add it.	
	if ( current_model_size >= min_model_size && max_score < prev_best_score )
	{
		return (-1);
	} else
	{
		prev_best_score <<- max_score ;		
		return (max_score_index ) ;	
	}
}

BuildModelFile <- function ( indicator_index, weight, t_model_filename )
{
	ilist_text <- readLines ( ilist );
	
	system ( sprintf("cat %s | head -%d > %s", current_model,current_model_size + 3, t_model_filename ) );
	for ( i in c(1:num_indicators) )
	{
		t_ilist_row <- unlist(strsplit(ilist_text[i+3], "\\ "));
        	if (  i == indicator_index )
		{
			t_ilist_row [2] <- weight;
			cat ( t_ilist_row,"\nINDICATOREND",file=t_model_filename, append=TRUE );
		}        
	}
}

BuildCattedStratFiles <- function( num_models )
{
	strat_text <- readLines ( strat_filename );
	t_strat_row <- unlist ( strsplit( strat_text, "\\ " ) );
	
	for ( i in c(1:num_models) )
	{
		test_strat_file_index <- ceiling ( i / max_sim_strats_per_run );
		test_strat_file <- paste ( test_strat_filename, "_", current_model_size , "_", test_strat_file_index, sep="" ); 
		t_strat_row[4] <- paste ( work_dir, "/tmp_model_", i, sep="" );
		t_strat_row[length(t_strat_row)] <- as.numeric(t_strat_row[length(t_strat_row)]) + 1;
		cat ( t_strat_row,"\n", file=test_strat_file, append=TRUE );
	}	
}


args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 11 ) {
        stop ("USAGE : <script> <shortcode> <regdata_file> <strat_filename> <ilist> <start_date> <num_days_lookback> <max_indep_correlation> <min_model_size> <max_model_size> <volume_cutoff> <work_dir>\n");
}

shortcode <- args[1];
regdata_file <- args[2];
strat_filename <- args[3];
ilist <- args[4];
start_date <- args[5];
num_days_lookback <- as.numeric(args[6]);
max_indep_correlation <- as.numeric(args[7]);
min_model_size <- as.numeric(args[8]);
max_model_size <- as.numeric(args[9]);
volume_cutoff <- as.numeric(args[10]);
work_dir <- paste( args[11],"/",shortcode,"/",floor(runif(1,100000,999999)),sep="");

max_sim_strats_per_run <- 50 

model_filename <- system ( sprintf ( "cat %s | awk '{print $4}'", strat_filename ), intern = TRUE );

system ( sprintf ("mkdir -p %s", work_dir) );
regdata <- as.matrix(read.table(regdata_file));

best_indicator_index <- 0;
results_filename <- paste( work_dir, "/results", sep="" );

list_of_dates <- c(); 

GetListOfDates();

num_indicators <- as.numeric(system( sprintf( "cat %s | grep \"INDICATOR \" | wc -l", ilist ), intern=TRUE ) );

Y <- regdata[,1];
X <- regdata[,2:ncol(regdata)];

initial_corrs <- cor(Y,X);
initial_corrs[is.na(initial_corrs)] <- 0

current_model_size <- 0;

current_model <- model_filename;

system ( sprintf("cat %s | head -n3 > %s", ilist, current_model ) );

current_model_weights <- c();

indicators_included <- c();

best_score <- -1000000001;
prev_best_score <- -1000000000;

move_forward <- 1;

Y_res <- Y;

OX <- c();

test_strat_filename <- paste ( work_dir, "/test_strat", sep = "" );

while ( current_model_size < max_model_size && move_forward == 1 )
{
	cat ( "Adding new indicator\n");
	move_forward <- AddNextIndicator ( );
} 
