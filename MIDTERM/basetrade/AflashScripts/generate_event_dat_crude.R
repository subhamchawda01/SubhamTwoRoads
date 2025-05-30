#!/usr/bin/Rscript


args = commandArgs( trailingOnly=TRUE );
if ( length(args) < 2)
{
  stop("USAGE: <script> <dat_filename> <evtime/-1> ");
}

fname <- args[1];
evtime <- args[2];

estimate_cat <- "API Weekly Crude Oil Stock";
release_cat <- "EIA Crude Oil Stocks change";

start_date <- "20151201";


alphd1 <- read.csv ( "/home/dvctrader/modelling/alphaflash/fxstreet_us_eventdates.csv", sep="," );

alphdc_est <- alphd1 [alphd1[,3]=="United States" & alphd1[,2]==estimate_cat, ];
alphdc_rel <- alphd1 [alphd1[,3]=="United States" & alphd1[,2]==release_cat, ];

orig_dates_obj <- as.Date(substr(alphdc_rel[,1],0,10), format="%m/%d/%Y");
est_dates_obj <- as.Date(substr(alphdc_est[,1],0,10), format="%m/%d/%Y");

dates <- intersect(format(orig_dates_obj, format="%Y%m%d"), format(est_dates_obj+1, format="%Y%m%d") );
dates <- dates[ dates > start_date ];

dates_obj <- as.Date(dates, format="%Y%m%d");

alphdc_rel <- alphdc_rel[ match(dates_obj, orig_dates_obj), ];
alphdc_est <- alphdc_est[ match(dates_obj-1, est_dates_obj), ];

times <- sapply(substr(alphdc_rel[,1],12,19), function(x) paste(substr(x,1,2),substr(x,4,5), sep=""));

alphdf = cbind(dates, times, alphdc_est[,5], alphdc_rel[,c(5,6)]);
alphdf[alphdf=="--"] = NA;
colnames(alphdf) = c("Date","Time","Survey", "Actual", "Prior");

if ( evtime != -1 ) { 
  valid_evtime_idx <- c();
  for ( i in 1:nrow(alphdf) ) {
    ideal_evtime <- system ( sprintf ( "/home/dvctrader/basetrade_install/bin/get_utc_hhmm_str %s %s", evtime, alphdf[i,1] ), intern=T );
    if ( ideal_evtime == alphdf[i,2] ) {
      valid_evtime_idx <- c(valid_evtime_idx, i);
    }
  }
  alphdf <- alphdf[valid_evtime_idx, ];
}

write.table(alphdf, quote=F, file=fname,sep=",",row.names=F,col.names=T);

sedcmd = sprintf("sed -i s,%%,,g %s", fname);
system(sedcmd)
