#!/usr/bin/Rscript


args = commandArgs( trailingOnly=TRUE );
if ( length(args) < 4)
{
  stop("USAGE: <script> <BBG/FXS> <dat_filename> <evtime/-1> <ev1_bbg_name> <ev2_bbg_name> .. ");
}

ev_opt <- args[1];
fname <- args[2];
evtime <- args[3];
cpi_cats <- args[4:length(args)];
catlen <- length(cpi_cats);
start_date <- 20130101;

alphdf <- NULL;
coln <- c();
ev_cols <- c("Survey", "Actual", "Prior", "Revised");

if ( ev_opt == "BBG" ) { 
  alphd1 <- read.csv ( "/home/dvctrader/modelling/alphaflash/bbg_us_eco.csv" );

  alphdc <- NULL;
  for ( i in 1:catlen ) {
    alphdc[[i]] = alphd1 [alphd1[,3]=="US" & alphd1[,4]==cpi_cats[i], ];
  }

  dates = unique ( alphdc[[1]][,1] );

  alphdf = alphdc[[1]][match( dates, alphdc[[i]][,1]) ,c(1,2)];
  alphdf[,2] = paste( "EST_", alphdf[,2], sep="" );
  for ( i in 1:nrow(alphdf) ) {
    alphdf[i,2] <- system ( sprintf ( "/home/dvctrader/basetrade_install/bin/get_utc_hhmm_str %s %s", alphdf[i,2], alphdf[i,1] ), intern=T );
  }
  coln = c("Date","Time");

  for ( i in 1:catlen ) {
    alphd_t = alphdc[[i]] [ match( dates, alphdc[[i]][,1]), c(6,7,8,9) ];
    alphd_t [ alphd_t == "--" ] = NA;
    alphdf = cbind( alphdf, alphd_t );
    coln = c(coln, paste ( cpi_cats[i], ev_cols, sep=" " ) ); 
  }
} else if ( ev_opt == "FXS" ) {
  alphd1 <- read.csv ( "/home/dvctrader/modelling/alphaflash/fxstreet_us_eventdates.csv", sep="," );

  alphdc <- NULL;
  for ( i in 1:catlen ) {
    alphdc[[i]] = alphd1 [alphd1[,3]=="United States" & alphd1[,2]==cpi_cats[i], ];
  }

  dates = unique ( alphdc[[1]][,1] );
  dates1 = paste( substr(dates,7,10), substr(dates,0,2), substr(dates,4,5), sep="" );
  dates <- dates[ dates1 >= 20140101 ];
  dates1 <- dates1[ dates1 >= 20140101 ];

  times <- substr(dates,12,19);
  times1 <- paste( substr(times,1,2), substr(times,4,5), sep="" );

  alphdf = cbind(dates1, times1);
  coln = c("Date","Time");

  for ( i in 1:catlen ) {
    alphd_t = alphdc[[i]] [ match( dates, alphdc[[i]][,1]), c(7,5,6) ];
    alphd_t [ alphd_t == "--" ] = NA;
    alphd_t = cbind( alphd_t, NA );
    alphdf = cbind( alphdf, alphd_t);
    coln = c(coln, paste ( cpi_cats[i], ev_cols, sep=" " ) ); 
  }
} else {
  stop(" Error: Please enter valid event_type option: BBG / FXS");
}

colnames(alphdf) = coln;

alphdf <- alphdf[as.numeric(as.character(alphdf[,1]))>=start_date, ];

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
