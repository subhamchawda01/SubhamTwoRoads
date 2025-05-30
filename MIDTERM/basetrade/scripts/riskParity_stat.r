#!/usr/bin/Rscript

###CONFIG#####
datF <- "~/RiskParity/datagen_out1";
codes <- c("FGBS_0", "FGBM_0", "FGBL_0", "FGBX_0", "LFR_0", "ZN_0", "ZB_0", "UB_0");
num2dollars <- c(1340.120000, 1340.120000, 1340.120000, 1340.120000, 1692.620000, 1000.000000, 1000.000000, 1000.000000);
stdevs_pxdiff <- c(0.003692, 0.018057, 0.036662, 0.058865, 0.037494, 0.030935, 0.057402, 0.085062);
close_EOD <- F;
aggress <- F;
horizon_msecs <- 60000;
ndays <- 100;
###############

args <- commandArgs(trailingOnly = TRUE);
print(args);
#options(echo=TRUE);
if (length(args) >= 1) { source(args[1]); }

datgt_raw <- read.table(datF, quote="\"");
ncolsinit <- c("date", "msecs","nevents", "rem", "rem");
names(datgt_raw) <- c(ncolsinit, codes, codes, codes);	
dropcols = c("rem");
datgt <- datgt_raw[, !(names(datgt_raw) %in% dropcols)];
rm(dropcols);
rm(datgt_raw);

k <- length(codes);

datgt_old <- datgt;
row_sub <- apply(datgt[,4:(3+3*k)], 1, function(row) all(row !=0 ));
datgt <- datgt[row_sub,];

bidpxs <- as.matrix(sweep(datgt[,(4+k):(3+2*k)], 2, num2dollars, `*`));
askpxs <- as.matrix(sweep(datgt[,(4+2*k):(3+3*k)], 2, num2dollars, `*`));
pxs <- as.matrix(sweep(datgt[,4:(3+k)], 2, num2dollars, `*`));

dates <- unique(datgt[,1]);
datest <- datgt[,1];
times <- datgt[,2];
ndays <- min(ndays, length(dates));
#rm (datgt);

#fval <- 0.1;
Pval <- 57013449745; #60000 * 1e6;

mean_pxs <- apply(pxs, 2, mean);
mean_pxs_no_n2d <- mean_pxs / num2dollars ;
# stdevs_pxdiff <- c(8.3, 8.3, 1.08, 8.3, 17.38, 15.36, 9.1, 8.65);
# 8.30  8.30  1.66  8.30 22.36 21.70 13.00 11.65
stdevs <- stdevs_pxdiff / mean_pxs_no_n2d;
fval <- k / sum(1/stdevs);

##SEEN FROM /spare/local/tradeinfo/PCAInfo/shortcode_stdev_DEFAULT.txt
#tradeCap <- c(300, 100, 20, 5, 15, 50, 30, 20);
#maxCap <- tradeCap * 3;

#capsalgo <- c('nocap', 'tcap', 'bcap');
#capc <- 1;
#for (capc in 1:3){

### Each row of the following variables for a day ###
P_const <- F;

max_pos <- NULL;
last_pos <- NULL;
closing_px <- NULL; 
last_pos_closedpx <- NULL; ## last_pos_closedpx <- last_pos * closing_px
contr_vol <- NULL;
daywisepnl <- NULL;
pxs_diff <- NULL;
pnl_diff <- NULL;
dtab <- list();
opening_pos <- rep(0, k);
acc_diff<- NULL;
daywiseacc <- NULL;

