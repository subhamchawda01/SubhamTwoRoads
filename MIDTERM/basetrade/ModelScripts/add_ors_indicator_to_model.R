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


TestPerformance <- function ( strat_filename, num_strats, list_of_dates )
{
	sum_pnl <- rep(0,num_strats);
	sum_vol <- rep(0,num_strats);

	avg_pnl <- rep(0,num_strats);
	avg_vol <- rep(0,num_strats);

	mkt_market_index <- as.numeric(system(sprintf('~/basetrade/scripts/get_market_model_for_shortcode.pl %s', shortcode ), intern = TRUE ) );

	for ( trading_date in list_of_dates )
	{	    
            sim_result_lines <- system (sprintf('~/basetrade_install/bin/sim_strategy SIM %s 22222 %s %s ADD_DBG_CODE -1 2>/dev/null',strat_filename,trading_date, mkt_model_index ), intern = TRUE );
	    for ( strat in c(1:num_strats) )
	    {
		sim_result_words <- unlist(strsplit(sim_result_lines[strat], "\\ " ) );
		sum_pnl[strat] <- sum_pnl[strat] + as.numeric(sim_result_words[2])  ;		
		sum_vol[strat] <- sum_vol[strat] + as.numeric(sim_result_words[3])  ;
		#cat ( trading_date, " ", sim_result_words,"\n");
		#cat ( trading_date, " Avg Pnl : ", sum_pnl[strat], " Avg Vol : ", sum_vol[strat], "\n" );
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

BuildNewModels <- function ()
{
	ilist_text <- readLines ( ilist );
	model_text <- readLines ( model_filename );
	
	for ( i in c(1:num_indicators) )
	{
		for ( j in c(1:length(ors_indicator_weights)) )
		{
			num_models <<- num_models + 1;
			t_new_model_filename <- paste (work_dir, "/tmp_model_", num_models,sep="");


			weight_new <- ors_indicator_weights;		
	
	
			for ( j in c(1:length(model_text) ) )
			{
				if ( j==1 )
				{
				    cat ( model_text[j],"\n",file=t_new_model_filename );
				} else {
				    cat ( model_text[j],"\n",file=t_new_model_filename, append=TRUE );			
				    if ( j == length (model_text) - 1)
				    {
				        cat ( t_ilist_row,"\n",file=t_new_model_filename, append=TRUE );
				    }
				}
			}				
		}	
	}
}

BuildCattedStratFile <- function()
{
	strat_text <- readLines ( strat_file );
	t_strat_row <- unlist ( strsplit( strat_text, "\\ " ) );
	
	for ( i in c(1:num_models) )
	{
		t_strat_row[4] <- paste ( work_dir, "/tmp_model_", i, sep="" );
		t_strat_row[length(t_strat_row)] <- as.numeric(t_strat_row[length(t_strat_row)]) + 1;
		if ( i==1 )
		{
		    cat ( t_strat_row,"\n", file=test_strat_filename );	
		}else {
		    cat ( t_strat_row,"\n", file=test_strat_filename, append=TRUE );
		}
	}	
}


args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 5 ) {
        stop ("USAGE : <script> <shortcode> <strat_file> <model_file> <ilist> <start_date> <num_days_lookback> <start_time> <end_time> <dgen_msecs> <dgen_l1events> <dgen_trades> <pred_duration> <pred_algo> <filter> \n");
}

num_models <- 0;
shortcode <- args[1];
strat_file <- args[2];
model_filename <- args[3];
ilist <- args[4];
start_date <- args[5];
num_days_lookback <- as.numeric(args[6]);
start_time <- as.numeric(args[7]);
end_time <- as.numeric(args[8]);
dgen_msecs <- as.numeric(args[9]);
dgen_l1events <- as.numeric(args[10]);
dgen_trades <- as.numeric(args[11]);
pred_duration <- as.numeric(args[12]);
pred_algo <- as.numeric(args[13]);
filter <- args[14];
work_dir <- paste("/spare/local/",system ("whoami",intern=TRUE), "/", shortcode, "/", floor(runif(1,100000,999999)),sep="");
system ( sprintf ("mkdir -p %s", work_dir) );
filtered_regdata_filename <- paste(work_dir, "/filtered_regdata_filename", sep="");
weights <- c();
best_indicator_index <- 0;
new_model_filename <- paste( work_dir, "/new_model_file",sep="" );
results_filename <- paste( work_dir, "/results", sep="" );

num_outsample_days <- 0.001 * num_days_lookback;

list_of_insample_days <- c();
list_of_outsample_days <- c(); 

max_indep_correlation <- 0.7

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

Y_hat_orig <- X_orig %*% weights;

Y_res <- Y - Y_hat_orig;

test_strat_filename <- paste( work_dir, "/test_strat_file", sep="" );

BuildNewModels ();

BuildCattedStratFile ();

cat ("Original Strategy File Insample Results\n", file = results_filename, append = TRUE );

TestPerformance ( strat_file, 1, list_of_insample_days );

cat ("Modified Strategy Files Insample Results\n", file = results_filename, append = TRUE );

TestPerformance  ( test_strat_filename, num_models, list_of_insample_days );

cat ( "Best Indicator Index ", best_indicator_index, "\n", file = results_filename, append = TRUE );

final_model_file <- paste ( work_dir, "/tmp_model_", best_indicator_index , sep="" );

system ( sprintf ("cp %s %s", final_model_file, new_model_filename ) );

#for ( i in c(1:num_models) )
#{
#	t_model_file <- paste ( work_dir, "/tmp_model_", i , sep="" );
#	system ( sprintf ( "rm -f %s", t_model_file ) );
#}

system ( sprintf ( "rm -f %s ", filtered_regdata_filename ) );
