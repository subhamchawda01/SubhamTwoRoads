#!/usr/bin/env Rscript
#nonrobust
.libPaths(c("/usr/lib64/R/library","/home/apps/R/R-3.1.0/library" ))

#=====================================convert YYYYMMDD to secs from epoch ================
GetSecsFromEpoch <- function ( x ) 
{
  date_ = as.Date(as.character(x), "%Y%m%d")
  t_var_ = as.POSIXct(date_)
  return( as.integer(t_var_) )
}
#========================================================================================

args = commandArgs( trailingOnly=TRUE )
if( length(args) < 2 )
{
  stop ("USAGE : <candidate_file> < duration_months> <temp_file> \n");
}

#File with n lines where each line has a pair of candidate stocks
candidate_pairs <- read.table(args[1])

#Temporary output file
temp_file = args[3]

#test duration
test_duration <-as.numeric(args[2]);
dates <- as.character( as.Date( seq(as.Date("2006-01-01"), as.Date("2015-12-01"), by="month"), "%Y-%m-%d" ), "%Y%m%d" );
num_test_per_pair <- length(dates) - test_duration;

num_pairs = length(candidate_pairs[,1])

filepath_prefix = "/NAS1/data/NSEBarData/FUT_BarData_Adjusted";

for( i in c(1:num_pairs) )
{
  stock_1 = candidate_pairs[i,1];
  stock_2 = candidate_pairs[i,2];
  stock_1_file = paste(filepath_prefix,stock_1, sep="/");
  stock_2_file = paste(filepath_prefix,stock_2, sep="/");
  #create file with close pxs -- R is using /bin/sh by default which apparently craps out with the format - hence a lot of massaging needed
  cmd_ = paste("echo \"join <(grep FF_0_0 ",stock_1_file,") <(grep FF_0_0 ",stock_2_file,") | awk '{ print \\$1, \\$7, \\$18 }' > ",temp_file, "\" | /bin/bash",sep="");
  system( cmd_ );
  
  if( file.info(temp_file)$size > 0 )
  { 
    px_series <- read.table(temp_file);
    for( j in c(1:num_test_per_pair) )
    {
      start_secs <- GetSecsFromEpoch(dates[j]);
      end_secs <- GetSecsFromEpoch(dates[j+test_duration]);
      px_series_selected <- subset(px_series, px_series$V1 >= start_secs & px_series$V1 < end_secs ); #px_series[(px_series$V1 >= start_secs) & (px_series$V1 < end_secs) ];
      if(length(px_series_selected$V1) > 1 )
      {
        #Get adf stats for the pair
        reg_1 = lm( px_series_selected[,2] ~ px_series_selected[,3] + 0 );
        reg_2 = lm( log(px_series_selected[,2]) ~ log(px_series_selected[,3]) + 0 );
        #run urca::ur.df test for stationarity
        t_res_1 <- urca::ur.df(reg_1$residual,type=c("none")); #tseries::adf.test(reg_1$residual);
        t_res_2 <- urca::ur.df(reg_2$residual,type=c("none")); #tseries::adf.test(reg_2$residual);
        #get half life values -- right now for a single setting
        y <- reg_1$residual;
        len <- length(y);
        y.lag <- y[1:len-1];
        y <- y[2:len]
        delta.y <- y - y.lag;
        regress.results <- lm( delta.y ~ y.lag );
        lambda <- summary(regress.results)$coefficients[2];
        half.life <- -log(2)/lambda;

        #get normalized price error metric
        px_series_selected[,2] <- px_series_selected[,2]*(100.0/px_series_selected[1,2]);
        px_series_selected[,3] <- px_series_selected[,3]*(100.0/px_series_selected[1,3]);
        z <- px_series_selected[,2] - px_series_selected[,3];
        sqr_err <- sum( z*z )/len;

        cat(sprintf("Stocks\t%s_%s\tSDate: %s\tEDate: %s\tStatistic: %f\tStatistic: %f Norm_Diff: %f\t Sprd_Var:%f\t Half_Life: %f\n",stock_1,stock_2,dates[j],dates[j+test_duration],t_res_1@testreg$coefficients[5],t_res_2@testreg$coefficients[5], sqr_err, sqrt(var(y)), half.life/60.0));
      }
    }
  }
}
