#!/usr/bin/Rscript
# It generates the stratstory for strategies. Given the config-file listing the features, 
# It finds the features that splits into good-bad pnl performance 
#

library(rpart);

GetNumLinesFile <- function (file)
{
  console.output = system( sprintf("wc -l %s", file), intern = TRUE);
  console.array = strsplit (console.output, " ");
  num.lines = as.integer (console.array[[1]][1]);
  return (num.lines);
}

FilterData <- function (data)
{
  length_data <- nrow(data);
  for(i in c(1:ncol(data))){
    length_colNA <- length(which(data[,i]=="NaN"));
    if(length_colNA > 0.4 * length_data) {
      if(i == ncol(data)) {
        cat("Highlights not generated for strat",stratname,"as pnl samples not available for more than 40 percent of points\n");
        stop();
      }
      remove_cols <<- c(remove_cols,-i);
    }
  }
  if(length(remove_cols > 0 )){
    data <- data[,remove_cols];
  }
  return(data);
}

options(scipen=999, "digits"=4);
HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");

REPO <- "basetrade";
SLACK_NOTIF <- paste(HOME_DIR, "/", "infracore_install/bin/send_slack_notification", sep="");
feature_datafile = "check";
Sys.setenv(LD_LIBRARY_PATH="/apps/gcc_versions/gcc-4_9_install/lib64/")

SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");
MODELSCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/ModelScripts/", sep="");
WF_SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/walkforward/", sep="");
LIVE_BIN_DIR <- paste(HOME_DIR,"/", REPO,"_install/bin/", sep="");
FEATURE_DIR <- paste(HOME_DIR, "/", "feature_configs/", sep="");
TRADELOG_DIR <- "/spare/local/logs/tradelogs/";

system( sprintf("mkdir -p %s", FEATURE_DIR) );
args = commandArgs( trailingOnly=TRUE );
if ( length(args) < 1) 
{
  stop("USAGE: <script> <shortcode> <features_config/INVALIDFILE> <stratname> <end-date> <numdays> [<address_lookahead=F>] [<add_to_DB=F>] [<start-hhmm=STRAT_START_HHMM>] [<end-hhmm=STRAT_END_HHMM>] \n");
}

shortcode <- args[1];
features_config <- args[2];
stratname <- args[3];
end_date <- args[4];
numdays <- args[5];

address_lookahead <- F;
if ( length(args) >= 6 ) { address_lookahead <- as.logical(args[6]); }

add_to_DB <- F;
if ( length(args) >= 7 ) { add_to_DB <- as.logical(args[7]); }

if ( length(args) >= 8 ) {
  start_hhmm <- args[8];
} else {
  start_hhmm <- system( sprintf("%s/basetrade/walkforward/process_config.py -c %s -m VIEW | grep -m1 '^START_TIME' | awk '{print $2}'", HOME_DIR, stratname), intern=T);
}

if ( length(args) >= 9 ) {
  end_hhmm <- args[9];
} else {
  end_hhmm <- system( sprintf("%s/basetrade/walkforward/process_config.py -c %s -m VIEW | grep -m1 '^END_TIME' | awk '{print $2}'", HOME_DIR, stratname), intern=T);
}


# In case the feature config is not provided, it uses these features [SSTREND,STDEV,VOL,L1SZ]
if (features_config == "INVALIDFILE" || features_config == "INVALID") {
  features_config <- paste(FEATURE_DIR,"features_config_", shortcode, sep="");
  feat_names <- c("SSTREND","STDEV","VOL","L1SZ");
  feats_mat <- cbind(shortcode, feat_names);
  write.table(feats_mat, file=features_config, row.names=F, col.names=F, quote=F);
}
feat_names <- system( sprintf("cat %s | tr ' ' '_' | grep -v '^#' | sed '/^$/d'", features_config), intern=T);

fconfig_path_tokens <- strsplit(features_config,"/")[[1]];
features_config_fname <- fconfig_path_tokens[ length(fconfig_path_tokens) ];
features_tmp <- paste(FEATURE_DIR, features_config_fname,".tmp",sep="");
system( sprintf("cp %s %s", features_config, features_tmp) );
system( sprintf("echo %s STRAT %s >> %s", shortcode, stratname, features_tmp) );


feature_datafile <- paste(FEATURE_DIR, features_config_fname,stratname,"data", sep="");

# Generate the feature data
script <- paste(HOME_DIR, "/", REPO, "_install/WKoDii/", "get_day_features.pl", sep="");
gen_feat_data_cmd <- sprintf("%s %s %s %s %s %s %s %s 1 1>%s 2>/dev/null", script, shortcode, end_date, numdays, features_tmp, "SAMPLE", start_hhmm, end_hhmm, feature_datafile);
system( gen_feat_data_cmd );

