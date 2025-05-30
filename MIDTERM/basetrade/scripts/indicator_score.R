#!/usr/bin/env Rscript

HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");

REPO <- "basetrade";

Sys.setenv(LD_LIBRARY_PATH="/apps/gcc_versions/gcc-4_9_install/lib64/")

SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");
MODELSCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/ModelScripts/", sep="");
GENPERLLIB_DIR <- paste(HOME_DIR, "/", REPO, "_install/GenPerlLib/", sep="");
LIVE_BIN_DIR <- paste(HOME_DIR, "/LiveExec/bin/", sep="");
STRATS_DIR <- "/home/dvctrader/modelling/strats";
if ( ! file.exists(LIVE_BIN_DIR ))
{
    LIVE_BIN_DIR <- paste(HOME_DIR,"/", REPO,"_install/bin/", sep="");
}

TRADELOG_DIR <- "/spare/local/logs/tradelogs/";
WORK_DIR <- "/spare/local/";

GetListOfDates <- function (shortcode, start_date, num_days_lookback )
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

GetDataForPeriod <- function (ilist, trading_date, start_time, end_time)
{
#	WORK_DIR <- "./";
#	datagen_exec <- paste(LIVE_BIN_DIR, "datagen", sep="");
	datagen_exec <- "/home/dvctrader/LiveExec/bin/datagen";
	t_dgen_outfile <- paste( WORK_DIR, "t_dgen_outfile", ".td.", trading_date, ".st.", start_time, ".et.", end_time, sep="" );
	system ( sprintf ( "%s %s %s %s %s %s %s %s %s %s %s 2>/dev/null", datagen_exec, ilist,  trading_date, start_time, end_time, 22222, t_dgen_outfile, 1000, 0, 0, 0 ) );
#	print (sprintf ( "%s %s %s %s %s %s %s %s %s %s %s 2>/dev/null", datagen_exec, ilist,  trading_date, start_time, end_time, 22222, t_dgen_outfile, 1000, 0, 0, 0 ));
}

GetModelFromStrat <- function (shortcode, strat)
{
	if (regexpr('/', strat)==-1) {
		exec_cmd <- paste(STRATS_DIR, "/",  shortcode, "/*/", strat,sep="");
#		print (exec_cmd);
#		strategy_file <- system( sprintf("cat %s/%s/*/%s", STRATS_DIR, shortcode, strat), intern=TRUE );
	}
	else {
		exec_cmd <- strat;
#		print (exec_cmd);
	}
	strategy_file <- system ( sprintf("cat %s", exec_cmd), intern=TRUE );
	strategy_file <- strsplit(strategy_file, " ");
	model <- strategy_file[[1]][4];
#	print (model);
	return (model);
}

GetIndicatorWeight <- function (model_file)
{
	conn <- file(model_file, open="r");
	lines <- readLines(conn);
	weights <- c();
	for (i in 1:length(lines)) {
		#print (lines[i])
		tokens <- strsplit(lines[i]," ")[[1]];
		#print (tokens);
		if (tokens[1] == "INDICATOR") { weights <- c(weights, as.numeric(tokens[2]))};
	}
	close(conn);
	return (weights);
}

GetIndicatorString <- function (model_file)
{
	conn <- file(model_file, open="r");
	lines <- readLines(conn);
	indicator_list <- list();
	for (i in 1:length(lines)) {
#		print (lines[i]);
		tokens <- strsplit(lines[i],"#")[[1]][1];
		if (regexpr('INDICATOR ', tokens)==1) { indicator_list[[length(indicator_list)+1]] <- tokens; }
	}
	close(conn);
	return (indicator_list);
}

GetWeightedIndicatorValues <- function (indicator_weights, trading_date, start_time, end_time)
{
	t_dgen_outfile <- paste( WORK_DIR, "t_dgen_outfile", ".td.", trading_date, ".st.", start_time, ".et.", end_time, sep="" );
	indicator_data <- read.table( t_dgen_outfile );
	weighted_indicator_data <- c();
	for (i in 1:length(indicator_weights)) {
		weighted_indicator_data <- cbind(weighted_indicator_data, indicator_data[,i+4]*indicator_weights[i]);
	}
	return (weighted_indicator_data);
}

CalcIndicatorScore <- function(weighted_indicator_data, trading_date, start_time, end_time)
{
	t_dgen_outfile <- paste( WORK_DIR, "t_dgen_outfile", ".td.", trading_date, ".st.", start_time, ".et.", end_time, sep="" );
	indicator_data <- read.table( t_dgen_outfile );
	PRICE_CHANGE <- indicator_data[nrow(indicator_data),3] - indicator_data[1,3];
	if(PRICE_CHANGE>0){PRICE_CHANGE=1;}
	else {PRICE_CHANGE=-1;}
	logical_indicator_score <- (weighted_indicator_data*PRICE_CHANGE) > 0;
	return (logical_indicator_score);
#	for (i in 1:ncol(weighted_indicator_data)){
#		print (paste("Indicator ", i, " score :: ",100*sum(logical_indicator_score[,i], na.rm=TRUE)/length(logical_indicator_score[,i])));
#	}
}

