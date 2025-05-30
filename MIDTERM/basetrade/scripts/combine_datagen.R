#!/usr/bin/env Rscript

HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");

REPO <- "basetrade";

SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");
MODELSCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/ModelScripts/", sep="");
LIVE_BIN_DIR <- paste(HOME_DIR, "/LiveExec/bin/", sep="");

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


CombineDatagen <- function ()
{
        t_dgen_outfile <- paste( work_dir, "t_dgen_outfile", sep="" );

        if(compute_sum_vars == 0)
        {
          t_dgen_outfile_sum_vars_ <- t_dgen_outfile ;
        }
        else
        {
          t_dgen_outfile_sum_vars_ <- paste("SUM_VARS@",t_dgen_outfile ,sep="" );
        }
        t_regdata_outfile <- paste( work_dir, "t_regdata_outfile", sep="" );
        catted_regdata_filename <- paste( work_dir, "catted_regdata_outfile", sep="" );
        combine_datagen_filename <- paste( work_dir, "combine_datagen_filename", sep="" );
        for ( trading_date in list_of_insample_days )
        {           
            datagen_exec <- paste(LIVE_BIN_DIR, "datagen", sep="");
            timed_data_to_reg_data_exec <- paste(LIVE_BIN_DIR, "timed_data_to_reg_data", sep="");
            cat ( datagen_exec, " ", ilist, " ", trading_date, " ", start_time, " ", end_time, " 22222 ", t_dgen_outfile, " ", dgen_msecs, " ", dgen_l1events, " ", dgen_trades, " 0\n");
            pred_counter_script <- paste(MODELSCRIPTS_DIR, "print_pred_counters_for_this_pred_algo.pl", sep="");
            system ( sprintf ( "%s %s %s %s %s %s %s %s %s %s %s 2>/dev/null", datagen_exec, ilist,  trading_date, start_time, end_time, 22222, t_dgen_outfile_sum_vars_ , dgen_msecs, dgen_l1events, dgen_trades, to_print_on_eco ) );            
                     
            system ( sprintf ( "cat %s >> %s", t_dgen_outfile_sum_vars_, combine_datagen_filename ) );

        }
}


args = commandArgs( trailingOnly=TRUE );

if ( length(args) < 14 ) {
        stop ("USAGE : <script> <shortcode> <ilist> <start_date> <num_days_lookback> <start_time> <end_time> <dgen_msecs> <dgen_l1events> <dgen_trades> <to_print_on_eco> <pred_duration> <pred_algo> <filter> <work_dir> <compute_sum_vars[0/1] = 0>\n");
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
pred_duration <- as.numeric(args[11]);
pred_algo <- args[12];
filter <- args[13];
work_dir <- args[14];
compute_sum_vars <- 0;

if(length(args) > 14)
{
  compute_sum_vars <- args[15];
}

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

CombineDatagen();



