#!/usr/bin/env Rscript

HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");

REPO <- "basetrade";

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
        dates <- system ( sprintf("%s %s %s %s 2>/dev/null",script, shortcode, start_date, num_days_lookback),intern=TRUE );
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


GetData <- function ()
{
	t_dgen_outfile <- paste( work_dir, "t_dgen_outfile", sep="" );	
	
	for ( trading_date in list_of_insample_days )	
	{	    
	    datagen_exec <- paste(LIVE_BIN_DIR, "datagen", sep="");
	    timed_data_to_reg_data_exec <- paste(LIVE_BIN_DIR, "timed_data_to_reg_data", sep="");
	    cat ( datagen_exec, " ", ilist, " ", trading_date, " ", start_time, " ", end_time, " 22222 ", t_dgen_outfile, " ", dgen_msecs, " ", dgen_l1events, " ", dgen_trades, " 1\n");
            system ( sprintf ( "%s %s %s %s %s %s %s %s %s %s %s 2>/dev/null", datagen_exec, ilist,  trading_date, start_time, end_time, 22222, t_dgen_outfile, dgen_msecs, dgen_l1events, dgen_trades, to_print_on_eco ) );

	    system ( sprintf ( "awk -vnf=%s \'{ print nf, $5, $6; }\' %s >> %s", trading_date, t_dgen_outfile, catted_dgen_filename ) );

	}
	system ( sprintf ( "rm -f %s ", t_dgen_outfile ) );
}


args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 11 ) {
        stop ("USAGE : <script> <shortcode> <ilist> <start_date> <num_days_lookback> <start_time> <end_time> <dgen_msecs> <dgen_l1events> <dgen_trades> <to_print_on_eco> <work_dir>\n");
}

num_models <- 0;
shortcode <- args[1];
ilist <- args[2];
start_date <- args[3];
num_days_lookback <- as.numeric(args[4]);
start_time <- args[5];
end_time <- args[6];
dgen_msecs <- as.numeric(args[7]);
dgen_l1events <- as.numeric(args[8]);
dgen_trades <- as.numeric(args[9]);
to_print_on_eco <- as.numeric(args[10]);
work_dir <- args[11];
#work_dir <- paste("/spare/local/",system ("whoami",intern=TRUE), "/", shortcode, "/", floor(runif(1,100000,999999)),sep="");
system ( sprintf ("mkdir -p %s", work_dir) );
catted_dgen_filename <- paste( work_dir, "catted_gendata_outfile", sep="" );
print ( cat ( catted_dgen_filename, "\n" ) );

num_outsample_days <- 0.001 * num_days_lookback;

list_of_insample_days <- c();
list_of_outsample_days <- c(); 

GetListOfDates();

GetData ();
