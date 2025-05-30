#!/usr/bin/Rscript

HOME_DIR <- Sys.getenv("HOME");

args <- commandArgs(trailingOnly = TRUE);
if (length(args) < 2) {
    stop ( "Usage: <script> <shc> <pxchange.dat_path> <dur>" );
}
shc = args[1];
shc_pxch_path = args[2];
dur = args[3];
dur_idx = 2;

if ( dur==2 ) { dur_idx = 2; }
if ( dur==5 ) { dur_idx = 3; }
if ( dur==10 ) { dur_idx = 4; }
if ( dur==20 ) { dur_idx = 5; }
#datGstr = paste(shc,'pxchange.dat',sep='/');
pxdat = read.csv( shc_pxch_path );
#pxdat <- pxdat [ pxdat[,1] > 20140101, ]

yyyymmdd <- system( "date +'%Y%m%d'", intern=T);
min_price_exec <- paste( HOME_DIR, '/basetrade_install/bin/get_min_price_increment', sep='');
min_price <- as.numeric( system( paste(min_price_exec, shc, yyyymmdd), intern=T ) );
#print (min_price);
cat(round(quantile(abs(pxdat[,dur_idx])/min_price, 0.75, na.rm=T), digits=1), "\n")
#abs_summary <- summary( abs(pxdat[,2]) / min_price );
#print ( abs_summary );
