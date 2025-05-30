#!/usr/bin/env Rscript

## using to read list of dates file if exists
library(data.table)

HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");

REPO <- "basetrade";

Sys.setenv(LD_LIBRARY_PATH="/apps/gcc_versions/gcc-4_9_install/lib64/")

SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");
MODELSCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/ModelScripts/", sep="");
LIVE_BIN_DIR <- paste(HOME_DIR, "/LiveExec/bin/", sep="");
if ( ! file.exists(LIVE_BIN_DIR )) 
{ 
    LIVE_BIN_DIR <- paste(HOME_DIR,"/", REPO,"_install/bin/", sep="");
}

TRADELOG_DIR <- "/spare/local/logs/tradelogs/";


GetListOfDates <- function ( )
{
        script <- paste (SCRIPTS_DIR, "get_list_of_dates_for_shortcode.pl", sep="");
	if (!as.logical(gsub("1","T",gsub("-1","F",as.character(regexpr("[^0-9]",start_date,))))) ){
                dates <- system ( sprintf("%s %s %s %s 2>/dev/null",script, shortcode, start_date, num_days_lookback),intern=TRUE);
        } else {
                dates <- system (sprintf("cat %s 2>/dev/null",start_date),intern=TRUE );
        }

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

GetNumLinesFile <- function (file)
{
    console.output = system( sprintf("wc -l %s", file), intern = TRUE);
      console.array = strsplit (console.output, " ");
        num.lines = as.integer (console.array[[1]][1]);
          return (num.lines);
}

GetDataForRegression <- function ()
{
	t_dgen_outfile <- paste( work_dir, "t_dgen_outfile", sep="" );	
	t_regdata_outfile <- paste( work_dir, "t_regdata_outfile", sep="" );
	catted_regdata_filename <- paste( work_dir, "catted_regdata_outfile", sep="" );
  day_flag = 3;	
	for ( trading_date in list_of_insample_days )	
	{	    
      day_flag = day_flag - 1;
	    datagen_exec <- paste(LIVE_BIN_DIR, "datagen", sep="");
	    timed_data_to_reg_data_exec <- paste(LIVE_BIN_DIR, "timed_data_to_reg_data", sep="");
	    cat ( datagen_exec, " ", ilist, " ", trading_date, " ", start_time, " ", end_time, " 22222 ", t_dgen_outfile, " ", dgen_msecs, " ", dgen_l1events, " ", dgen_trades, " 0\n");
            pred_counter_script <- paste(MODELSCRIPTS_DIR, "print_pred_counters_for_this_pred_algo.pl", sep="");
	    system ( sprintf ( "%s %s %s %s %s %s %s %s %s %s %s 2>/dev/null", datagen_exec, ilist,  trading_date, start_time, end_time, 22222, t_dgen_outfile, dgen_msecs, dgen_l1events, dgen_trades, to_print_on_eco ) ); 
	    if (GetNumLinesFile(t_dgen_outfile) == 0 && day_flag == 0 ) {
        cat("Error: Datagen not created")
        stop("");
      }
      for ( pred_duration in pred_duration_list ) {
	    	for( pred_algo in pred_algo_list ) {
			t_dgen_outfile_multiple <- paste( t_dgen_outfile, "_", pred_duration, "_", pred_algo, sep="" ); 
			t_regdata_outfile_multiple <- paste( t_regdata_outfile, "_", pred_duration, "_", pred_algo, sep="" );
			catted_regdata_filename_multiple <- paste( catted_regdata_filename, "_", pred_duration, "_", pred_algo, sep="" );
			system ( sprintf( 'cp %s %s', t_dgen_outfile, t_dgen_outfile_multiple ) );
		    	pred_counter <- system ( sprintf ('perl %s %s %s %s %s 2>/dev/null' ,pred_counter_script , shortcode, pred_duration, pred_algo, t_dgen_outfile_multiple, start_time, end_time ), intern=TRUE) ;
		    	system ( sprintf ( "%s %s %s %s %s %s 2>/dev/null", timed_data_to_reg_data_exec, ilist, t_dgen_outfile_multiple, pred_counter, pred_algo, t_regdata_outfile_multiple ) );	   
     
		 	system ( sprintf ( "cat %s >> %s", t_regdata_outfile_multiple, catted_regdata_filename_multiple ) );
                        # deleting these files so we can run multiple ilists to the same directory, helps in consolidating reg data for a product
                        #cmd_2 <- sprintf ( "rm -f %s %s %s", t_dgen_outfile_multiple, t_regdata_outfile_multiple, t_dgen_outfile )
                        #print( cmd_2 )
                        #system( cmd_2 )

	    	}
	    }

	}
	for ( pred_duration in pred_duration_list )
	{
		for( pred_algo in pred_algo_list )
		{
			for( filter in filter_list )
			{
				catted_regdata_filename_multiple <- paste( catted_regdata_filename, "_", pred_duration, "_", pred_algo, sep="" );
                                # adding ilist so we can used multiple ilists into same directory
				filtered_regdata_filename_multiple <- paste( filtered_regdata_filename, "_", pred_duration, "_", pred_algo, "_", filter, "_", basename(ilist), sep="" );
				apply_dep_filter_script <- paste(MODELSCRIPTS_DIR, "apply_dep_filter.pl", sep="");
			        system ( sprintf ( "%s %s %s %s %s %s", apply_dep_filter_script, shortcode, catted_regdata_filename_multiple, filter, filtered_regdata_filename_multiple, start_date ) )
                
                        

			}
                        # deleting these files so we can run multiple ilists to the same directory, helps in consolidating reg data for a product
                        cmd_1 <- sprintf ( "rm -f %s", catted_regdata_filename_multiple)
                        print( cmd_1 )
                        system( cmd_1 )

		}
	}



}


args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 14 ) {
        stop ("USAGE : <script> <shortcode> <ilist> <start_date> <num_days_lookback> <start_time> <end_time> <dgen_msecs> <dgen_l1events> <dgen_trades> <to_print_on_eco> <pred_duration> <pred_algo> <filter> <work_dir> <file_with_list_of_dates>\n");
}



