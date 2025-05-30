#!/usr/bin/env Rscript

HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");

REPO <- "basetrade";

Sys.setenv(LD_LIBRARY_PATH="/apps/gcc_versions/gcc-4_9_install/lib64/")

SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");
MODELSCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/ModelScripts/", sep="");
GENPERLLIB_DIR <- paste(HOME_DIR, "/", REPO, "_install/GenPerlLib/", sep="");
LIVE_BIN_DIR <- paste(HOME_DIR, "/LiveExec/bin/", sep="");
if ( ! file.exists(LIVE_BIN_DIR )) 
{ 
    LIVE_BIN_DIR <- paste(HOME_DIR,"/", REPO,"_install/bin/", sep="");
}

TRADELOG_DIR <- "/spare/local/logs/tradelogs/";


GetListOfDates <- function ( )
{
        script <- paste (SCRIPTS_DIR, "get_list_of_dates_for_shortcode.pl", sep="");
        dates <- system ( sprintf("%s %s %s %s 2>/dev/null",script, shortcode, start_date, num_days_lookback),intern=TRUE);
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


GetDataForRegression <- function ()
{
	t_dgen_outfile <- paste( work_dir, "t_dgen_outfile", sep="" );	
	t_regdata_outfile <- paste( work_dir, "t_regdata_outfile", sep="" );
	catted_regdata_filename <- paste( work_dir, "catted_regdata_outfile", sep="" );
	
	for ( trading_date in list_of_insample_days )	
	{	    
	    datagen_exec <- paste(LIVE_BIN_DIR, "datagen", sep="");
	    timed_data_to_reg_data_exec <- "~/basetrade/scripts/timed_data_to_reg_data.R"
	    cat ( datagen_exec, " ", ilist, " ", trading_date, " ", start_time, " ", end_time, " 22222 ", t_dgen_outfile, " ", dgen_msecs, " ", dgen_l1events, " ", dgen_trades ," 0\n");
	    system ( sprintf ( "%s %s %s %s %s %s %s %s %s %s %s 2>/dev/null", datagen_exec, ilist,  trading_date, start_time, end_time, 22222, t_dgen_outfile, dgen_msecs, dgen_l1events, dgen_trades, to_print_on_eco ) ); 

	    cat ( timed_data_to_reg_data_exec," ",shortcode," ",ilist," ", trading_date," ", t_dgen_outfile," ", upper_threshold," ",lower_threshold_ratio," 1000 ",t_regdata_outfile,"\n");
	    
	    system ( sprintf ( "%s %s %s %s %s %s %s %s %s 2>/dev/null", timed_data_to_reg_data_exec,shortcode,ilist, trading_date, t_dgen_outfile, upper_threshold,lower_threshold_ratio, 1000,t_regdata_outfile ) );	 	     
	    system ( sprintf ( "cat %s >> %s", t_regdata_outfile, catted_regdata_filename ) );

	}
	apply_dep_filter_script <- paste(MODELSCRIPTS_DIR, "apply_dep_filter.pl", sep="");

        system ( sprintf ( "%s %s %s %s %s %s", apply_dep_filter_script, shortcode, catted_regdata_filename, filter, filtered_regdata_filename, start_date ) );
#	system ( sprintf ( "rm -f %s %s %s", catted_regdata_filename, t_dgen_outfile, t_regdata_outfile ) );
}


args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 14 ) {
        stop ("USAGE : <script> <shortcode> <ilist> <start_date> <num_days_lookback> <start_time> <end_time> <dgen_msecs> <dgen_l1events> <dgen_trades> <to_print_on_eco> <upper_threshold> <lower_threshold_factor> <filter> <work_dir>\n");
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
upper_threshold <- as.numeric(args[11]);
lower_threshold_ratio <- args[12];
filter <- args[13];
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

GetDataForRegression ();