for (i in 1:ndays) {
  di <- ndays+1-i;
  print(dates[di]);
  day_pxs <- pxs[datest == dates[di],];
  
  if(sum(colSums(day_pxs)==0)){
    print("skipping");
    next;
  }
  
  day_bidpxs <- bidpxs[datest == dates[di],];
  day_askpxs <- askpxs[datest == dates[di],];
  day_times <- times[datest == dates[di]];
  
  timcum <- day_times - day_times[1];
  timhorizons <- floor(timcum / horizon_msecs);
  unik <- !duplicated(timhorizons);
  
  hor_ind <- seq_along(timhorizons)[unik];
  nhors <- length(hor_ind);
  
  pxsByStdevs <- sweep(day_pxs[hor_ind,], 2, stdevs, `*`);
  
  if (P_const) {
    positions <- (Pval * fval / k) / pxsByStdevs;
  } else {
    positions <- matrix(0, nhors, k);
    if (i!=1 && !close_EOD) {
      Pvali <- sum(opening_pos * day_pxs[hor_ind[1],]);
    } else {
      Pvali <- Pval;
    }
    positions[1,] <- (Pvali * fval / k) / pxsByStdevs[1,];
    
    for (j in 2:nhors) {
      Pvali <- sum(positions[j-1,] * day_pxs[hor_ind[j],]);
      positions[j,] <- (Pvali * fval / k) / pxsByStdevs[j,];
    }
  }
  
#   if (i==1) {
#    positions[1,] = rep(0,k);
#   }

  if (i!=1 && !close_EOD) {
    hor_diff_posit <- rbind((positions[1,] - opening_pos), (positions[2:nhors,] - positions[1:(nhors-1),]));
    hor_pxs <- day_pxs[hor_ind[1:nhors],];
    hor_askpxs <- day_askpxs[hor_ind[1:nhors],];
    hor_bidpxs <- day_bidpxs[hor_ind[1:nhors],];
    nrebal <- nhors;
    orig_pos <- last_pos[i-1,];
    orig_pos_closedpx <- last_pos_closedpx[i-1,];
  } else {
    hor_diff_posit <- positions[2:nhors,] - positions[1:(nhors-1),];
    hor_pxs <- day_pxs[hor_ind[2:nhors],];
    hor_askpxs <- day_askpxs[hor_ind[2:nhors],];
    hor_bidpxs <- day_bidpxs[hor_ind[2:nhors],];
    positions <- positions[2:nhors,];
    nrebal <- nhors-1;
    orig_pos <- rep(0, k);
    orig_pos_closedpx <- rep(0, k);
  }

  if (aggress) {
    hor_aggpxs <- matrix(0, nrebal, k);
    hor_aggpxs[hor_diff_posit>0] <- hor_askpxs[hor_diff_posit>0];
    hor_aggpxs[hor_diff_posit<0] <- hor_bidpxs[hor_diff_posit<0];
    hor_tradingpxs <- hor_aggpxs;
    hor_closingpxs <- hor_bidpxs;
  } else {
    hor_tradingpxs <- hor_pxs;
    hor_closingpxs <- hor_pxs;
  }
  
  account_day <- -1 * apply((round(hor_diff_posit) * hor_tradingpxs), 2, cumsum);

  act_pos_day <- apply(round(hor_diff_posit), 2, cumsum);
  act_pos <- sweep(act_pos_day, 2, orig_pos, `+`);

  position_closingpx <- act_pos * hor_closingpxs;
  position_closingpx_day <- sweep(position_closingpx, 2, orig_pos_closedpx, `-`);

  pnlser_day <- position_closingpx_day + account_day;
  
  opening_pos <- positions[nrebal,];
  max_pos <- rbind(max_pos, apply(act_pos, 2, max));
  last_pos <- rbind(last_pos, act_pos[nrebal,]);
  closing_px <- rbind(closing_px, hor_closingpxs[nrebal,]);
  last_pos_closedpx <- rbind(last_pos_closedpx, position_closingpx[nrebal,]);
  
  daywisepnl <- rbind(daywisepnl, pnlser_day[nrebal,]);
  contr_vol <- rbind(contr_vol, apply(abs(round(hor_diff_posit)), 2, sum));
  
  pxs_diff <- rbind(pxs_diff, apply(hor_pxs,2,diff));
  pnl_diff <- rbind(pnl_diff, rbind(pnlser_day[1,], apply(pnlser_day,2,diff)));
  acc_diff <- rbind(acc_diff, apply(account_day,2,diff));
  daywiseacc <- rbind(daywiseacc, account_day[nrebal,]);
  
  pnlser_day_diff <- rbind(pnlser_day[1,], apply(pnlser_day,2,diff));
  
  for (pk in 1:k) {
      srows <- (round(hor_diff_posit[,pk]) != 0);
      dtab_t <- cbind(dates[di],round(hor_pxs[srows,pk]), round(positions[srows,pk]), round(hor_diff_posit[srows,pk]), act_pos[srows, pk], round(pnlser_day_diff[srows, pk]));
      if (length(dtab) < pk) {
          dtab[[pk]] = dtab_t;
      } else {
          dtab[[pk]] = rbind(dtab[[pk]], dtab_t);
      }
  }
  #dtab <- cbind(pnlser, cumsum(hor_diff_posit[,4]), hor_pxs[,4]/(1348*0.02), hor_diff_posit[,4])
}

daytotalpnl <- rowSums(daywisepnl);
productpnl <- colSums(daywisepnl);

totalpnl <- sum(productpnl);
avgpnl <- productpnl / ndays;

sdpnl <- apply(daywisepnl, 2, sd);
psharpe <- sqrt(252) * avgpnl / sdpnl;

tsharpe <- sqrt(252) * sum(avgpnl) / sd(daytotalpnl);

print("avg PNL Product-wise:");
print(avgpnl);
print("Sharpe Productwise:");
print(psharpe);
print("avg PNL:");
print(sum(avgpnl));
print("Sharpe:");
print(tsharpe);

for (pk in 1:k) {
  tabfname <- paste(datF, codes[pk], sep="_");
  colnames(dtab[[pk]]) <- c("date", "Mkt Price", "Position: formula", "n:contracts", "Actual Position", "PNL for day");
  write.table(dtab[[pk]], file = tabfname, row.names=F);
}

#matplot(daywisepnl, type="l", col=1:8)
#legend("bottomright", inset=0.05, legend=c("FGBS", "FGBM", "FGBL", "FGBX", "LFR", "ZN", "ZB", "UB"), pch=1, col=1:8)

tabfname <- paste(datF, "last_position", sep="_");
write.table(last_pos, file=tabfname, row.names=F);

  
  
