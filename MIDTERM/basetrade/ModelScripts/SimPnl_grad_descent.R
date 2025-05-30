#! /usr/bin/env Rscript
#suppressPackageStartupMessages( library ( "numDeriv" , lib.loc="/apps/R/root/library/" ) );
.libPaths("/apps/R/root/library/")
suppressPackageStartupMessages( require('numDeriv') );

use_L1_penalty = FALSE;
L1_lambda = 0.00;
use_L2_penalty = FALSE;
L2_lambda = 0.0;
use_L0_penalty = FALSE;
L0_lambda = 0.0;

final_correlation <- 0;
final_mse <- 0;

r2_original <- 0;
r2_model <- 0;
r_squared <- 0;

sd_dep <- 0;
sd_model <- 0;
sd_residual <- 0;

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 7 ) {
	stop ("USAGE : <script> <shortcode> <strat_filename> <model_filename> <reg_output_file> <min_volume> <start_date> <num_days_lookback>\n");	
}

shortcode <-args[1];
reg_output_file <- args[4];
strat_filename <- args[2];
model_filename <- args[3];
min_volume <- args[5];
start_date <- args[6];
num_days_lookback <- args[7];

list_of_dates <- c( );

GetListofDates <- function ( )
{
	cat(sprintf("~/basetrade/scripts/get_list_of_dates_for_shortcode.pl %s %s %s", shortcode, start_date, num_days_lookback),"\n" );
	dates <- system ( sprintf("~/basetrade/scripts/get_list_of_dates_for_shortcode.pl %s %s %s", shortcode, start_date, num_days_lookback),intern=TRUE );
	list_of_dates <<- unlist(strsplit(dates, "\\ "));
	print (list_of_dates);	
}

InitializeIndicators <- function ( )
{
        model_text <- readLines (model_filename);
        for ( i in c(1:length(model_text)) )
        {
            if ( grepl(pattern="INDICATOR ", x = model_text[i] ) )
            {
                t_model_row <- unlist(strsplit(model_text[i], "\\ "));
		t_weight <- as.numeric(t_model_row[2] );
                B <<- c(B,t_weight);
            }
        }	
}

ModifyModelFile <- function ( B ) {
	model_text <- readLines (model_filename);
	for ( i in c(1:length(model_text)) )
	{
	    if ( grepl(pattern="INDICATOR ", x = model_text[i] ) )
	    {
		t_model_row <- unlist(strsplit(model_text[i], "\\ "));	
		t_model_row[2] <- B[i-3];
	    
	  	model_text[i] <- "INDICATOR" ;
	    	for ( j in c(2:length(t_model_row)) )
	    	{
			model_text[i] <- paste(model_text[i], t_model_row[j]);
	    	}
	    }
	}
	cat(model_text,file=model_filename,sep="\n");
	cat(model_text,sep="\n");
}


ComputeObjective <- function (  ) {
	sum_pnl <- 0;
	sum_vol <- 0;

	for ( date in list_of_dates )
	{
	    cat (sprintf('~/basetrade_install/bin/sim_strategy SIM %s 22222 %s %s ADD_DBG_CODE -1',strat_filename,date,mkt_model),"\n");
	    sim_result <- system (sprintf('~/basetrade_install/bin/sim_strategy SIM %s 22222 %s ADD_DBG_CODE -1',strat_filename,date), intern = TRUE );
	    sim_result_words <- unlist(strsplit(sim_result, "\\ "));
	    sum_pnl <- sum_pnl + as.numeric(sim_result_words[2]);
	    sum_vol <- sum_vol + as.numeric(sim_result_words[3]);
	}	
	avg_pnl <- sum_pnl / ( length(list_of_dates) );
	avg_vol <- sum_vol / ( length(list_of_dates) );
	
	penalty <- 0;

	if ( avg_vol < min_volume )
	{
	   penalty <- 1000000;	   
	}
	cat (avg_pnl,avg_vol,"\n",sep=" ");
	retVal <- - avg_pnl + penalty;

}

SimPnl <- function ( B ) {
	ModifyModelFile( B );
	retVal <- ComputeObjective ();
}

SimPnlGradNumerical <- function ( B ) {
	retVal <- grad (SimPnl,B,method="simple");
}

GetListofDates();
B <- c();
InitializeIndicators ( );

mkt_model <- system( sprintf ( '~/basetrade/scripts/get_market_model_for_shortcode.pl %s',shortcode ), intern=TRUE );

num_indicators <- length(B);
print(B);
res <- optim(B, SimPnl, SimPnlGradNumerical, method="BFGS");

Beta <- res$par ;
FinalObjectiveValue <- res$value ;
print(Beta);