num_models <- 0;
shortcode <- args[1];
ilist <- args[2];
start_date <- args[3];
num_days_lookback <- as.numeric(args[4]);
start_time <- args[5];
end_time <- args[6];
dgen_msecs <- as.numeric(args[7]);
dgen_l1events <- args[8];
dgen_trades <- args[9];
to_print_on_eco <- as.numeric(args[10]);
pred_duration_list <- toString(args[11]);
pred_algo_list <- toString(args[12]);
filter_list <- toString(args[13]);
work_dir <- args[14];
#work_dir <- paste("/spare/local/",system ("whoami",intern=TRUE), "/", shortcode, "/", floor(runif(1,100000,999999)),sep="");
system ( sprintf ("mkdir -p %s", work_dir) );
filtered_regdata_filename <- paste(work_dir, "filtered_regdata_filename", sep="");

num_outsample_days <- 0.001 * num_days_lookback;

list_of_insample_days <- c();
list_of_outsample_days <- c(); 

max_models_to_keep <- 5 ;
best_cost_vec <- c() ;
best_index_vec <- c() ;

max_indep_correlation <- 0.7;

GetListOfDates();

## if we want data only for bad days, read from this file
if ( length(args) > 14 ) {
date.table <- fread(args[15])
only_these_dates <- date.table[[1]]
list_of_insample_days <- list_of_insample_days[list_of_insample_days %in% only_these_dates]
}


pred_duration_list <- strsplit(pred_duration_list, ",")[[1]];
pred_algo_list <- strsplit(pred_algo_list, ",")[[1]];
filter_list <- strsplit(filter_list, ",")[[1]];
GetDataForRegression ();