# read feature data
# date feature1 feature2 ....
# warns if #complete_rows (without NAs) < 30% of #rows
# exits if no data generated
remove_cols = c();
if(file.exists(feature_datafile) && GetNumLinesFile(feature_datafile) > 0 )
{
  feature_data_full <- read.table(feature_datafile); #feature_data_full <- as.matrix(feature_data_full);
  nrow_complete_data <- nrow(feature_data_full);
  feature_data_full <- FilterData(feature_data_full);
  feature_data_full <- feature_data_full[complete.cases(feature_data_full),];
  nrow_filter_data <- nrow(feature_data_full);
  if(nrow_filter_data < 0.3 * nrow_complete_data) {
    system(sprintf("%s strat_highlights DATA \"Highlights not generated for strat %s as filter data less than 30% of complete data for %s \"", SLACK_NOTIF, stratname, features_config));
  }
} else {
  system(sprintf("%s strat_highlights DATA \"Highlights not generated for strat %s as feature data not found for %s \"", SLACK_NOTIF, stratname, features_config));
  stop();
}

if(length(remove_cols) > 0 )
{
  feature_col_remove <- remove_cols + 1;
  feat_names <- feat_names[feature_col_remove];
}

dates <- apply(feature_data_full, 1, function(x) unlist(strsplit(as.character(x[1]),"_"))[1]);
uniqdates <- unique(dates);

# if address_lookahead, binds features(day:d) to pnl(day:d+1)
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

sharpe <- mean(daily_pnls) / sd(daily_pnls);
cat("Sharpe:", sharpe, " PnlAvg:", mean(daily_pnls), " MinPnl:", min(daily_pnls), "\n", sep="");

# randomly splits data into num_breaks parts
# for each subset and for entire set, the sharpe-diff of data must hold (abs(sharpe_diff) > 0.25 and sign(sharpe_diff) must be consistent)
# best_split_pos holds if sharpe-diff holds +ve (>0.25) for [feature_index, percententile_index]
# best_split_neg holds if sharpe-diff holds +ve (>0.25) for [feature_index, percententile_index]
# sharpe_diff has the sharpe_diff on the entire data
uniqdates <- sample(uniqdates);
folds <- cut(seq(1,length(uniqdates)),breaks=5,labels=FALSE);
set.seed(123);
num_breaks <- 3;
percentiles <- c(0.25,0.5,0.75);
best_split_pos <- matrix(1,nrow = length(feat_names), ncol = length(percentiles));
best_split_neg <- matrix(1,nrow = length(feat_names), ncol = length(percentiles));
sharpe_diff <- NULL;

