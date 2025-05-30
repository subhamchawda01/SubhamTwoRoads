#!/usr/bin/env Rscript

HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");
REPO <- "basetrade";

SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");
MODELSCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/ModelScripts/", sep="");
GENPERLLIB_DIR <- paste(HOME_DIR, "/", REPO, "_install/GenPerlLib/", sep="");
LIVE_BIN_DIR <- paste(HOME_DIR, "/LiveExec/bin/", sep="");


get_error <- function(c, k, owp, mkt, r){
	return ( mkt + (owp-mkt)*c*(1-exp(-k*(r-1))) );
	#return ( mkt + (owp-mkt)*c*(1-(1/(1+k*r-k))) );
}

args = commandArgs( trailingOnly=TRUE )
  if ( length(args) < 7 ) {
            stop ("USAGE : <script> <shortcode> <start_date> <num_days_lookback> <start_time> <end_time> <pred_num_events> <work_directory>\n");
  }

dep_shortcode <- args[1];
#ilist <- args[2];
start_date <- args[2];
num_days_lookback <- max(as.numeric(args[3]),5);
start_time <- args[4];
end_time <- args[5];
num_events <- max(as.numeric(args[6]),3);
WORK_DIR <- args[7] ;
system(paste('mkdir -p', WORK_DIR))

ilist <- paste(WORK_DIR,"/ilist",sep="");
sink(ilist);
cat("MODELINIT DEPBASE", dep_shortcode, "MktSizeWPrice OrderWPrice \nMODELMATH LINEAR CHANGE \nINDICATORSTART \nINDICATOR 1.0 Expression MAXBYMIN 3 1.0 LevelSizePerOrder",dep_shortcode,"0 0 3 1.0 LevelSizePerOrder",dep_shortcode,"0 1 \nINDICATOREND\n");
sink();

generate_data_exec <- paste(MODELSCRIPTS_DIR, "generate_data_for_online_price_const.R", sep="");

system (sprintf(  "%s %s %s %s %s %s %s %s %s 2>/dev/null;", generate_data_exec, dep_shortcode, ilist, start_date, num_days_lookback, start_time, end_time, num_events, WORK_DIR )) ;

cated_regdata_filename <- paste( WORK_DIR, "/cated_regdata_outfile", sep="" );
data=read.table(cated_regdata_filename, sep=' '); # take data into frame

system (sprintf("rm -rf %s ;", WORK_DIR)) ;

n_row = nrow(data);

min.RSS <- function (data, par){
        error = sum ((get_error(par[1], par[2] , data[,1], data[,2],data[,3]) - ( 4*get_error(par[1],par[2],data[,4], data[,5],data[,6]) + 3*get_error(par[1],par[2],data[,7], data[,8],data[,9]) + 2*get_error(par[1],par[2],data[,10], data[,11],data[,12]) )/9)^2) ;
        return (error) ;
}

result <- optim( par = c(0.9, 2), min.RSS, data=data , method="L-BFGS-B", lower = c(0,0), upper = c(1,5)) ; 
cat ("c : ", result$par[1], "\n");
cat("k : ", result$par[2], "\n");
