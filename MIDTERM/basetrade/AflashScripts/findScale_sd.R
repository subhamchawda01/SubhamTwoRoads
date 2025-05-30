#!/usr/bin/Rscript

args <- commandArgs(trailingOnly = TRUE);
if (length(args) < 2) { 
  stop ( "Usage: <script> <event_datfile> <pxchange.dat file>" );
}

ev_datfile = args[1];
datGstr = args[2];
pxdat = read.csv( datGstr );

cpid = read.csv(ev_datfile);
dates = intersect ( pxdat[,1], cpid[,1] )
pxdat = pxdat [ match ( dates, pxdat[,1] ), ]
cpid = cpid[ match(dates, cpid[,1] ), ]
cpid[ cpid == "--" ] = NA;

nevents = floor ( ncol(cpid) / 4 );

nch <- NULL;
for (i in 1:nevents) {
  ev_no = i-1;
  nch_t = as.numeric(cpid[,4*ev_no + 4]) - as.numeric(cpid[,4*ev_no + 3]);
  nch <- cbind( nch, abs(nch_t) );
}
#cor( pxdat[,4], nch_rel );

ndates = length(dates)
nperyr = floor(ndates/3);

n12 = which(substr(dates,1,4)=="2012");
n13 = which(substr(dates,1,4)=="2013");
n14 = which(substr(dates,1,4)=="2014");
n15 = which(substr(dates,1,4)=="2015");
ntot = c(n13,n14,n15);

pxsd1 = pxdat[ntot, 3] - pxdat[ntot, 2];
pxsd2 = pxdat[ntot, 4] - pxdat[ntot, 2];
pxsd3 = pxdat[ntot, 5] - pxdat[ntot, 2];

lm_tot1 = lm ( pxsd1 ~ 0 + nch[ntot,1] )
lm_tot2 = lm ( pxsd2 ~ 0 + nch[ntot,1] )
lm_tot3 = lm ( pxsd3 ~ 0 + nch[ntot,1] )

#specify_decimal <- function(x, k) format(round(x, k), nsmall=k);
#cat(c("beta2: ", specify_decimal(lm_tot1$coefficients, 4), summary(lm_tot1)$adj.r.squared,"\n" ) )
#cat(c("beta5: ", specify_decimal(lm_tot2$coefficients, 4), summary(lm_tot2)$adj.r.squared,"\n" ) )
#cat(c("beta10: ", specify_decimal(lm_tot3$coefficients, 4), summary(lm_tot3)$adj.r.squared,"\n" ) )

cat(c("beta5: ", lm_tot1$coefficients, summary(lm_tot1)$adj.r.squared,"\n" ) )
cat(c("beta10: ", lm_tot2$coefficients, summary(lm_tot2)$adj.r.squared,"\n" ) )
cat(c("beta20: ", lm_tot3$coefficients, summary(lm_tot3)$adj.r.squared,"\n" ) )

corval = rep(0,3);
corval[1]= cor( lm_tot1$fitted.values, (lm_tot1$fitted.values + lm_tot1$residuals) );
corval[2] = cor( lm_tot2$fitted.values, (lm_tot2$fitted.values + lm_tot2$residuals) );
corval[3] = cor( lm_tot3$fitted.values, (lm_tot3$fitted.values + lm_tot3$residuals) );
cat(c("lm_corr: ", corval ,"\n") )

#for ( i in 2:5 )  {
#print ( colnames(pxdat)[i])
#print ( quantile ( pxdat[,i], c(0,0.5,0.1,0.3,0.5,0.7,0.9,0.95,1), na.rm=T ) ) 
#print ("\n" )
#}
