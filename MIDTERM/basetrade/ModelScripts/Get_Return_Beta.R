
HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");
WORK_DIR <- paste('/spare/local/', USER,"/ReturnBeta/", Sys.Date(), sample(1:1000,1), sep = "")
system(paste('mkdir -p', WORK_DIR))
REPO <- "basetrade";

SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");
MODELSCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/ModelScripts/", sep="");
GENPERLLIB_DIR <- paste(HOME_DIR, "/", REPO, "_install/GenPerlLib/", sep="");
LIVE_BIN_DIR <- paste(HOME_DIR, "/LiveExec/bin/", sep="");
args = commandArgs( trailingOnly=TRUE )
  if ( length(args) < 7 ) {
            stop ("USAGE : <script> <indep_shortcode> <dep_shortcode> <start_date> <num_days_lookback> <start_time> <end_time> <duration> \n");
  }

indep_shortcode <- args[1];
dep_shortcode <- args[2];
start_date <- args[3];
num_days_lookback <- max(as.numeric(args[4]),5);
start_time <- args[5];
end_time <- args[6];
duration <- args[7];
dgen_msecs <- 1000
dgen_l1events <- 100
dgen_trades <- 100
to_print_on_eco <- 1
list_of_insample_days <- c();
list_of_outsample_days <- c();

ilist = paste(WORK_DIR, '/temp_ilist', sep = "")
sink(ilist)
cat(paste('MODELINIT DEPBASE ', dep_shortcode , ' MktSizeWPrice MktSizeWPrice', sep = ""))
cat('\nMODELMATH LINEAR CHANGE')
cat('\nINDICATORSTART')
cat('\nINDICATOR 1.00 SimplePriceType ', indep_shortcode ,' MktSizeWPrice', sep = "")
cat('\nINDICATOR 1.00 SimplePriceType ', dep_shortcode ,' MktSizeWPrice', sep = "")
cat('\nINDICATOR 1.00 SimpleTrend ', indep_shortcode ," ", duration, ' MktSizeWPrice', sep = "")
cat('\nINDICATOR 1.00 SimpleTrend ', dep_shortcode ,' ', duration,' MktSizeWPrice', sep = "")
cat('\nINDICATOREND')
sink()

script <- paste (SCRIPTS_DIR, "get_list_of_dates_for_shortcode.pl", sep="");
dates <- system ( sprintf("%s %s %s %s 2>/dev/null",script, dep_shortcode, start_date, num_days_lookback),intern=TRUE );
list_of_dates <- unlist(strsplit(dates, "\\ "));
for ( i in c(1:num_days_lookback) )
{         
     list_of_insample_days <<- append ( list_of_insample_days, list_of_dates[i] );            
}


t_dgen_outfile <- paste( WORK_DIR, "/t_dgen_outfile", sep="" );
cated_dgen_outfile <- paste( WORK_DIR, "/cated_dgen_outfile", sep="" );

system ( sprintf ( "rm -f %s", cated_dgen_outfile))
for ( trading_date in list_of_insample_days )
{
      datagen_exec <- paste(LIVE_BIN_DIR, "datagen", sep="");
      timed_data_to_reg_data_exec <- paste(LIVE_BIN_DIR, "timed_data_to_reg_data", sep="");
#      cat ( datagen_exec, " ", ilist,  " ", trading_date, " ", start_time, " ", end_time, " 22222 ", t_dgen_outfile, " ", dgen_msecs, " ", dgen_l1events, " ", dgen_trades, " 1\n");
      system ( sprintf ( "%s %s %s %s %s %s %s %s %s %s %s 2>/dev/null; cat %s >> %s", datagen_exec, ilist,  trading_date, start_time, end_time, 22222, t_dgen_outfile, dgen_msecs, dgen_l1events, dgen_trades, to_print_on_eco, t_dgen_outfile,   cated_dgen_outfile) );
}

data = read.csv(cated_dgen_outfile, sep = ' ');
colnames(data) = c('time','events','tgt_price','base_price','indep_price','dep_price','indep_trend','dep_trend')
data$dep_ret = data$dep_trend/data$dep_price
data$indep_ret = data$indep_trend/data$indep_price
print(as.numeric((lm((data$dep_ret) ~ (data$indep_ret) + 0))$coefficients[1]))
system(paste('rm -r', WORK_DIR))






