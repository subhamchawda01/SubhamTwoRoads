#!/usr/bin/env Rscript

GetListOfDates <- function ( )
{
        dates <- system ( sprintf("~/basetrade/scripts/get_list_of_dates_for_shortcode.pl %s %s %s 2>/dev/null", shortcode, start_date, num_days_lookback),intern=TRUE );
        list_of_dates <- unlist(strsplit(dates, "\\ "));
	
	for ( i in c(1:num_days_lookback) )
	{
		if ( i <= num_outsample_days ) {
			list_of_outsample_days <<- append( list_of_outsample_days, list_of_dates[i] );
		} else {
			list_of_insample_days <<- append ( list_of_insample_days, list_of_dates[i] );
		}
	}
}


GetOriginalModelCoeffs <- function ()
{
	model_text <- readLines ( model_filename );
        for ( i in c(1:length(model_text)) )
        {
            if ( grepl(pattern="INDICATOR ", x = model_text[i] ) )
            {
                t_model_row <- unlist(strsplit(model_text[i], "\\ "));
                t_weight <- as.numeric(t_model_row[2] );
                weights <<- c(weights,t_weight);
            }
        }
}

TestPerformanceOriginal <- function ( strat_filename, list_of_dates )
{

	sum_pnl <- 0;
	sum_vol <- 0;
	
        avg_pnl <- 0;
        avg_vol <- 0;

        for ( trading_date in list_of_dates )
        {
            sim_result_line <- system (sprintf('~/basetrade_install/bin/sim_strategy SIM %s 22222 %s ADD_DBG_CODE -1 2>/dev/null',strat_filename,trading_date), intern = TRUE );
            sim_result_words <- unlist(strsplit(sim_result_line, "\\ " ) );
            sum_pnl <- sum_pnl + as.numeric(sim_result_words[2]);
            sum_vol <- sum_vol + as.numeric(sim_result_words[3]);
        }

        avg_pnl <- sum_pnl/length(list_of_dates);
        avg_vol <- sum_vol/length(list_of_dates);

        if ( avg_pnl > 0 )
        {
           t_score <- avg_pnl * sqrt ( avg_vol );
        } else
        {
           t_score <- avg_pnl / sqrt ( avg_vol );
        }
        cat ( "For Original Strategy ", " Avg Pnl : ",floor(avg_pnl), " Avg Vol : ", floor(avg_vol), " Avg Score : ", t_score, "\n", file = results_filename, append=TRUE );
}

