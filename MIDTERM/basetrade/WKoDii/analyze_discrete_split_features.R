#!/usr/bin/Rscript

options(scipen=999, "digits"=4);
HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");

REPO <- "basetrade";

Sys.setenv(LD_LIBRARY_PATH="/apps/gcc_versions/gcc-4_9_install/lib64/")

SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");
MODELSCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/ModelScripts/", sep="");
LIVE_BIN_DIR <- paste(HOME_DIR,"/", REPO,"_install/bin/", sep="");

TRADELOG_DIR <- "/spare/local/logs/tradelogs/";

args = commandArgs( trailingOnly=TRUE );
if ( length(args) < 1) 
{
  stop("USAGE: <script> <shortcode> <features_config/INVALIDFILE> <stratname> <end-date> <numdays> [<start-hhmm=STRAT_START_HHMM>] [<end-hhmm=STRAT_END_HHMM>] [<address_lookahead=F>] [<%instances in training_data=0.7>] [<print_tree=0>]\n");
}

shortcode <- args[1];
features_config <- args[2];
stratname <- args[3];
end_date <- args[4];
numdays <- args[5];

stratpath <- system( sprintf("%s/print_strat_from_base.sh %s", SCRIPTS_DIR, stratname), intern=T);

address_lookahead <- F;
if ( length(args) >= 6 ) { address_lookahead <- args[6]; }

if ( length(args) >= 7 ) {
  start_hhmm <- args[7];
} else {
  start_hhmm <- system( sprintf("cut -d' ' -f6 %s", stratpath), intern=T);
  if ( length(start_hhmm) == 0 ) { start_hhmm <- "2400"; }
}

if ( length(args) >= 8 ) {
  end_hhmm <- args[8];
} else {
  end_hhmm <- system( sprintf("cut -d' ' -f7 %s", stratpath), intern=T);
  if ( length(end_hhmm) == 0 ) { end_hhmm <- "2400"; }
}

if ( features_config == "INVALIDFILE" ) {
  feat_names <- c("SSTREND","STDEV","VOL","L1SZ");
  feats_mat <- cbind(shortcode, feat_names);

  features_config <- paste("features_config_", shortcode, sep="");
  features_config_tmp <- paste(features_config, ".tmp", sep="");
  write.table(feats_mat, file=features_config_tmp, row.names=F, col.names=F, quote=F);
  system( sprintf("echo %s STRAT %s >> %s", shortcode, stratname, features_config_tmp) );
} else {
  feat_names <- system( sprintf("cut -d' ' -f2- %s | tr ' ' '_'", features_config), intern=T);

  features_config_tmp <- paste(features_config,".tmp",sep="");
  system( sprintf("cp %s %s", features_config, features_config_tmp) );
  system( sprintf("echo %s STRAT %s >> %s", shortcode, stratname, features_config_tmp) );
}

feature_datafile <- paste(features_config,stratname,"data", sep="_");
plotfname <- paste(paste(features_config,stratname,"plot",sep="_"),"png",sep=".");

script <- paste(HOME_DIR, "/", REPO, "_install/WKoDii/", "get_day_features.pl", sep="");
gen_feat_data_cmd <- sprintf("%s %s %s %s %s %s %s %s 1>%s 2>/dev/null", script, shortcode, end_date, numdays, features_config_tmp, "SAMPLE", start_hhmm, end_hhmm, feature_datafile);
cat(gen_feat_data_cmd,"\n");
system( gen_feat_data_cmd );
system( sprintf("rm %s 2>/dev/null", features_config_tmp) );

#read feature data
#date feature1 feature2 ....
feature_data_full <- read.table(feature_datafile); #feature_data_full <- as.matrix(feature_data_full); 
dates <- apply(feature_data_full, 1, function(x) unlist(strsplit(as.character(x[1]),"_"))[1]);
uniqdates <- unique(dates);

if ( address_lookahead ) {
  feature_data_full_t <- feature_data_full;
  feature_data_full <- NULL;
  for (dt in uniqdates) {
    feature_data_t <- feature_data_full_t[dates==dt, ];
    feature_data_tt <- feature_data_t[2:nrow(feature_data_t),];
    feature_data_tt[, 2:(ncol(feature_data_tt)-1)] <- feature_data_t[1:(nrow(feature_data_t)-1), 2:(ncol(feature_data_t)-1)];
    feature_data_full <- rbind(feature_data_full, feature_data_tt);
  }
  dates <- apply(feature_data_full, 1, function(x) unlist(strsplit(as.character(x[1]),"_"))[1]);
  uniqdates <- unique(dates);
}    

daily_pnls <- c();
for (dt in uniqdates) {
    daily_pnls <- c(daily_pnls, sum(feature_data_full[dates==dt, ncol(feature_data_full)]) );
}
pnl_data_full <- feature_data_full[, ncol(feature_data_full)];

numsamplesperday <- nrow(feature_data_full) / length(uniqdates);
feature_data <- feature_data_full[, 2:(ncol(feature_data_full)-1)];
pnl_data <- feature_data_full[, ncol(feature_data_full)];
colnames(feature_data) <- feat_names;

first_feature_idx_ <- 1;
last_feature_idx_ <- dim(feature_data)[2];
num_features <- last_feature_idx_ - first_feature_idx_ + 1;

train_data <- data.frame( y = as.numeric(pnl_data), x = feature_data );
#head(train_data);
#plot(train_data[,2]);

split_vars <- colnames(train_data[,c(2:ncol(train_data))]);
percentiles <- c(0.2,0.4,0.6,0.8);
split_mat <- NULL;
for ( percentile in percentiles)
{
  split_mat_percentile <- NULL;
  split_values <- array();
  for (i in c(1:num_features) ) {
    split_values[i] <- quantile(train_data[,i+1], c(percentile));
    pnls_low <- pnl_data[train_data[split_vars[i]] < split_values[i]];
    pnls_high <- pnl_data[train_data[split_vars[i]] >= split_values[i]];
    sharpe_low <- sqrt(numsamplesperday * percentile) * mean(pnls_low) / sd(pnls_low);
    sharpe_high <- sqrt(numsamplesperday * (1 - percentile)) * mean(pnls_high)/sd(pnls_high);
    if ( percentile == percentiles[1] ){
      split_mat <- rbind( split_mat, c( split_vars[i], sprintf("%.1f : %.1f || %.2f : %.2f",mean(pnls_low),mean(pnls_high),sharpe_low,sharpe_high) ) );
    }
    else {
      split_mat_percentile <- rbind ( split_mat_percentile, sprintf(" %.1f : %.1f || %.2f : %.2f  ",mean(pnls_low),mean(pnls_high),sharpe_low,sharpe_high) );
    }
  }
  if( percentile != percentiles[1]){
    split_mat <- cbind(split_mat, split_mat_percentile);
  }
}
cat("\nSplits performance on data:\n");
cat("*note: mean is per 15min sample; sharpe is normalized to daily values*\n");
write.table(format(split_mat, justify="left"), quote=F, row.names=F, col.names=F);
cat("\n");

system( sprintf("rm %s 2>/dev/null", feature_datafile) );
system( sprintf("rm %s 2>/dev/null", plotfname) );

