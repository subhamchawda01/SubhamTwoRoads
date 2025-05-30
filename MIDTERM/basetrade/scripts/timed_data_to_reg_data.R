#!/usr/bin/env Rscript

HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");

REPO <- "basetrade";

SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");
MODELSCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/ModelScripts/", sep="");
GENPERLLIB_DIR <- paste(HOME_DIR, "/", REPO, "_install/GenPerlLib/", sep="");
LIVE_BIN_DIR <- paste(HOME_DIR, "/LiveExec/bin/", sep="");
if ( ! file.exists(LIVE_BIN_DIR )) 
{ 
    LIVE_BIN_DIR <- paste(HOME_DIR,"/", REPO,"_install/bin/", sep="");
}

TRADELOG_DIR <- "/spare/local/logs/tradelogs/";

Search_Linear <- function (Arr,index,val)
{
	len <- length(Arr)
	for(i in index:len)
	{
		if(Arr[i] >= val)
			return(i)
	}
	return(len)
}

GetTradeVolumeData <- function (Base_Time,trade_volume_filename)
{
	len <- length(Base_Time)
	trade_volume_factor <- rep(1,len)
	
	if( (trade_volume_filename == "") | (!file.exists(trade_volume_filename)) )
		return(trade_volume_factor)
	
	Trade_Data <- read.table(trade_volume_filename)

	Trade_Data <- aggregate(Trade_Data[,2]~Trade_Data[,1],data=Trade_Data,FUN=sum)
	Trade_Data[,2] <- cumsum(Trade_Data[,2])
	avg_trades_per_sec <- tail(Trade_Data[,2],1)/(tail(Trade_Data[,1],1) - Trade_Data[1,1])

	index1 <- 1;
	index2 <- 1;

	for(i in 1:len)	
	{
		base_sec <- Base_Time[i]/1000
		future_sec <- base_sec + 180	
		index1 <- Search_Linear(Trade_Data[,1],index1,base_sec)
		index2 <- Search_Linear(Trade_Data[,1],index2,future_sec)
		if( (Trade_Data[index2,1] - Trade_Data[index1,1]) < 0.000001)
			next;
		trade_volume_factor[i] <- (Trade_Data[index2,2] - Trade_Data[index1,2])/avg_trades_per_sec 
	}

	return(trade_volume_factor)
}

GetDataNewModel <- function ( Data, threshold_high,threshold_low)
{
	base <- Data[,3]
	future <- Data[,4]
	time <- Data[,1]
	len <- length(base)
	ret <- matrix( rep(0,2*len), ncol = 2)
	val <- 0	

	for(i in 1:len)
	{
		threshold_high_flag <- 0;
		pos_threshold_low_flag <- 0;
		neg_threshold_low_flag <- 0;
		j <- i+1;
		while(j <= len)
		{
			returns <- future[j] - base[i]
			if(returns >= threshold_high)
			{
				threshold_high_flag <- 1;
				break;
			}
			if(returns <= -threshold_high)
			{
				threshold_high_flag <- -1;
				break;
			}
			j <- j + 1;
			if(returns >= threshold_low)
			{
				pos_threshold_low_flag <- 1;
				next;
			}
			if(returns <= -threshold_low)
			{
				neg_threshold_low_flag <- -1;
				next;
			}
		}
		if(j > len)
		{
			ret[i,1] <- NA
			next;	
		}

		ret[i,2] <- time[j] - time[i]
		time_diff <- max((time[j] - time[i]),time_cap );
		time_diff_inv <- 1/time_diff;


		if( ( threshold_high_flag*pos_threshold_low_flag == -1 ) | ( threshold_high_flag*neg_threshold_low_flag == -1 ) )
		{
			if(filter_uncertain_values == 1) {
				ret[i,1] <- NA
			} else {
				val <- val + time_diff_inv;
			}
			next;
		}

		ret[i,1] <- returns*time_diff_inv;
		val <- val + time_diff_inv;
	}
	len_non_na_values <- sum(!is.na(ret[,1]))

	norm_factor <- len_non_na_values/val;
	ret[,1] <- ret[,1]*norm_factor
	return(ret)	 	
}	

GetDataForRegression <- function ()
{
  get_min_price_increment_ <- paste( LIVE_BIN_DIR, "get_min_price_increment", sep="");
  min_price_increment_  <- as.numeric( system (sprintf ("%s %s %s", get_min_price_increment_, shortcode, start_date), intern=TRUE));
	
  read_result <- try( Data <- read.table(t_dgen_outfile), silent = TRUE );

  	if(inherits(read_result, 'try-error'))
       	{
        	stop("Timed Data Does Not Exists")
       	}
	
	Returns_per_Delta_Time <- GetDataNewModel(Data,upper_threshold*min_price_increment_,lower_threshold_ratio*upper_threshold*min_price_increment_)
	Trade_Volume_Data <- GetTradeVolumeData(Data[,1],trade_volume_filename)

	Data [,4] <- Returns_per_Delta_Time[,1]     	    
	 	
	Trade_Volume_Data <- Trade_Volume_Data[!is.na(Data[,4])]      	
	Data <- Data[!is.na(Data[,4]),]

	base_time <- Data[,1]

	Data <- Data [,-(1:3)]	    

	factor <- Trade_Volume_Data

	if(fsudm_level == 1)
	{
		factor <- factor*abs(Data[,1])
	}

	if(fsudm_level == 2)
	{
		factor <- factor*Data[,1]^2
	}

	if(fsudm_level == 3)
	{
		factor <- factor*abs(Data[,1]^3)
	}

	Data <- sweep(Data,1,factor,"*")

	if(print_time == 1)
	{
		Data <- cbind(Data,base_time,deparse.level = 0)
	}

  dir.create(dirname(t_regdata_filename),recursive = TRUE, showWarnings = FALSE)

  write.table(Data,file = t_regdata_filename,append = TRUE, row.names = FALSE, col.names = FALSE)

}

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 8 ) {
        stop ("USAGE : <script> <shortcode> <ilist> <date> <t_dgen_outfile> <upper_threshold> <lower_threshold_ratio> <time_cap> <t_regdata_filename> <filter_uncertain_values>? <trade_volume_filename>? <print_time>? <fsudm_level>?\n");
}

shortcode <- args[1];
ilist <- args[2]
t_dgen_outfile <- args[4];
upper_threshold <- as.numeric(args[5]);
t_regdata_filename <- args[8];
lower_threshold_ratio <- as.numeric(args[6]);
time_cap <- as.numeric(args[7]);
start_date <- args[3];
filter_uncertain_values <- 0;
trade_volume_filename <- "";
print_time <- 0;
fsudm_level <- 0;

if(length(args) > 8) {
	filter_uncertain_values <- max(0,min(3, as.numeric(args[9])));
}
if(length(args) > 9) {
	trade_volume_filename <- args[10];
}
if(length(args) > 10) {
	print_time <- as.numeric(args[11]);
}
if(length(args) > 11) {
	fsudm_level <- as.numeric(args[12]);
}	

GetDataForRegression ();
