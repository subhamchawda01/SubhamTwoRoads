#!/apps/R/root/bin/Rscript
##!/usr/bin/Rscript

#library(tseries)
#library(forecast)

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

args = commandArgs( trailingOnly=TRUE );
if ( length(args) < 1) 
{
  stop("USAGE: <script> <date> [num_of_pred=1] [model=ARIMA / USE_LAST_DAY] [<datafile>] [arima_param_file(/spare/local/tradeinfo/day_features/dayfeatures_arima_param, ARIMA_DEF=(p,d,q=1,1,1 for ALL)] [events_file/IF]\n");
}

# reading the data with DATES as row.names
date <- args[1];
num_of_pred <- 1;
model_algo <- "ARIMA";
model_algo_args <- c();
datafilename <- "/spare/local/tradeinfo/day_features/dailyfeatures.txt";
arima_param_fl <- "ARIMA_DEF";
special_events_file_ <- "IF";
#special_events_file_ <- "/home/spare/local/tradeinfo/day_features/special_day_features";

#arima_param_fl <- "/spare/local/tradeinfo/day_features/dayfeatures_arima_param";
if ( length(args) >= 2 )
{
  num_of_pred <- max(1, as.integer(args[2]) );
}
if ( length(args) >= 3 )
{
  model_algo_t <- unlist(strsplit(args[3],","));
  model_algo <- model_algo_t[1];
  if ( length(model_algo_t) >= 2 ) {
    model_algo_args <- model_algo_t[2:length(model_algo_t)];
  }
}
if ( length(args) >= 4 )
{
  datafilename <- args[4];
}
if ( length(args) >= 5 )
{
  arima_param_fl <- args[5];
}
if ( length(args) >= 6 )
{
  special_events_file_ <- args[6];
}



# reading 
data <- read.table(datafilename, row.names=1); data <- as.matrix(data);
# filtering in data whose dates are before the required date
data <- data [ row.names(data) < date, , drop=FALSE];
# sorting the data by the date row.names
data <- data [ order(row.names(data)), , drop=FALSE];

special_day_events <- NULL;
if ( special_events_file_ != "IF" ) {
  special_day_events <- system( paste("cat",special_events_file_), intern=TRUE );
}

events_script <- paste(LIVE_BIN_DIR, "economic_events_of_the_day", sep="");
today_events <- system( sprintf("%s %s | awk \'{print $5,$7;}\' | tail -n +2", events_script, date), intern=TRUE );

special_day_events_today <- special_day_events [ which(special_day_events %in% today_events) ];

ev_lastdate_ <- max( row.names(data) );
if ( length(special_day_events_today) != 0 ) {
  ev_lastdate_vec_ = c();
  for ( event in special_day_events_today ) {
    ev_dates <- system ( sprintf("grep -h \"%s\" /home/dvctrader/infracore_install/SysInfo/BloombergEcoReports/merged_eco_*_processed.txt | awk \'{split($5,dt,\"_\"); print dt[1];}\'", event), intern=TRUE );
    ev_dates <- ev_dates [ ev_dates < date & ev_dates %in% row.names(data) ];
    if ( length(ev_dates) > 0 ) {
      ev_lastdate_vec_ = c(ev_lastdate_vec_, max(ev_dates));
    }
  }
  if ( length(ev_lastdate_vec_) > 0 ) { 
    ev_lastdate_ <- max(ev_lastdate_vec_);
    model_algo <- "USE_LAST_DAY";
  }
}

predvalues <- NULL;
if ( model_algo == "ARIMA" ) {
  if ( arima_param_fl == "ARIMA_DEF" ) {
    def_arima_pdq <- matrix(c(1,1,1), nrow=1);
    arima_params <- def_arima_pdq [ rep(1,ncol(data)), , drop=FALSE ];
  } else {
    arima_params <- read.csv(arima_param_fl);
  }

  if ( nrow(arima_params) < ncol(data) ) 
  {
    stop("Error: arima_param_file do not have arima parameter for all features\n");
  }

  for ( i in 1:ncol(data) ) {
    ar_order = as.numeric(arima_params[i,]);

#reading
    x <- as.numeric(data[,i]);

#cleaning
    x<-x[x!=0 & x!=Inf & x!=-Inf];

#train
    tryCatch({ arima_model <<- arima(x, order=ar_order); }, error = function(e) { x <- x[2:length(x)]; print(e); arima_model <<- arima(x, order=ar_order); })
    
#print(arima_model)

#forecast
    arima_predict <- ( predict(arima_model,num_of_pred)$pred );
    predvalues <- cbind(predvalues, as.numeric(arima_predict) );
#    arima_predict <- forecast(arima_model,num_of_pred);    predvalues <- cbind(predvalues, prima_predict$mean);
  }
} else if ( model_algo == "USE_LAST_DAY" ) {
  data_lastrow <- data[row.names(data)==ev_lastdate_,];
  for ( i in 1:ncol(data) ) {
    predvalues <- cbind(predvalues, rep(data_lastrow[i],num_of_pred) );
  }
}

for ( i in 1:num_of_pred )
{
  cat ( predvalues[i,], "\n" );
}
