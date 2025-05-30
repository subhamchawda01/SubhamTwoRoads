#!/usr/bin/env Rscript

HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");
REPO <- "basetrade";

SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");
MODELSCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/ModelScripts/", sep="");
GENPERLLIB_DIR <- paste(HOME_DIR, "/", REPO, "_install/GenPerlLib/", sep="");
LIVE_BIN_DIR <- paste(HOME_DIR, "/LiveExec/bin/", sep="");
args = commandArgs( trailingOnly=TRUE )
  if ( length(args) < 8 ) {
            stop ("USAGE : <script> <shortcode> <ilist> <start_date> <num_days_lookback> <start_time> <end_time> <pred_num_events> <work_directory>\n");
  }
dep_shortcode <- args[1];
ilist <- args[2];
start_date <- args[3];
num_days_lookback <- max(as.numeric(args[4]),5);
start_time <- args[5];
end_time <- args[6];
num_events <- max(as.numeric(args[7]),3);
WORK_DIR <- args[8] ;
system(paste('mkdir -p', WORK_DIR))
dgen_msecs <- 0
dgen_l1events <- 0
dgen_trades <- 0
to_print_on_eco <- 0
list_of_insample_days <- c();
list_of_outsample_days <- c();

script <- paste (SCRIPTS_DIR, "get_list_of_dates_for_shortcode.pl", sep="");
dates <- system ( sprintf("%s %s %s %s 2>/dev/null",script, dep_shortcode, start_date, num_days_lookback),intern=TRUE );
list_of_dates <- unlist(strsplit(dates, "\\ "));
for ( i in c(1:num_days_lookback) )
{         
     list_of_insample_days <<- append ( list_of_insample_days, list_of_dates[i] );            
}


#cat (WORK_DIR, "\n") ;

t_dgen_outfile <- paste( WORK_DIR, "/t_dgen_outfile", sep="" );
cated_regdata_filename <- paste( WORK_DIR, "/cated_regdata_outfile", sep="" );

system ( sprintf ( "rm -f %s", cated_regdata_filename)) ; 
for ( trading_date in list_of_insample_days )
{
      datagen_exec <- paste(LIVE_BIN_DIR, "datagen", sep="");
      timed_data_to_reg_data_exec <- paste(LIVE_BIN_DIR, "timed_data_to_reg_data", sep="");
#      cat ( datagen_exec, " ", ilist,  " ", trading_date, " ", start_time, " ", end_time, " 22222 ", t_dgen_outfile, " ", dgen_msecs, " ", dgen_l1events, " ", dgen_trades, " 1\n");
      timed_data_to_training_data_exec <- paste(MODELSCRIPTS_DIR,"timed_data_to_training_data.py", sep = "");
 

      system ( sprintf ( "%s %s %s %s %s %s %s %s %s %s %s 2>/dev/null;", datagen_exec, ilist,  trading_date, start_time, end_time, 22222, t_dgen_outfile, dgen_msecs, dgen_l1events, dgen_trades, to_print_on_eco) );
	system(sprintf("%s %s %s >> %s ;", timed_data_to_training_data_exec , t_dgen_outfile, num_events, cated_regdata_filename) );
}

system (sprintf("rm -f %s ;", t_dgen_outfile)) ; 






