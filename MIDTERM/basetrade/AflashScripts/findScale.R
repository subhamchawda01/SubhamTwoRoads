#!/usr/bin/env Rscript

.libPaths("/apps/R/root/library/") ; # probably not needed after ~/.Renviron change
suppressPackageStartupMessages ( library ( "quantreg" ) ) ;

args <- commandArgs(trailingOnly = TRUE);
if (length(args) < 2) { 
  stop ( "Usage: <script> <event_datfile> <shc> percentile(csv)" );
}

ev_datfile = args[1];
shc = args[2];
datGstr = args[3];

percentile = c(0.5,0.75);
if (length(args) > 3) {
  percentile = as.numeric(strsplit(args[4], ",")[[1]]);
}

revised = c();
if (length(args) > 4) {
  revised = strsplit(args[5], ",");
}

pxdat = read.csv( datGstr );

cpid = read.csv(ev_datfile);
dates = intersect ( pxdat[,1], cpid[,1] )
pxdat = pxdat [ match ( dates, pxdat[,1] ), ]
cpid = cpid[ match(dates, cpid[,1] ), ]
cpid[ cpid == "--" ] = NA;

nevents = floor ( ncol(cpid) / 4 );
#nevents = 1;

nch <- NULL;
for (i in 1:nevents) {
  ev_no = i-1;
  nch_t = as.numeric(cpid[,4*ev_no + 4]) - as.numeric(cpid[,4*ev_no + 3]);
  nch <- cbind( nch, nch_t);
}
for (i in 1:nevents) {
  ev_no = i-1;
  if ( i %in% revised ) {
    nch_t = as.numeric(cpid[,4*ev_no + 6]) - as.numeric(cpid[,4*ev_no + 5]);
    nch <- cbind( nch, nch_t);
  }
}
#cor( pxdat[,4], nch_rel );

ndates = length(dates)
nperyr = floor(ndates/3);

n12 = which(substr(dates,1,4)=="2012");
n13 = which(substr(dates,1,4)=="2013");
n14 = which(substr(dates,1,4)=="2014");
n15 = which(substr(dates,1,4)=="2015");
n16 = which(substr(dates,1,4)=="2016");
ntot = c(n14,n15,n16);

HOME_DIR <- Sys.getenv("HOME");
options(digits = 3);

lm_tot1 = lm ( pxdat[ntot, 2] ~ 0 + nch[ntot,] );
lm_tot2 = lm ( pxdat[ntot, 3] ~ 0 + nch[ntot,] );
lm_tot3 = lm ( pxdat[ntot, 4] ~ 0 + nch[ntot,] );

cat("beta2: ", lm_tot1$coefficients, summary(lm_tot1)$adj.r.squared,"\n" );
cat("beta5: ", lm_tot2$coefficients, summary(lm_tot2)$adj.r.squared,"\n" );
cat("beta10: ", lm_tot3$coefficients, summary(lm_tot3)$adj.r.squared,"\n" );

corval = rep(0,3);
corval[1]= cor( lm_tot1$fitted.values, (lm_tot1$fitted.values + lm_tot1$residuals) );
corval[2] = cor( lm_tot2$fitted.values, (lm_tot2$fitted.values + lm_tot2$residuals) );
corval[3] = cor( lm_tot3$fitted.values, (lm_tot3$fitted.values + lm_tot3$residuals) );
cat(c("lm_corr: ", corval ,"\n") )

yyyymmdd <- system( "date +'%Y%m%d'", intern=T);
min_price_exec <- paste( HOME_DIR, '/basetrade_install/bin/get_min_price_increment', sep='');
min_price <- as.numeric( system( paste(min_price_exec, shc, yyyymmdd), intern=T ) );
for (pctile in percentile) {
  median = rep(0,3);
  median[1] = round(quantile(abs(lm_tot1$fitted.values)/min_price, pctile, na.rm=T), digits=1);
  median[2] = round(quantile(abs(lm_tot2$fitted.values)/min_price, pctile, na.rm=T), digits=1);
  median[3] = round(quantile(abs(lm_tot3$fitted.values)/min_price, pctile, na.rm=T), digits=1);
  cat(paste("percentile",pctile,sep=""), median, "\n");
  
  median[1] = round(quantile(abs(lm_tot1$fitted.values + lm_tot1$residuals)/min_price, pctile, na.rm=T), digits=1);
  median[2] = round(quantile(abs(lm_tot2$fitted.values + lm_tot2$residuals)/min_price, pctile, na.rm=T), digits=1);
  median[3] = round(quantile(abs(lm_tot3$fitted.values + lm_tot3$residuals)/min_price, pctile, na.rm=T), digits=1);
  cat(paste("act_perc",pctile,sep=""), median, "\n");
}

# LM on filtered data
#filt_rows = which( abs(lm_tot2$fitted.values) > quantile(abs(lm_tot2$fitted.values), 0.5, na.rm=T) );
#ntot1 = ntot[filt_rows];
#filt_lm_tot1 = lm ( pxdat[ntot1, 2] ~ 0 + nch[ntot1,] );
#filt_lm_tot2 = lm ( pxdat[ntot1, 3] ~ 0 + nch[ntot1,] );
#filt_lm_tot3 = lm ( pxdat[ntot1, 4] ~ 0 + nch[ntot1,] );
#cat("filt_beta2: ", filt_lm_tot1$coefficients, summary(filt_lm_tot1)$adj.r.squared,"\n" );
#cat("filt_beta5: ", filt_lm_tot2$coefficients, summary(filt_lm_tot2)$adj.r.squared,"\n" );
#cat("filt_beta10: ", filt_lm_tot3$coefficients, summary(filt_lm_tot3)$adj.r.squared,"\n" );

# LAD
#lad_tot1 = rq ( pxdat[ntot, 2] ~ 0 + nch[ntot,], 0.5);
#lad_tot2 = rq ( pxdat[ntot, 3] ~ 0 + nch[ntot,], 0.5);
#lad_tot3 = rq ( pxdat[ntot, 4] ~ 0 + nch[ntot,], 0.5);
#cat("LAD_beta2: ", lad_tot1$coefficients, "\n");
#cat("LAD_beta5: ", lad_tot2$coefficients, "\n");
#cat("LAD_beta10: ", lad_tot3$coefficients, "\n");

#cat("beta2:", lm_tot1$coefficients, filt_lm_tot1$coefficients, lad_tot1$coefficients, "\n");
#cat("beta5:", lm_tot2$coefficients, filt_lm_tot2$coefficients, lad_tot2$coefficients, "\n");
#cat("beta10:", lm_tot3$coefficients, filt_lm_tot3$coefficients, lad_tot3$coefficients, "\n");

