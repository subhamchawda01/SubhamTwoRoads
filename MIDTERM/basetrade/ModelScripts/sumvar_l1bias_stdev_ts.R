#!/usr/bin/env Rscript
# ~/basetrade_install/bin/datagen 300/w_model_ilist_NKM_0_US_Mkt_Mkt_best_300_na_e3_20170228_20170606_EST_800_UTC_2025_4000_c3_0_4938404_0_fv_FSRMSH.6.0.02.0.0.0.65.12.N 20170619 EST_0800 UTC_1900 2897 PBAT_BIAS\@/home/dvctrader/kputta/kp_dout 1000 1 0 0
# 43200010 3662 20047.790927 20045.000000 20050.000000 -0.821260
# 43200078 3664 20047.790927 20045.000000 20050.000000 -0.842891



GetListOfDates <- function ( )
{
        script <- paste ("~/basetrade/scripts/", "get_dates_for_shortcode.pl", sep="");
        if (!as.logical(gsub("1","T",gsub("-1","F",as.character(regexpr("[^0-9]",start_date,))))) ){
                dates <- system ( sprintf("%s %s %s %s 2>/dev/null",script, shortcode, num_days_to_lookback, start_date),intern=TRUE);
        } else {
                dates <- system (sprintf("cat %s 2>/dev/null",start_date),intern=TRUE );
        }
        list_of_dates <- unlist(strsplit(dates, "\\ "));
        return (list_of_dates)
}


GetDataMatrix <- function ( ) 
{

	data_file <- paste(work_dir, "gdout", sep = "")
	out_file <- paste(work_dir, "dout", sep = "")
	dout <- paste("PBAT_BIAS", "@", out_file, sep = "")
	rid <- 9881
	sumvars_sd <- c()
	l1bias_sd <- c()
	target_sd <- c()
	corr_sumvars_l1bias <- c()

        for (date in list_of_dates)
        {
                script <- paste("~/cvquant_install/basetrade/bin/datagen", model_file, date, start_time, end_time, rid, dout, "1000 0 0 0", sep = " " )
                system(script, intern=T)
		if ( file.info(out_file)$size > 0 ) 
		{
		   data <- read.table(out_file, sep = " ")
		   system(paste("rm ", out_file, sep = "" ))
		   sumvars_sd <- c(sumvars_sd, sd ( data$V6 ))
		   l1bias_sd <- c(l1bias_sd, sd(data$V3 - (data$V4 + data$V5)/2))
		   target_sd <- c(target_sd, sd(data$V6 + (data$V3 - (data$V4 + data$V5)/2)))
		   corr_sumvars_l1bias <- c(corr_sumvars_l1bias, cor(data$V6, (data$V3 - (data$V4 + data$V5)/2)))
		   data <- c()
		}
        }
	write.table(cbind(list_of_dates, sumvars_sd, l1bias_sd, target_sd, corr_sumvars_l1bias), output_file)
}

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 6 )
{

        stop ("USAGE : <script> <shortcode> <start_date> <num_days_lookback> <start_time> <end_time> <model_file>\n");
}

shortcode <- args[1]
start_date <- args[2]
num_days_to_lookback <- as.numeric(args[3])
start_time <- args[4]
end_time <- args[5]
model_file <- args[6]
work_dir <- "/tmp/svl1/"
system(paste("mkdir -p ", work_dir, sep = ""))
output_file <- paste(work_dir, "data", sep = "")
list_of_dates <- GetListOfDates()
GetDataMatrix()

