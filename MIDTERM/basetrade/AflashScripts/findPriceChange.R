#!/usr/bin/Rscript

args <- commandArgs(trailingOnly = TRUE);
if (length(args) >= 3) { eventf = args[1]; datGstr = args[2]; outf = args[3]; tm_intvs_m= args[4:length(args)];} else { stop ("<script> EvnetFileName DatagenName OutFilename TimeAfterMins"); }
print(length(args))
library(zoo);

px_dat  <- read.table( datGstr ); 
ev_times_dat <- read.table(eventf, sep=","); 

dates <- intersect ( ev_times_dat[,1], unique(px_dat[,1]) );
ndates <- length(dates);

ev_times = ev_times_dat [ match(dates, ev_times_dat[,1]), 2];

tm_intvs= as.numeric(tm_intvs_m)*60 * 1000;
tm_dur = 10*1000; #time_duration
tm_dur1 = 1*1000;
nintv = length(tm_intvs);

px_ts = NULL;
for (i in 1:ndates) { 
  px_ts[[i]] = zoo(c(px_dat[px_dat[,1]==dates[i],5]),c(px_dat[px_dat[,1]==dates[i],2] - ( (3600*floor(ev_times[i]/100) + 60*(ev_times[i]%%100)) * 1000 ) ) ); 
}
pxmean = matrix(0, nrow=ndates, ncol=nintv)
for (i in 1:nintv) { 
  for (j in 1:ndates) { 
    pxser=px_ts[[j]]; 
    t_et = tm_intvs[i]
    pxsert = pxser [ time(pxser) > t_et ];
    pxmean[j,i] = pxsert[1];
  }
}
pxmean_bf = rep(0,ndates)
for (j in 1:ndates) { 
  pxser=px_ts[[j]]; 
  pxsert = pxser[time(pxser)<0 & time(pxser)>(-tm_dur)]; 
  tm_dur_t = tm_dur;
  while (length(pxsert) == 0 && tm_dur_t <= 5000*tm_dur) {
#    print (tm_dur_t);
    tm_dur_t = 2*tm_dur_t;
    pxsert = pxser[time(pxser)<0 & time(pxser)>(-tm_dur_t)]; 
  }
  pxmean_bf[j] = mean(pxsert); 
}
pxch = sweep(pxmean, 1, pxmean_bf, '-');
colnames(pxch) = tm_intvs_m;
#print( dim ( pxmean ) )
px_dat = cbind(dates, pxch );
colnames(px_dat) = c("Date", colnames(pxch) );
#print(dim(px_dat))
write.table(px_dat, quote=F, file=outf,sep=",",row.names=F,col.names=T);
