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

#===================================get returns vector from price vector ================
GetRetCorr <- function( x, y, len )
{
  l_x_ret <- length(x) - len;
  x_ret <- rep(0, l_x_ret);
  y_ret <- rep(0, l_x_ret);
  for( i in 1:l_x_ret )
  {
    x_ret[i] <- x[i+len]/x[i] - 1.0;
    y_ret[i] <- y[i+len]/y[i] - 1.0;
  }
  return(cor(x_ret,y_ret));
}
#========================================================================================

args = commandArgs( trailingOnly=TRUE )
if( length(args) < 4 )
{
  stop ("USAGE : <candidate_file> <temp_file> <start_date> <end_date> <plot_residual Y/N - optional>\n");
}

#File with n lines where each line has a pair of candidate stocks
candidate_pairs <- read.table(args[1])

#Temporary output file
temp_file = args[2]

#Start and end times for the test -- convert to secs from epoch
start_time = GetSecsFromEpoch(args[3])
end_time = GetSecsFromEpoch(args[4])

#if arg 5 is present and Y we plot
plot_res = 0;
if(length(args) == 5 && args[5] == "Y")
{
  plot_res = 1;
}

num_pairs = length(candidate_pairs[,1])

filepath_prefix = "/NAS1/data/NSEBarData/FUT_BarData_Adjusted";

for( i in c(1:num_pairs) )
{
  stock_1 = candidate_pairs[i,1];
  stock_2 = candidate_pairs[i,2];
  stock_1_file = paste(filepath_prefix,stock_1, sep="/");
  stock_2_file = paste(filepath_prefix,stock_2, sep="/");
  #create file with close pxs -- R is using /bin/sh by default which apparently craps out with the format - hence a lot of massaging needed
  cmd_ = paste("echo \"join <(grep FF_0_0 ",stock_1_file,") <(grep FF_0_0 ",stock_2_file,") | awk '{ if(\\$1 >= ",start_time," && \\$1 <= ",end_time," ){ print \\$7, \\$18 } }' > ",temp_file, "\" | /bin/bash",sep="");
  system( cmd_ );
  
  if( file.info(temp_file)$size > 0 )
  { 
    #Get adf stats for the pair
    px_series <- read.table(temp_file);
    reg_1 = lm( px_series[,1] ~ px_series[,2] );
  reg_2 = lm( px_series[,1] ~ px_series[,2] + 0 );
  reg_3 = lm( log(px_series[,1]) ~ log(px_series[,2]) );
  reg_4 = lm( log(px_series[,1]) ~ log(px_series[,2]) + 0 );
  #run urca::ur.df test for stationarity
  t_res_1 <- urca::ur.df(reg_1$residual,type=c("none")); #tseries::adf.test(reg_1$residual);
  t_res_2 <- urca::ur.df(reg_2$residual,type=c("none")); #tseries::adf.test(reg_2$residual);
  t_res_3 <- urca::ur.df(reg_3$residual,type=c("none")); #tseries::adf.test(reg_3$residual);
  t_res_4 <- urca::ur.df(reg_4$residual,type=c("none")); #tseries::adf.test(reg_4$residual);
  #get half life values -- right now for a single setting
  y <- reg_2$residual;
  len <- length(y);
  y.lag <- y[1:len-1];
  y <- y[2:len]
  delta.y <- y - y.lag;
  regress.results <- lm( delta.y ~ y.lag );
  lambda <- summary(regress.results)$coefficients[2];
  half.life <- -log(2)/lambda;

  cat(sprintf("Stocks\t%s_%s\tStatistic: %f\tStatistic: %f\tStatistic: %f\tStatistic: %f\tHalf_Life: %f\n",stock_1,stock_2,t_res_1@testreg$coefficients[5],t_res_2@testreg$coefficients[5],t_res_3@testreg$coefficients[5],t_res_4@testreg$coefficients[5], half.life/60.0));
  if( plot_res == 1 )
  {
    x11();
    plot(resid(reg_2));
    Sys.sleep(100)
  }
  }
}
