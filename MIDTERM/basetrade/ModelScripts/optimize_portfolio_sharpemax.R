#!/usr/bin/env Rscript


### This script takes, as input, a data-file containing pnl-series of constituents as columns.. 
### It returns the weight-vector of the constituents in the portfolio to maximize the sharpe-ratio of the portfolio 

args <- commandArgs(trailingOnly = TRUE);
if ( length(args) < 1 ) {
    stop ( " Usage: <script> <pnls-data-file> [daywise(F/T, default:T)] [Analysis(F/T, default:F)]\n " ) ;
}

daywise = T;
if ( length(args) > 1 ) { daywise = as.logical(args[2]); }

analyze_test = F;
if ( length(args) > 2 ) { analyze_test = as.logical(args[3]); }

dat <- read.table(args[1], header=F);
row.names(dat) <- dat[,1]
dat_dates <- as.numeric(substr(row.names(dat), start=1, stop=8))
dat_wts <- dat[,2]
dat <- dat[,3:ncol(dat)]

if ( daywise ) {
  dates <- unique(sort(dat_dates))
  dat_t <- matrix(0, nrow = length(dates), ncol = ncol(dat));
  dat_wts_t <- rep(0, length(dates));

  for (dti in 1:length(dates)) {
    dat_t[dti,] <- colSums(dat[dat_dates == dates[dti],], na.rm=T);
    dat_wts_t[dti] <- max(dat_wts[dat_dates == dates[dti]]);
  }
  dat <- dat_t;
  dat_wts <- dat_wts_t;
} else {
  dat <- as.matrix(dat);
}

options(digits=3);

cat("No. of days/samples:", nrow(dat), "\n")


if (analyze_test) {
  test_inst <- 1:floor(nrow(dat)/2)
  train_inst <- (1+floor(nrow(dat)/2)):nrow(dat)

  # create Test Data
  dat_test <- dat[test_inst,]
  dat_wts_test <- dat_wts[test_inst]

  # create Data
  dat <- dat[train_inst,]
  dat_wts <- dat_wts[train_inst]
}

{
  # use L1Norm of Train-Data to normalize 
  pnl_l1norms <- apply(dat, 2, function(x) mean(abs(x)));
  cat("SD of PNLs:", pnl_l1norms, "\n");

  # normalize Data
  dat <- sweep(dat, 2, pnl_l1norms, FUN="/")

  # compute the mean, sd, cov for Train-Data
  CovM <- cov.wt(dat, wt = dat_wts); # pairwise.complete.obs
  meanV <- apply(dat, 2, function(x) weighted.mean(x, dat_wts, na.rm=T));
  sdV <- sqrt(diag(CovM$cov));
  cat("Individual sharpe:", meanV / sdV, "\n");

  lambda_vec <- c(0.2)
}

if (analyze_test) {
  # normalize Data
  dat_test <- sweep(dat_test, 2, pnl_l1norms, FUN="/")

  # compute the mean, sd, cov for Train-Data
  meanV_test <- apply(dat_test, 2, function(x) weighted.mean(x, dat_wts_test, na.rm=T));
  CovM_test <- cov.wt(dat_test, wt = dat_wts_test);
  sdV_test <- sqrt(diag(CovM_test$cov));
  cat("Test Individual sharpe:", meanV_test / sdV_test, "\n");

  lambda_vec <- 0:5 / 10;
}

get_sharpe <- function ( weights ) {
  return ( (weights %*% meanV) / sqrt(t(weights) %*% CovM$cov %*% weights) );
}

get_sharpe_test <- function ( weights ) {
  return ( (weights %*% meanV_test) / sqrt(t(weights) %*% CovM_test$cov %*% weights) );
}

get_sharpe_opt_function <- function ( weights ) {
  return ( (weights %*% meanV) / sqrt(t(weights) %*% CovM$cov %*% weights) - lambda * sum(weights^2) / sum(abs(weights))^2 );
}


nfeat <- ncol(dat);
w_init <- pnl_l1norms / sum(pnl_l1norms);
cat("w_init:", w_init, "\n");

for (lambda in lambda_vec) {
  cat("\n\nlambda:", lambda, "\n")

  res_opt <- optim ( w_init, get_sharpe_opt_function,  NULL, method="L-BFGS-B", lower=rep(0, nfeat), control = list(fnscale = -1) );

  weights <- res_opt$par / sum(res_opt$par);

  cat("Norm-Weights:", weights, "\n")

  weights <- weights / pnl_l1norms;
  weights <- weights / sum(weights);
  cat("Weights:", weights, "\n")

  ## Performance on Train-Data
  cat("Combined Sharpe:", get_sharpe(weights), "\n");

  if (analyze_test) {
    ## Performance of Test-Data:
    cat("Test Combined Sharpe:", get_sharpe_test(weights), "\n");
  }
}


#ICov <- solve(CovM$cov);
#weights <- ICov %*% meanV / sum(ICov %*% meanV);
#weights <- pmax(0, weights);

#dat[is.na(dat)] <- 0;
#combined_pnl <- dat %*% weights;
#wm <- weighted.mean(combined_pnl, dat_wts, na.rm=T);
#wsd <- sqrt(sum(dat_wts * (combined_pnl - wm)^2) / sum(dat_wts));
