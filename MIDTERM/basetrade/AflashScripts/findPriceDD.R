#!/usr/bin/Rscript

args <- commandArgs(trailingOnly = TRUE);
if (length(args) >= 3) { eventf = args[1]; datGstr = args[2]; outf = args[3]; tm_intvs_m= args[4];} else { stop ("<script> EventFileName DatagenName OutFilename TimeAfterMins"); }
print(length(args))
library(zoo);

px_dat  <- read.table( datGstr ); 
ev_times_dat <- read.table(eventf, sep=","); 

dates <- intersect ( ev_times_dat[,1], unique(px_dat[,1]) );
ndates <- length(dates);

ev_times = ev_times_dat [ match(dates, ev_times_dat[,1]), 2];

tm_intv = as.numeric(tm_intvs_m)*60 * 1000;
tm_dur = 10*1000; #time_duration

px_ts = NULL;
for (i in 1:ndates) { 
  px_ts[[i]] = zoo(c(px_dat[px_dat[,1]==dates[i],5]),c(px_dat[px_dat[,1]==dates[i],2] - ( (3600*floor(ev_times[i]/100) + 60*(ev_times[i]%%100)) * 1000 ) ) ); 
}
pxmax = rep(0,ndates);
pxmin = rep(0,ndates);
pxmean_bf = rep(0,ndates);
pxpost = rep(0, ndates);
pxdd_fall = rep(0,ndates);
pxdd_rise = rep(0,ndates);
pxdd_fall_px = rep(0,ndates);
pxdd_rise_px = rep(0,ndates);

for (j in 1:ndates) { 
  pxser=px_ts[[j]];
  pxsert = pxser [ time(pxser) < tm_intv & time(pxser) > 0 ]; 
  pxmax[j] = max(pxsert);
  pxmin[j] = min(pxsert);

  pxsert_dd_fall = cummax(pxsert) - pxsert;
  dd_fall_idx = which.max(pxsert_dd_fall);
  pxdd_fall_px[j] = pxsert[dd_fall_idx];
  pxdd_fall[j] = pxsert_dd_fall[dd_fall_idx];

  pxsert_dd_rise = pxsert - cummin(pxsert);
  dd_rise_idx = which.max(pxsert_dd_rise);
  pxdd_rise_px[j] = pxsert[dd_rise_idx];
  pxdd_rise[j] = pxsert_dd_rise[dd_rise_idx];

  pxsert = pxser [ time(pxser) > tm_intv ];
  pxpost[j] = pxsert[1];
}

for (j in 1:ndates) { 
  pxser=px_ts[[j]]; 
  pxsert = pxser[time(pxser)<0 & time(pxser)>(-tm_dur)]; 
  tm_dur_t = tm_dur;
  while (length(pxsert) == 0 && tm_dur_t <= 5000*tm_dur) {
    tm_dur_t = 2*tm_dur_t;
    pxsert = pxser[time(pxser)<0 & time(pxser)>(-tm_dur_t)]; 
  }
  pxmean_bf[j] = mean(pxsert); 
}


#print( dim ( pxmean ) )
px_dat = cbind(dates,pxmin-pxmean_bf, pxmax-pxmean_bf, pxpost-pxmean_bf, pxdd_fall, pxdd_fall_px-pxmean_bf, pxdd_rise, pxdd_rise_px-pxmean_bf);
colnames(px_dat) = c("Date", "Min", "Max", "Post", "DDFall", "DDFall_Px", "DDRise", "DDRise_Px");
write.table(px_dat, quote=F, file=outf,sep=",",row.names=F,col.names=T);