TestPerformance <- function ( strat_filename, num_strats, list_of_dates )
{
	sum_pnl <- rep(0,num_strats);
	sum_vol <- rep(0,num_strats);

	avg_pnl <- rep(0,num_strats);
	avg_vol <- rep(0,num_strats);

	for ( trading_date in list_of_dates )
	{       
	    for ( i in c(1:num_strat_files) )
	    {
		t_strat_filename <- paste ( test_strat_filename, "_", i, sep="" );
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
		
	max_pnl <- -100000;
	max_vol <- 0;
	max_score <- -1000000000;
	
	max_index <- 0;

	for ( i in c(1:num_strats) )
	{
	    if ( avg_pnl[i] > 0 ) 
	    {
		t_score <- avg_pnl[i] * sqrt ( avg_vol[i] );		
	    } else 
	    {
		t_score <- avg_pnl[i] / sqrt ( avg_vol[i] );
	    }

  	    if ( avg_vol[i] <= 1 )
	    {	    
		t_score <- -1000000001;
	    }

	    cat ( "For Strategy ",i, " Avg Pnl : ",floor(avg_pnl[i]), " Avg Vol : ", floor(avg_vol[i]), " Avg Score : ", t_score, "\n", file = results_filename, append=TRUE );
	    if ( t_score > max_score )
	    {
		max_score <- t_score;
		max_pnl <- avg_pnl[i];
		max_vol <- avg_vol[i];
		max_index <- i; 
	    }
	} 			
	best_indicator_index <<- max_index;
}

WriteOutModel <- function ( ) 
{
	t_model_size <- as.numeric ( system ( sprintf ( "cat %s | grep \"INDICATOR\" | wc -l", model_filename ), intern=TRUE ) );
	system ( sprintf ( "cat %s | awk '{if(NR==%s){$2=%s} print $0}' > %s", model_filename, t_model_size + 1, weight_new, new_model_filename ) );
}


BuildNewModels <- function ( index )
{
	ilist_text <- readLines ( ilist );
	model_text <- readLines ( model_filename );

        t_X_orig <- X_orig[, -index ] ;
        t_Y_hat_orig <- t_X_orig %*% weights[-index] ;
        t_Y_res <- Y - t_Y_hat_orig ; 
	
	for ( i in c(1:num_indicators) )
	{
		t_x <- X_new[,i];
		num_models <<- num_models + 1;
		t_new_model_filename <- paste (work_dir, "/tmp_model_", num_models,sep="");
		indep_corrs <- cor ( t_x, t_X_orig );

		indep_corrs [ which(is.na(indep_corrs)) ] <- 0;

		if ( sum (abs(indep_corrs) > max_indep_correlation ) >=1 )
		{
		    num_models <<- num_models - 1;
		    next;
		}

		res_corr <- cor( t_Y_res, t_x );
		if( is.na(res_corr) ) {
			res_corr <- 0;
		}

		dep_corr <- cor ( Y, t_x );
		
		if( is.na(dep_corr) ) {
			dep_corr <- 0;
		}

		if ( res_corr * dep_corr < 0 )
		{
		    num_models <<- num_models - 1;
		    next;
		}

		orig_model_correlation <- cor( Y, t_Y_hat_orig );

		weight_new <- sum ( t_Y_res * t_x ) / sum ( t_x * t_x );		
	
		t_Y_hat <- t_Y_hat_orig + t_x * weight_new ;

		mse_orig <- sqrt(sum((Y - t_Y_hat_orig ) * ( Y - t_Y_hat_orig ))/length(t_Y_hat_orig));
		mse_final <- sqrt(sum((Y - t_Y_hat) * (Y - t_Y_hat))/length(t_Y_hat));

		t_ilist_row <- unlist(strsplit(ilist_text[i+3], "\\ "));
		t_ilist_row[2]  <- weight_new;
	
		for ( j in c(1:length(model_text) ) )
		{
			if ( j==1 )
			{
			    cat ( model_text[j],"\n",file=t_new_model_filename );
			} else if ( j != index + 3 ) {
			    cat ( model_text[j],"\n",file=t_new_model_filename, append=TRUE );			
			    if ( j == length (model_text) - 1)
			    {
			        cat ( t_ilist_row,"\n",file=t_new_model_filename, append=TRUE );
			    }
			}
		}				
	
		cat ("For index ", index , " and indicator " , i , " mse : ", mse_orig, " ", mse_final, "\n", file = results_filename, append = TRUE );	
	}	
}

BuildCattedStratFile <- function( )
{
	strat_text <- readLines ( strat_file );
	t_strat_row <- unlist ( strsplit( strat_text, "\\ " ) );
	
	for ( i in c(1:num_models) )
	{
		test_strat_file_index <- ceiling ( i / max_sim_strats_per_run );
		test_strat_file <- paste ( test_strat_filename, "_", test_strat_file_index, sep="" ); 
		t_strat_row[4] <- paste ( work_dir, "/tmp_model_", i, sep="" );
		t_strat_row[length(t_strat_row)] <- as.numeric(t_strat_row[length(t_strat_row)]) + 1;
		cat ( t_strat_row,"\n", file=test_strat_file, append=TRUE );
	}	
}

GetDataForRegression <- function ()
{
	t_dgen_outfile <- paste( work_dir, "/t_dgen_outfile", sep="" );	
	t_regdata_outfile <- paste( work_dir, "/t_regdata_outfile", sep="" );
	catted_regdata_filename <- paste( work_dir, "/catted_regdata_outfile", sep="" );
	
	for ( trading_date in list_of_insample_days )	
	{	    
	    system ( sprintf ( "~/basetrade_install/bin/datagen %s %s %s %s %s %s %s %s %s %s 2>/dev/null", model_plus_ilist_file,  trading_date, start_time, end_time, 22222, t_dgen_outfile, dgen_msecs, dgen_l1events, dgen_trades, 1 ) ); 
	    
	    system ( sprintf ( "~/basetrade_install/bin/timed_data_to_reg_data %s %s %s %s %s 2>/dev/null", model_plus_ilist_file, t_dgen_outfile, pred_duration, pred_algo, t_regdata_outfile ) );	   
	   	     
	    system ( sprintf ( "cat %s >> %s", t_regdata_outfile, catted_regdata_filename ) );

	}

apply_dep_filter_script <- paste(MODELSCRIPTS_DIR, "apply_dep_filter.pl", sep="");

system ( sprintf ( "%s %s %s %s %s %s", apply_dep_filter_script, shortcode, catted_regdata_filename, filter, filtered_regdata_filename, start_date ) );

system ( sprintf ( "rm -f %s %s %s", catted_regdata_filename, t_dgen_outfile, t_regdata_outfile ) );
}


args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 5 ) {
        stop ("USAGE : <script> <shortcode> <strat_file> <ilist> <start_date> <num_days_lookback> <start_time> <end_time> <dgen_msecs> <dgen_l1events> <dgen_trades> <pred_duration> <pred_algo> <filter> \n");
}

num_models <- 0;
shortcode <- args[1];
strat_file <- args[2];
ilist <- args[3];
start_date <- args[4];
num_days_lookback <- as.numeric(args[5]);
start_time <- as.numeric(args[6]);
end_time <- as.numeric(args[7]);
dgen_msecs <- as.numeric(args[8]);
dgen_l1events <- as.numeric(args[9]);
dgen_trades <- as.numeric(args[10]);
pred_duration <- as.numeric(args[11]);
pred_algo <- as.numeric(args[12]);
filter <- args[13];

max_sim_strats_per_run <- 50 

work_dir <- paste("/spare/local/",system ("whoami",intern=TRUE), "/", shortcode, "/", floor(runif(1,100000,999999)),sep="");

model_filename <- system ( sprintf ( "cat %s | awk '{print $4}'", strat_file ), intern = TRUE );

system ( sprintf ("mkdir -p %s", work_dir) );
filtered_regdata_filename <- paste(work_dir, "/filtered_regdata_filename", sep="");
weights <- c();
best_indicator_index <- 0;
new_model_filename <- paste( work_dir, "/new_model_file",sep="" );
results_filename <- paste( work_dir, "/results", sep="" );

num_outsample_days <- 0.3 * num_days_lookback;

list_of_insample_days <- c();
list_of_outsample_days <- c(); 

max_indep_correlation <- 0.8

GetListOfDates();
GetOriginalModelCoeffs();

model_plus_ilist_file <- paste( work_dir, "/model_plus_ilist_file", sep="" );

temp_model_file <- paste( work_dir, "/tmp_model_file_tmp", sep="" );

system ( sprintf ( "cp %s %s", model_filename, temp_model_file ) );

system ( sprintf ( "cat %s | grep -v INDICATOREND > %s", temp_model_file, model_plus_ilist_file ) );

system ( sprintf ( "cat %s | grep \"INDICATOR \" >> %s", ilist, model_plus_ilist_file ) );

system ( sprintf ( "echo INDICATOREND >> %s", model_plus_ilist_file ) );

num_model_file_indicators <- as.numeric(system( sprintf( "cat %s | grep \"INDICATOR \" | wc -l", model_filename ), intern=TRUE ) );

cat ( "num_model_file_indicators ", num_model_file_indicators, "\n" );

system ( sprintf( "~/basetrade/scripts/remove_dup_indicators_from_ilist.pl %s ", model_plus_ilist_file ) );

num_model_plus_ilist_indicators <-  as.numeric(system( sprintf( "cat %s | grep \"INDICATOR \" | wc -l", model_plus_ilist_file ), intern=TRUE ) );

cat ( "num_model_plus_ilist_indicators ", num_model_plus_ilist_indicators , "\n" );

num_indicators <- num_model_plus_ilist_indicators - num_model_file_indicators;

cat ( "num_indicators ", num_indicators, "\n" );

GetDataForRegression ();
regdata <- as.matrix(read.table(filtered_regdata_filename));

Y <- regdata[,1];
X_orig <- regdata[,2:(ncol(regdata) - num_indicators)];

X_new <- regdata[,( ncol(regdata) - num_indicators + 1 ): ncol(regdata) ];

for ( i in c(1:num_model_file_indicators) )
{
	BuildNewModels ( i );
}

test_strat_filename <- paste( work_dir, "/test_strat_file", sep="" );

BuildCattedStratFile ();

num_strat_files <- ceiling ( num_models/max_sim_strats_per_run);

cat ("Original Strategy File Insample Results\n", file = results_filename, append = TRUE );

TestPerformanceOriginal ( strat_file, list_of_insample_days );

cat ("Original Strategy File Outsample Results\n", file = results_filename, append = TRUE );

TestPerformanceOriginal ( strat_file, list_of_outsample_days );

cat ("Modified Strategy Files Insample Results\n", file = results_filename, append = TRUE );

TestPerformance  ( test_strat_filename, num_models, list_of_insample_days );

cat ("Modified Strategy Files Outsample Results\n", file = results_filename, append = TRUE );

TestPerformance  ( test_strat_filename, num_models, list_of_outsample_days );

cat ( "Best Indicator Index ", best_indicator_index, "\n", file = results_filename, append = TRUE );

final_model_file <- paste ( work_dir, "/tmp_model_", best_indicator_index , sep="" );

system ( sprintf ("cp %s %s", final_model_file, new_model_filename ) );

#for ( i in c(1:num_models) )
#{
#	t_model_file <- paste ( work_dir, "/tmp_model_", i , sep="" );
#	system ( sprintf ( "rm -f %s", t_model_file ) );
#}

system ( sprintf ( "rm -f %s ", filtered_regdata_filename ) );