GetPnlSamplesForStrat <- function(shortcode, strat, date, num_days)
{
	script <- paste (SCRIPTS_DIR, "get_strat_pnl_samples.pl", sep="");
        pnl_samples_for_strat_for_date <- system ( sprintf("perl %s %s %s %s %s 2>/dev/null",script, shortcode, strat, date, num_days),intern=TRUE);
	pnls_samples_for_strat_for_date <- strsplit(pnl_samples_for_strat_for_date, " ");	
	return (pnls_samples_for_strat_for_date);
}

GetDrawdownsForStrat <- function(pnl_samples, threshold)
{
	dd_time_periods = list();
	for (samples in pnl_samples){
		sample_date <- samples[1];
		max_draw_down <- 0;
		peak <- -999999
		drawdown <- c();
		samples <- as.numeric(samples[3:length(samples)]);
		for (i in seq(2,length(samples),2)){
			if(samples[i] > peak) {
				peak <- samples[i];
			}
			drawdown <- c(drawdown, peak - samples[i]);
		}
#		print (max(drawdown));
		max_drawdown = max(drawdown);
#		print ("Samples ::");
#		print (samples);
#		print ("Drawdown ::");
#		print (drawdown);
		max_drawdown_idx <- which(drawdown == max_drawdown)[1];
#		print (c("Max drawdown index :: ", max_drawdown_idx));
		peak_idx <- which(samples[seq(1,2*max_drawdown_idx)] == max(samples[seq(2, 2*max_drawdown_idx, 2)]))[1];
#		print (samples);
#		print (drawdown);
#		print (samples[peak_idx-1]);
#		print (samples[2*max_drawdown_idx-1]);
		dd_time_periods[[length(dd_time_periods)+1]] <- c(sample_date, samples[peak_idx-1], samples[2*max_drawdown_idx-1], max_drawdown);
	}
	return (dd_time_periods);
}

GetUTCFromMfm <- function(t_Mfm)
{
	minutes <- t_Mfm / 60000;
	hours <- minutes %/% 60;
	minutes <- minutes %% 60;
 	return (sprintf("%d%02d",hours, minutes));
}

GetThreshold <- function(bad_periods, percentile) {
	drawdown_vec <- c();
	for (period in bad_periods) {
		drawdown_vec <- c(drawdown_vec,as.numeric(period[4]));
	}
	threshold <- quantile(drawdown_vec, percentile);
	return (threshold);
}
args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 2 ) {
        stop ("USAGE : <script> <shortcode> <strat> <date=TODAY> <num_days=10> <percentile=0.9>\n");
}

shortcode <- args[1];
strat <- args[2];
date <- format(Sys.Date()-1, format="%Y%m%d");
num_days <- 10;
threshold <- 1500;
percentile <- 0.9;
if (length(args) > 2){
date <- args[3];
num_days <- args[4];
percentile <- as.numeric(args[5]);
}

model_file <- GetModelFromStrat(shortcode, strat);
indicator_weights <- GetIndicatorWeight(model_file);
indicator_list <- GetIndicatorString(model_file);
logical_indicator_score <- c()
if (regexpr("/", strat)!=-1) {
	strat_words <- strsplit(strat, "/")[[1]];
#	print (strat_words);
	strat <- strat_words[length(strat_words)];
}
pnl_samples <- GetPnlSamplesForStrat(shortcode, strat, date, num_days);
bad_periods <- GetDrawdownsForStrat(pnl_samples);
#threshold <- GetThreshold(bad_periods, 0.9);
drawdown_vec <- c();
for (period in bad_periods) {
        drawdown_vec <- c(drawdown_vec,as.numeric(period[4]));
}
threshold <- quantile(drawdown_vec, percentile);
bad_periods_idx <- drawdown_vec > threshold;
imp_bad_periods <- bad_periods[bad_periods_idx];
sum_drawdowns <- sum(drawdown_vec[bad_periods_idx]);
weighted_indicator_score <- rep(0,length(indicator_weights));
print ("DATE START_TIME END_TIME DRAWDOWN");
for (period in imp_bad_periods)
{
	print (period);
	trading_date = period[1];
	start_time = GetUTCFromMfm(as.numeric(period[2]));
	end_time = GetUTCFromMfm(as.numeric(period[3]));
	drawdown = as.numeric(period[4]);
#	weighted_indicator_score <- rep(0,length(indicator_weights));
	if(drawdown>threshold){
		GetDataForPeriod(model_file, trading_date, start_time, end_time);
		weighted_indicator_data <- GetWeightedIndicatorValues(indicator_weights, trading_date, start_time, end_time);
		logical_indicator_score <- rbind(logical_indicator_score, CalcIndicatorScore(weighted_indicator_data, trading_date, start_time, end_time));
		for (i in 1:ncol(weighted_indicator_data)) {
			weighted_indicator_score[i] <- weighted_indicator_score[i] + 100*(drawdown/sum_drawdowns)*sum(logical_indicator_score[,i], na.rm=TRUE)/length(logical_indicator_score[,i]);
#			print (weighted_indicator_score[i]);
		}
	}
}
for (i in 1:length(weighted_indicator_score))
{
	print (paste(indicator_list[[i]],"::", weighted_indicator_score[i], sep=""));
}