for (i in c(1: ( num_breaks + 1))){
  Indexes <- which(folds==i,arr.ind=TRUE);
  if( i == num_breaks + 1) {
    train_dates <- uniqdates;
  } else {
    train_dates <- uniqdates[-Indexes];
  }
  train_ind <- dates %in% train_dates;
  t_dates <- dates[ train_ind ];
  feature_data <- feature_data_full[train_ind, 2:(ncol(feature_data_full)-1)];
  pnl_data <- feature_data_full[train_ind, ncol(feature_data_full)];
  colnames(feature_data) <- feat_names;

  first_feature_idx_ <- 1;
  last_feature_idx_ <- dim(feature_data)[2];
  num_features <- last_feature_idx_ - first_feature_idx_ + 1;

  train_data <- data.frame( y = as.numeric(pnl_data), x = feature_data );
  split_vars <- colnames(train_data[,c(2:ncol(train_data))]);

  split_value_mat <- NULL;
  sharpe_mat <- NULL;
  sharpe_diff <- NULL;
  daily_pnlavg_mat <- NULL;
  daily_minpnl_mat <- NULL;
  
  for ( percentile in percentiles)
  {
    sharpe_mat_percentile <- NULL;
    sharpe_diff_percentile <- NULL;
    daily_pnlavg_percentile <- NULL;
    daily_minpnl_percentile <- NULL;
    split_value_mat_percentile <- NULL;
    
    split_values <- array();
    for (i in c(1:num_features) ) {
      split_values[i] <- quantile(train_data[,i+1], c(percentile));
      pnls_low_idx <- train_data[split_vars[i]] < split_values[i];

      pnls_low <- pnl_data[pnls_low_idx];
      pnls_high <- pnl_data[!pnls_low_idx];
      if( length(pnls_low) == 0 || length(pnls_high) == 0 || all(pnls_low == 0) || all(pnls_high == 0) ) {
        sharpe_low <- 0 ;
        sharpe_high <- 0;
      } else {
        daily_pnls_low <- c();
        daily_pnls_high <- c();
        for (dt in train_dates) {
          daily_pnls_low <- c(daily_pnls_low, sum(pnl_data[t_dates==dt & pnls_low_idx]));
          daily_pnls_high <- c(daily_pnls_high, sum(pnl_data[t_dates==dt & !pnls_low_idx]));
        }
        
        sharpe_low <- mean(daily_pnls_low) / sd(daily_pnls_low);
        sharpe_high <- mean(daily_pnls_high) / sd(daily_pnls_high);

        daily_pnlavg_low <- mean(daily_pnls_low);
        daily_pnlavg_high <- mean(daily_pnls_high);
         
        daily_minpnl_low <- min(daily_pnls_low);
        daily_minpnl_high <- min(daily_pnls_high);
      }

      split_value_mat_percentile <- rbind( split_value_mat_percentile, split_values[i]);
      sharpe_mat_percentile <- rbind ( sharpe_mat_percentile, sprintf("%.2f:%.2f",sharpe_low,sharpe_high) );
      sharpe_diff_percentile <- rbind( sharpe_diff_percentile, sharpe_high - sharpe_low );

      daily_pnlavg_percentile <- rbind ( daily_pnlavg_percentile, sprintf("%.2f:%.2f",daily_pnlavg_low,daily_pnlavg_high) );
      daily_minpnl_percentile <- rbind ( daily_minpnl_percentile, sprintf("%.2f:%.2f",daily_minpnl_low,daily_minpnl_high) );
    }
      
    split_value_mat <- cbind(split_value_mat, split_value_mat_percentile);
    sharpe_mat <- cbind(sharpe_mat, sharpe_mat_percentile);
    sharpe_diff <- cbind(sharpe_diff, sharpe_diff_percentile);

    daily_pnlavg_mat <- cbind(daily_pnlavg_mat, daily_pnlavg_percentile);
    daily_minpnl_mat <- cbind(daily_minpnl_mat, daily_minpnl_percentile);
  }
  best_split_pos = best_split_pos * (sharpe_diff > 0.25);
  best_split_neg = best_split_neg * (sharpe_diff < -0.25);

  # cat("\nSplits performance on data:\n");
  # cat("*note: mean is per 15min sample; sharpe is normalized to daily values*\n");
  # write.table(format(sharpe_diff, justify="left"), quote=F, row.names=F, col.names=F);
}

if( ! add_to_DB ) {
  output_mat <- sharpe_mat;
  colnames(output_mat) <- percentiles;
  rownames(output_mat) <- feat_names;
  cat("\n");
  print(output_mat, quote = F, justify = "right");
  cat("\n");
}

# final_split is non_zero if [feature_index, percentile_index] has consistently -ve (< -0.25) or consistently +ve (> 0.25) sharpe_diffs
best_split <- best_split_pos + best_split_neg;
final_split <- abs(best_split * sharpe_diff);
flag = 0;

# for each feature, out of all consistent percentiles, prints the one with highest overall sharpe_diff, 
# also prints its split_value and sharpes, pnl_avgs and min_pnls for the two splits
for( i in c(1:nrow(final_split))){
  max_index <- 0;
  max_index <- which.max(final_split[i,]);
  
  if(final_split[i,max_index] > 0 && ! add_to_DB ){
    cat("Feature ", feat_names[i], " Percentile ", percentiles[max_index], " Value " , split_value_mat[i,max_index], " Sharpe ", sharpe_mat[i,max_index], sep="");
    cat(" DailyPnlAvg ", daily_pnlavg_mat[i,max_index], " DailyMinPnl ", daily_minpnl_mat[i,max_index], "\n", sep=""); 
    flag = 1;
  }
  if(final_split[i,max_index] > 0 && add_to_DB ){
    cat(feat_names[i], ";Split:", percentiles[max_index], ":",split_value_mat[i,max_index], ";Sharpe:", sharpe_mat[i,max_index], sep="");
    cat(";DailyPnlAvg:", daily_pnlavg_mat[i,max_index], ";DailyMinPnl:", daily_minpnl_mat[i,max_index], " ", sep=""); 
    flag = 1;
  }
}
if(flag == 0 && ! add_to_DB ){
  cat("No feature selected");
}
system(sprintf("rm %s %s", features_tmp, feature_datafile));
