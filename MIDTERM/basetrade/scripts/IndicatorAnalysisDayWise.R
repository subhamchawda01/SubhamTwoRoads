#!/usr/bin/env Rscript

HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");

REPO <- "basetrade";

WORKING_DIR <- paste("/spare/local/",USER,"/",sep="");
SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");

method <- "Pearson";

args = commandArgs( trailingOnly=TRUE )
type_index <- match('-t',args)

if(!is.na(type_index))
{
        if((type_index + 1) > length(args))
        {
                stop("Provide a value for type (-t) flag \n")
        }
        method <- args[type_index+1]
        if((method != "Pearson") && (method != "Spearman") && (method != "Kendall") && (method != "Compare")) {
                stop ("Value of type paramter must be one of the following : 1) Pearson (default) 2) Spearman 3)Kendall 4) Compare\n");
        }
        args <- args[-type_index]
        args <- args[-type_index]
}

if ( length(args) < 13 ) {
        stop ("\n\nUSAGE : <script> <shortcode> <ilist> <start_date> <num_days_lookback> <start_time> <end_time> <dgen_msecs> <:dgen_l1events> <:dgen_trades> <to_print_on_eco> <pred_duration> <pred_algo> <filter>\n\nOptional flag : \n     -t: Type of correlation to be plotted. Value must be one of Pearson (default), Spearman, Kendall and Compare(to compare all types of correlation)\n\nOutput : Sharpe, Average & Range values of correlation method for all indicators in ilist and Plots will also be visible on screen. \n\nExample: \n     ./IndicatorAnalysisDayWise.R TOPIX_0 ilist 20150805 10 JST_1645 EST_800 1000 0 0 0 900 na_e3 f0\n     ./IndicatorAnalysisDayWise.R TOPIX_0 ilist 20150805 10 JST_1645 EST_800 1000 0 0 0 900 na_e3 f0 -t Spearman\n\n");
}

shortcode <- args[1];
ilist <- args[2];
start_date <- args[3];
num_days_lookback <- as.numeric(args[4]);
start_time <- args[5];
end_time <- args[6];
dgen_msecs <- as.numeric(args[7]);
dgen_l1events <- as.numeric(args[8]);
dgen_trades <- as.numeric(args[9]);
to_print_on_eco <- as.numeric(args[10]);
pred_duration <- as.numeric(args[11]);
pred_algo <- args[12];
filter <- args[13];
time <- sub(" ", "", Sys.time(), fixed = TRUE);
dirname <- paste (WORKING_DIR,"temp",time,"/", sep="");
output_pdf <- paste(dirname,"Output.pdf",sep="")

GetFileteredRegData <- function ( dirname )
{
        sink("/dev/null")	
        dir.create(dirname);
        script_path <- paste (SCRIPTS_DIR, "get_regdata_and_correlation.R", sep="");
        system( sprintf("%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", script_path, shortcode, ilist, start_date, num_days_lookback, start_time, end_time, dgen_msecs, dgen_l1events, dgen_trades, to_print_on_eco, pred_duration, pred_algo, filter, dirname) , intern = TRUE);
	sink()
}

PlotCorrelationDayWise <- function (path, method)
{
        path1 <- paste (path,method,"_correlation_daywise_outfile", sep="");
        Data <- read.table(path1);
        pdf(output_pdf,width=7,height=5);
        days <- 1:dim(Data)[1];

        for (n in 2:dim(Data)[2])
        {
		x11()
		Indicator_name <- paste("Indicator ",n-1,sep="")
                plot(Data[,n], type = "o", main = paste(method," Correlation Stats DayWise - ",Indicator_name,sep="") , xlab = "Date" , ylab = "Correlation", ylim = c(-1,1), xaxt = "n" )
                axis(1, at=days, labels=Data[,1])
                lo <- loess(Data[,n]~days)
                lines(predict(lo),col="red",lwd=2);
		legend(x="topright", c(method, "Smoothed Curve"),lty=c(1,1),col=c("black", "red"), bg="white")
                cat('Type any key and press enter to continue: ')
        	b <- scan("stdin", character(), n=1, quiet=TRUE)
	}
        garbage <- dev.off();
}

PlotCorrelationDayWiseCompare <- function (path)
{
	path1 <- paste (path,"Pearson_correlation_daywise_outfile", sep="");
        Pearson_data <- read.table(path1);
        path2 <- paste (path,"Spearman_correlation_daywise_outfile", sep="");
        Spearman_data <- read.table(path2);
       	path3 <- paste (path,"Kendall_correlation_daywise_outfile", sep="");
       	Kendall_data<- read.table(path3);
	num_days <- dim(Pearson_data)[1];
       	pdf(output_pdf,width=7,height=5);
       	for (n in 2:dim(Pearson_data)[2])
       	{
		x11()
                Indicator_name <- paste("Indicator ",n-1,sep="")
       		plot(Pearson_data[,n], type = "o", main = paste("Correlation Stats DayWise - ",Indicator_name,sep=""), xlab = "Date" , ylab = "Correlation", ylim = c(-1,1), xaxt = "n" )
                axis(1, at=1:num_days, labels=Pearson_data[,1])
                lines(Spearman_data[,n], type = "o", lty = 2, col = "red")
                lines(Kendall_data[,n], type = "o", lty = 2, col = "blue")
		legend(x="topright", c("Pearson", "Spearman", "Kendall"),lty=c(1,2,2),col=c("black", "red", "blue"), bg="white")
        	cat('Type any key and press enter to continue: ')
        	b <- scan("stdin", character(), n=1, quiet=TRUE)
       	}
       	garbage <- dev.off();
}

GetCorrelationStats <- function(path,method)
{
        path1 <- paste (path,method,"_correlation_daywise_outfile", sep="");
        Data <- read.table(path1);
     
        for (n in 2:dim(Data)[2])
        {
        	cat (paste(method, " Correlation Statistics for Indicator ", n-1, ":\n", sep=""))
		Avg <- mean(Data[,n])
		Sharpe <- abs(Avg/(sd(Data[,n])))
		cat(paste ("       Average: ",Avg,"\n"))
		cat(paste ("       Sharpe: ",Sharpe,"\n"))
		cat(paste ("       Range: [",min(Data[,n]),",",max(Data[,n]),"]\n",sep="") )
        }
}

GetFileteredRegData ( dirname )

if(method == "Compare") {
	GetCorrelationStats(dirname,"Pearson")
	GetCorrelationStats(dirname,"Spearman")
	GetCorrelationStats(dirname,"Kendall")
	PlotCorrelationDayWiseCompare( dirname )
} else {
	GetCorrelationStats(dirname,method)
	PlotCorrelationDayWise(dirname,method)
}

unlink(dirname, recursive = TRUE); # comment this line to get regdata, correlation data and plots in spare/local/[USER]/temp[DATE][TIME] directory

