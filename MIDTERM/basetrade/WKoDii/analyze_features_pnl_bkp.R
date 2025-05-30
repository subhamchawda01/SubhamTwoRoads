#!/usr/bin/Rscript
##!/apps/R/root/bin/Rscript
library(rpart);
#library(rpart.plot);

options(scipen=999, "digits"=4);
HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");

REPO <- "basetrade";

Sys.setenv(LD_LIBRARY_PATH="/apps/gcc_versions/gcc-4_9_install/lib64/")

SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");
MODELSCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/ModelScripts/", sep="");
LIVE_BIN_DIR <- paste(HOME_DIR,"/", REPO,"_install/bin/", sep="");

TRADELOG_DIR <- "/spare/local/logs/tradelogs/";

labeldata <- function ( data, bin_per, start_label = 0 )
{
  out <- array( start_label, length(data) );
  num_per <- length(bin_per);
  if (num_per > 0) 
  {
    bin_bound <- as.numeric(quantile(data, bin_per));
    out[data >= bin_bound[num_per]] <- start_label + num_per;
    if ( num_per > 1 )
    {
      for ( i in seq ( 2, num_per ) )
      {
        out[(data >= bin_bound[i-1]) & (data < bin_bound[i])] <- start_label + i -1;
      }
    }
  }
  return (out);
}

valtolabel <- function ( vals, bin_bound, start_label = 0 )
{
  out <- array( start_label, length(vals) );
  num_per <- length(bin_bound);
  if (num_per > 0) 
  {
    out[vals >= bin_bound[num_per]] <- start_label + num_per;
    if ( num_per > 1 )
    {
      for ( i in seq ( 2, num_per ) )
      {
        out[(vals >= bin_bound[i-1]) & (vals < bin_bound[i])] <- start_label + i -1;
      }
    }
  }
  return (out);
}

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
if ( length(args) >= 6 ) {
  start_hhmm <- args[6];
} else {
  start_hhmm <- system( sprintf("cut -d' ' -f6 %s", stratpath), intern=T);
  if ( length(start_hhmm) == 0 ) { start_hhmm <- "2400"; }
}

if ( length(args) >= 7 ) {
  end_hhmm <- args[7];
} else {
  end_hhmm <- system( sprintf("cut -d' ' -f7 %s", stratpath), intern=T);
  if ( length(end_hhmm) == 0 ) { end_hhmm <- "2400"; }
}

address_lookahead <- F;
if ( length(args) >= 8 ) { address_lookahead <- args[8]; }

train_frac <- 0.7;
if ( length(args) >= 9 ) { train_frac <- as.numeric(args[9]); }

print_tree <- FALSE;
if ( length(args) >= 10 ) { print_tree <- args[10]; }

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
#slots <- apply(feature_data_t, 1, function(x) unlist(strsplit(as.character(x[1]),"_"))[2]);
    feature_data_tt <- feature_data_t[2:nrow(feature_data_t),];
    feature_data_tt[, 2:(ncol(feature_data_tt)-1)] <- feature_data_t[1:(nrow(feature_data_t)-1), 2:(ncol(feature_data_t)-1)];
    feature_data_full <- rbind(feature_data_full, feature_data_tt);
  }
  dates <- apply(feature_data_full, 1, function(x) unlist(strsplit(as.character(x[1]),"_"))[1]);
  uniqdates <- unique(dates);
}    
#print(summary(feature_data_full));

daily_pnls <- c();
for (dt in uniqdates) {
    daily_pnls <- c(daily_pnls, sum(feature_data_full[dates==dt, ncol(feature_data_full)]) );
}
pnl_data_full <- feature_data_full[, ncol(feature_data_full)];

set.seed(123);
train_dates <- sample( uniqdates, size = train_frac * length(uniqdates) );
train_ind <- dates %in% train_dates;

numsamplesperday <- nrow(feature_data_full) / length(uniqdates);
#cat("numsamplesperday: ", numsamplesperday, "\n");

cat("Daily PNL mean:", mean(daily_pnls), ", sd:", sd(daily_pnls), "sharpe:", mean(daily_pnls) / sd(daily_pnls), "\n");
cat("Samples PNL mean:", mean(pnl_data_full), ", sd:", sd(pnl_data_full), 
    "sharpe(normalized to daily_sharpe):", sqrt(numsamplesperday) * mean(pnl_data_full) / sd(pnl_data_full), "\n");

feature_data <- feature_data_full[train_ind, 2:(ncol(feature_data_full)-1)];
pnl_data <- feature_data_full[train_ind, ncol(feature_data_full)];
colnames(feature_data) <- feat_names;

test_feature_data <- feature_data_full[!train_ind, 2:(ncol(feature_data_full)-1)];
test_pnl_data <- feature_data_full[!train_ind, ncol(feature_data_full)];
colnames(test_feature_data) <- feat_names;

first_feature_idx_ <- 1;
last_feature_idx_ <- dim(feature_data)[2];
num_features <- last_feature_idx_ - first_feature_idx_ + 1;

