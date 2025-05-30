#!/usr/bin/Rscript

args <- commandArgs(trailingOnly = TRUE);
if (length(args) < 2) { 
  stop ( "Usage: <script> <event_datfile> <shc>" );
}

ev_datfile = args[1];
shc = args[2];
datGstr = args[3];
pxdat = read.csv( datGstr );

cpid = read.csv(ev_datfile);
dates = intersect ( pxdat[,1], cpid[,1] )
pxdat = pxdat [ match ( dates, pxdat[,1] ), ]
cpid = cpid[ match(dates, cpid[,1] ), ]
cpid[ cpid == "--" ] = NA;

#nevents = floor ( ncol(cpid) / 4 );
nevents = 1;

nch <- NULL;
for (i in 1:nevents) {
  ev_no = i-1;
  nch_t = as.numeric(cpid[,4*ev_no + 4]) - as.numeric(cpid[,4*ev_no + 3]);
  nch <- cbind( nch, nch_t);
  nch_t = as.numeric(cpid[,4*ev_no + 6]) - as.numeric(cpid[,4*ev_no + 5]);
  nch <- cbind( nch, nch_t);
}

#cor( pxdat[,4], nch_rel );

ndates = length(dates)
nperyr = floor(ndates/3);

n12 = which(substr(dates,1,4)=="2012");
n13 = which(substr(dates,1,4)=="2013");
n14 = which(substr(dates,1,4)=="2014");
n15 = which(substr(dates,1,4)=="2015");
n16 = which(substr(dates,1,4)=="2016");
ntot = c(n13,n14,n15,n16);

lm_tot1 = lm ( pxdat[ntot, 2] ~ 0 + nch[ntot,] );
lm_tot2 = lm ( pxdat[ntot, 3] ~ 0 + nch[ntot,] );
lm_tot3 = lm ( pxdat[ntot, 4] ~ 0 + nch[ntot,] );
lm_tot4 = lm ( pxdat[ntot, 5] ~ 0 + nch[ntot,] );

#specify_decimal <- function(x, k) format(round(x, k), nsmall=k);
#cat(c("beta2: ", specify_decimal(lm_tot1$coefficients, 4), summary(lm_tot1)$adj.r.squared,"\n" ) )
#cat(c("beta5: ", specify_decimal(lm_tot2$coefficients, 4), summary(lm_tot2)$adj.r.squared,"\n" ) )
#cat(c("beta10: ", specify_decimal(lm_tot3$coefficients, 4), summary(lm_tot3)$adj.r.squared,"\n" ) )

options(digits = 3);

cat("beta2: ", lm_tot1$coefficients, summary(lm_tot1)$adj.r.squared,"\n" );
cat("beta5: ", lm_tot2$coefficients, summary(lm_tot2)$adj.r.squared,"\n" );
cat("beta10: ", lm_tot3$coefficients, summary(lm_tot3)$adj.r.squared,"\n" );
cat("beta20: ", lm_tot4$coefficients, summary(lm_tot4)$adj.r.squared,"\n" );

corval = rep(0,3);
corval[1]= cor( lm_tot1$fitted.values, (lm_tot1$fitted.values + lm_tot1$residuals) );
corval[2] = cor( lm_tot2$fitted.values, (lm_tot2$fitted.values + lm_tot2$residuals) );
corval[3] = cor( lm_tot3$fitted.values, (lm_tot3$fitted.values + lm_tot3$residuals) );
corval[4] = cor( lm_tot4$fitted.values, (lm_tot4$fitted.values + lm_tot4$residuals) );

cat(c("lm_corr: ", corval ,"\n") )

HOME_DIR <- Sys.getenv("HOME");

yyyymmdd <- system( "date +'%Y%m%d'", intern=T);
min_price_exec <- paste( HOME_DIR, '/basetrade_install/bin/get_min_price_increment', sep='');
min_price <- as.numeric( system( paste(min_price_exec, shc, yyyymmdd), intern=T ) );

median = rep(0,3);
median[1] = round(quantile(abs(lm_tot1$fitted.values)/min_price, 0.5, na.rm=T), digits=1);
median[2] = round(quantile(abs(lm_tot2$fitted.values)/min_price, 0.5, na.rm=T), digits=1);
median[3] = round(quantile(abs(lm_tot3$fitted.values)/min_price, 0.5, na.rm=T), digits=1);
cat("Median: ", median, "\n");

median[1] = round(quantile(abs(lm_tot1$fitted.values)/min_price, 0.75, na.rm=T), digits=1);
median[2] = round(quantile(abs(lm_tot2$fitted.values)/min_price, 0.75, na.rm=T), digits=1);
median[3] = round(quantile(abs(lm_tot3$fitted.values)/min_price, 0.75, na.rm=T), digits=1);
cat("75percentile: ", median, "\n");

#for ( i in 2:5 )  {
#print ( colnames(pxdat)[i])
#print ( quantile ( pxdat[,i], c(0,0.5,0.1,0.3,0.5,0.7,0.9,0.95,1), na.rm=T ) ) 
#print ("\n" )
#}