#binning features
#{
#  binned_feature_data <- matrix(0,dim(feature_data)[1],num_features);
#  binned_test_feature_data <- matrix(0,dim(test_feature_data)[1],num_features);
#  feature_bin_per <- c(0.25,0.75);
#  for( i in 1:num_features )
#  {
#    bin_bound <- as.numeric(quantile(feature_data[,first_feature_idx_+i-1], feature_bin_per));
#    binned_feature_data[,i] <- valtolabel(feature_data[,first_feature_idx_+i-1], bin_bound);
#    binned_test_feature_data[,i] <- valtolabel(test_feature_data[,first_feature_idx_+i-1], bin_bound );
#  }
#}

#binning pnl
#{ 
#  pnl_bin_per <- c(0.1,0.35,0.65,0.9);
#  pnl_start_label_ <- -2;
#
#  pnl_bin_bound <- as.numeric(quantile(pnl_data, pnl_bin_per));
#  binned_pnl_data <- valtolabel(pnl_data, pnl_bin_bound, pnl_start_label_);
#  binned_test_pnl_data <- valtolabel(test_pnl_data, pnl_bin_bound, pnl_start_label_);
#}

#generating training data
train_data <- data.frame( y = as.numeric(pnl_data), x = feature_data );
test_data <- data.frame( y = as.numeric(test_pnl_data), x = test_feature_data );

#training
#train_data$y <- factor(train_data$y);
#pnl_classes <- as.numeric(levels(train_data$y));
#loss_matrix <- matrix(0, length(pnl_classes), length(pnl_classes));
#for ( i in 1:length(pnl_classes) ) { loss_matrix[i,] <- abs(pnl_classes - pnl_classes[i]); }
#fit <- rpart( y ~ ., train_data , method="class", parms=list(split="information", loss=loss_matrix), cp=0.001, minsplit=10 );
minbucket_lim <- round(nrow(train_data) * 0.2);
fit <- rpart( y ~ ., train_data, method="anova", minbucket=minbucket_lim, cp =0.001, maxdepth=2);

split_vars <- rownames(fit$splits);
split_values <- fit$splits[,4];
init_splits <- which(fit$splits[,1]==nrow(train_data));

#print train-data evaluation
{
  split_mat <- NULL;
  for (i in init_splits ) {
    pnls_low <- pnl_data[train_data[split_vars[i]] < split_values[i]];
    pnls_high <- pnl_data[train_data[split_vars[i]] >= split_values[i]];
    sharpe_low <- sqrt(numsamplesperday) * mean(pnls_low) / sd(pnls_low);
    sharpe_high <- sqrt(numsamplesperday) * mean(pnls_high)/sd(pnls_high);

    split_mat <- rbind( split_mat, c( split_vars[i], split_values[i], sprintf("len:: %d: %d",length(pnls_low),length(pnls_high)), sprintf("mean:: %.1f: %.1f",mean(pnls_low),mean(pnls_high)), sprintf("sharpe:: %.2f: %.2f",sharpe_low,sharpe_high) ) );
  }

  cat("\nSplits performance on train-data:\n");
  cat("*note: mean is per 15min sample; sharpe is normalized to daily values*\n");
  write.table(format(split_mat, justify="left"), quote=F, row.names=F, col.names=F);
  cat("\n");

  out <- predict( fit, train_data );
  mse <- sqrt(mean( (train_data$y - out)^2 ));
  cat("\nTree_MSE:", mse,"\n");
  rsq <- 1 - (mean ( (train_data$y - out)^2 ) / mean ( (train_data$y - mean (train_data$y) )^2 ) );
  cat("Tree_Rsq:", rsq, "\n");
}

#print test-data evaluation
{
  split_mat <- NULL;
  for (i in init_splits ) {
    pnls_low <- test_pnl_data[test_data[split_vars[i]] < split_values[i]];
    pnls_high <- test_pnl_data[test_data[split_vars[i]] >= split_values[i]];
    sharpe_low <- sqrt(numsamplesperday) * mean(pnls_low) / sd(pnls_low);
    sharpe_high <- sqrt(numsamplesperday) * mean(pnls_high)/sd(pnls_high);

    split_mat <- rbind( split_mat, c( split_vars[i], split_values[i], sprintf("len:: %d: %d",length(pnls_low),length(pnls_high)), sprintf("mean:: %.1f: %.1f",mean(pnls_low),mean(pnls_high)), sprintf("sharpe:: %.2f: %.2f",sharpe_low,sharpe_high) ) );
  }

  cat("\nSplits performance on test-data:\n");
  cat("*note: mean is per 15min sample; sharpe is normalized to daily values*\n");
  write.table(format(split_mat, justify="left"), quote=F, row.names=F, col.names=F);
  cat("\n");

  out <- predict( fit, test_data );
  mse <- sqrt(mean( (test_data$y - out)^2 ));
  cat("\nOutofSample_Tree_MSE:", mse,"\n");
}

if ( print_tree ) {
  png(plotfname);
  rpart.plot(fit);
  dev.off();
  cat("\nTree:\n");
  print(fit);
  cat("\n");
  system(paste("eog ",plotfname));
}

system( sprintf("rm %s 2>/dev/null", feature_datafile) );
system( sprintf("rm %s 2>/dev/null", plotfname) );

