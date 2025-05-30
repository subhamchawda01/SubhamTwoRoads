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
    LIVE_BIN_DIR <- paste(HOME_DIR,"/", REPO,"_install/bin/", sep="")
}

WORKING_DIR <- paste("/spare/local/",USER,"/",sep="");

Sys.setenv(LD_LIBRARY_PATH="/apps/gcc_versions/gcc-4_9_install/lib64/")

#OptionalArguments
dgen_msecs <- 1000;
dgen_l1events <- 0;
dgen_trades <- 0;
to_print_on_eco <- 0;
pred_duration <- 900;
pred_algo <- "na_e3";
filter <- "f0";
mode <- 0;
method <- "Pearson"

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

mode_index <- match('-m',args)

if(!is.na(mode_index))
{
	if((mode_index + 1) > length(args))
        {
                stop("Provide a value for mode (-m) flag")
        }
	mode <- as.numeric(args[mode_index+1])
	args <- args[-mode_index]
	args <- args[-mode_index] 
}

if ( length(args) < 7 ) {
        stop ("\n\nUSAGE : <script> <shortcode> <ilist> <start_date> <num_days_lookback> <start_time> <end_time> <num_bins_for_percentiles_plot> <optional:dgen_msecs> <optional:dgen_l1events> <optional:dgen_trades> <optional:to_print_on_eco> <optional:pred_duration> <optional:pred_algo> <optional:filter>\n\nOptional flags: \n     -m: Mode of operation : 0 for Indicator Distribution Plot (default), 1 for Indicator Correlation Percentiles Plot , 2 for Returns Correlation Percentiles Plot and 3 for Indicator Comparison Plot on Return Percentiles\n     -t: Type of correlation to be plotted. Value must be one of Pearson (default), Spearman, Kendall and Compare(to compare all types of correlation)\n\nOutput : Plots will be visible on the screen.\n\nExample:\n     ./Indicator_Analysis_Utility.R TOPIX_0 ilist 20150805 100 JST_1645 EST_800 20 -m 1 -t Compare\n     ./Indicator_Analysis_Utility.R TOPIX_0 ilist 20150805 100 JST_1645 EST_800 20 1000 0 0 0 900 na_e3 fsg3 -m 2 -t Kendall\n\n");
}

shortcode <- args[1];
ilist <- args[2];
start_date <- args[3];
num_days_lookback <- as.numeric(args[4]);
start_time <- args[5];
end_time <- args[6];
bins <- as.numeric(args[7]);
time <- sub(" ", "", Sys.time(), fixed = TRUE);
dirname <- paste (WORKING_DIR,"temp",time,"/", sep="");
output_pdf <- paste(dirname,"Output.pdf",sep="")

if( length(args) >10 )
{
	dgen_msecs <- as.numeric(args[8]);
	dgen_l1events <- as.numeric(args[9]);
	dgen_trades <- as.numeric(args[10]);
	to_print_on_eco <- as.numeric(args[11]);
	if(length(args) > 13)
	{
		pred_duration <- as.numeric(args[12]);
		pred_algo <- args[13];
		filter <- args[14];
	}
}


GetFileteredRegData <- function ( dirname )
{
	sink("/dev/null")
	dir.create(dirname);
	script_path <- paste (SCRIPTS_DIR, "get_regdata.py", sep="");
	system( sprintf("%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", script_path, shortcode, ilist, start_date, num_days_lookback, start_time, end_time, dgen_msecs, dgen_l1events, dgen_trades, to_print_on_eco, pred_duration, pred_algo, filter, dirname) , intern = TRUE);
	sink()
}

GetIndicatorDistribution <- function (i,bins=5000,Name="Indicator")
{
	x11()
        b <- bins;
	h <- hist(i,breaks=b,col="red", main=paste("Distribution Histogram for ",Name,sep="") );
	xfit<-seq(min(i),max(i),length=40)
        yfit<-dnorm(xfit,mean=mean(i),sd=sd(i)) 
        yfit <- yfit*diff(h$mids[1:2])*length(i) 
        lines(xfit, yfit, col="blue", lwd=2)
}

PlotIndicatorsFromFile <- function ( ind_data )
{
	pdf(output_pdf,width=7,height=5)
	for (n in 2:dim(ind_data)[2])
	{
		Indicator_Name <- paste("Indicator ",n-1,sep="")
		i <- ind_data[,n]
		remove <- c(0)
		i <- i[! i %in% remove]
		GetIndicatorDistribution (i,5000,Indicator_Name);
	}
	cat('Type any key and press enter to exit: ')
	b <- scan("stdin", character(), n=1, quiet=TRUE)
	garbage <- dev.off()
}

GetCorrelationData <- function(sorted_column,dependent,bins=20,xlabel,type="Pearson", x_axis_unit=1, Indicator_Name="Indicator")
{
	type_lower <- tolower(type)
        range_table <- hist(sorted_column, breaks=bins , plot = FALSE)
        bins <- length(range_table$counts)

        Data <- rep(0,bins);

        NumberPoints <- range_table$counts
        start <- 1;

	axis_labels <- format( round(range_table$mids/x_axis_unit,3), nsmall = 3) 
        gap <- (range_table$breaks[2] - range_table$breaks[1])/x_axis_unit
	gap <- format( round(gap,3), nsmall = 3)

        for (i in 1:bins)
        {
                end <- NumberPoints[i] + start - 1;
                data_ind <- sorted_column[start:end];
                data_ret <- dependent[start:end];
                Data[i] <- cor(data_ret,data_ind,method=type_lower);
                start <- end + 1;
        }

        xlabel_new <- paste("Mid Values of the ",xlabel, " Range with width ", gap,sep="")
	x11()
        plot(Data,type = "o", main = paste(type," Correlation Stats for Different range - ",Indicator_Name,sep=""), xlab = xlabel_new , ylab = "Correlation", ylim = c(-1,1), xaxt = "n" )
        axis(1, at=1:bins, labels=axis_labels)
	usr <- par("usr")
	x_axis_unit <- format( round(x_axis_unit,2), nsmall = 2)
	text( usr[2], usr[3], paste(xlabel," Value = ",x_axis_unit,sep=""), adj = c( 1, 0 ), col = "red" )
}

GetCorrelationDataAll <- function(sorted_column,dependent,bins=20,xlabel,x_axis_unit=1, Indicator_Name="Indicator")
{

	range_table <- hist(sorted_column, breaks=bins , plot = FALSE)
	bins <- length(range_table$counts)
	
	Pearson <- rep(0,bins);
	Spearman <- rep(0,bins);
	Kendall <- rep(0,bins);
	
	NumberPoints <- range_table$counts	
	start <- 1;

        axis_labels <- format( round(range_table$mids/x_axis_unit,3), nsmall = 3)
        gap <- (range_table$breaks[2] - range_table$breaks[1])/x_axis_unit
        gap <- format( round(gap,3), nsmall = 3)

	for (i in 1:bins)
	{
		end <- NumberPoints[i] + start - 1;
		data_ind <- sorted_column[start:end];
		data_ret <- dependent[start:end];
		Pearson[i] <- cor(data_ret,data_ind,method="pearson");
		Spearman[i] <- cor(data_ret,data_ind,method="spearman");
		Kendall[i] <- cor(data_ret,data_ind,method="kendall");
		start <- end + 1; 
	}

	xlabel_new <- paste("Mid Values of the ",xlabel, " Range with width ", gap, sep="") 
	x11()
	plot(Pearson, type = "o", main = paste("Correlation Stats for Different range - ",Indicator_Name,sep=""), xlab = xlabel_new , ylab = "Correlation", ylim = c(-1,1), xaxt = "n" )
	axis(1, at=1:bins, labels=axis_labels)
	lines(Spearman, type = "o", lty = 2, col = "red") 
	lines(Kendall, type = "o", lty = 2, col = "blue")
	legend(x="topright", c("Pearson", "Spearman", "Kendall"),lty=c(1,2,2),col=c("black", "red", "blue"), bg="white", cex = 0.75)
	usr <- par("usr")
        x_axis_unit <- format( round(x_axis_unit,2), nsmall = 2)
        text( usr[2], usr[3], paste(xlabel," Value = ",x_axis_unit,sep=""), adj = c( 1, 0 ), col = "lightgoldenrod4" ) 		
}

PlotCorrelationDataIndicatorAxis <- function (bins=20,ind_data,method="Pearson")
{
	pdf(output_pdf,width=7,height=5)
	if (method == "Compare") {
		for (n in 2:dim(ind_data)[2])
		{
			Indicator_Name <- paste("Indicator ",n-1,sep="")
			ind_data <- ind_data[order(ind_data[,n]),]
			y <- ind_data[,1];
			indicator <- ind_data[,n]
                        std_dev <- sd(indicator)
			GetCorrelationDataAll (indicator,y,bins,"Indicator Standard Deviation",std_dev,Indicator_Name);
		}
	} else {
		for (n in 2:dim(ind_data)[2])
                {
                        Indicator_Name <- paste("Indicator ",n-1,sep="")
                        ind_data <- ind_data[order(ind_data[,n]),]
                        y <- ind_data[,1];
                        indicator <- ind_data[,n]
			std_dev <- sd(indicator)
                        GetCorrelationData (indicator,y,bins,"Indicator Standard Deviation",method,std_dev,Indicator_Name);
                }
	}
	cat('Type any key and press enter to exit: ')
	b <- scan("stdin", character(), n=1,quiet=TRUE)
	garbage <- dev.off()
}

PlotCorrelationDataReturnAxis <- function (bins=20,ind_data,method="Pearson")
{
	script_path <- paste (LIVE_BIN_DIR, "get_min_price_increment", sep="");
	min_price_increment <- system(sprintf("%s %s %s", script_path,shortcode,start_date) , intern = TRUE);
	min_price_increment <- as.numeric(min_price_increment);

	ind_data <- ind_data[order(ind_data[,1]),];
	y <- ind_data[,1];
	pdf(output_pdf,width=7,height=5)
	if (method == "Compare") {
		for (n in 2:dim(ind_data)[2])
		{	
                        Indicator_Name <- paste("Indicator ",n-1,sep="")
			indicator <- ind_data[,n];
			GetCorrelationDataAll (y,indicator,bins,"Returns Tick",min_price_increment,Indicator_Name);
		}
	} else {
		for (n in 2:dim(ind_data)[2])
                {
                        Indicator_Name <- paste("Indicator ",n-1,sep="")
                        indicator <- ind_data[,n];
                        GetCorrelationData (y,indicator,bins,"Returns Tick",method,min_price_increment,Indicator_Name);
                }
	}
        cat('Type any key and press enter to exit: ')
	b <- scan("stdin", character(), n=1,quiet=TRUE)
	garbage <- dev.off()
}

CompareIndicatorsOnReturnsAxis <- function (ind_data, bins=20,method="Pearson")
{
        script_path <- paste (LIVE_BIN_DIR, "get_min_price_increment", sep="");
        min_price_increment <- system(sprintf("%s %s %s", script_path,shortcode,start_date) , intern = TRUE);
        min_price_increment <- as.numeric(min_price_increment);

	Indicator_Name <- c("Indicator 1","Indicator 2","Indicator 3","Indicator 4","Indicator 5")
	Indicator_Colour <- c("Black","Blue","Red","darkgreen","deeppink4")

	ind_data <- ind_data[order(ind_data[,1]),]
	y <- ind_data[,1]
	Indicator_data <- ind_data[,2]
	num_indicators <- dim(ind_data)[2] - 1

        type_lower <- tolower(method)

	range_table <- hist(y, breaks=bins , plot = FALSE)
        bins <- length(range_table$counts)

	Data <- rep(0,bins)
	NumberPoints <- range_table$counts
        start <- 1;

        axis_labels <- format( round(range_table$mids/min_price_increment,3), nsmall = 3)
        gap <- (range_table$breaks[2] - range_table$breaks[1])/min_price_increment
        gap <- format( round(gap,3), nsmall = 3)

        for (i in 1:bins)
        {
                end <- NumberPoints[i] + start - 1;
                data_ind <- y[start:end];
                data_ret <- Indicator_data[start:end];
                Data[i] <- cor(data_ret,data_ind,method=type_lower);
                start <- end + 1;
        }

        xlabel_new <- paste("Mid Values of the Returns Tick Range with width ", gap,sep="")
        x11()
        plot(Data,type = "o",main = paste("Indicator Comparison - ",method," Correlation Stats",sep=""), xlab = xlabel_new, ylab = "Correlation", ylim = c(-1,1), xaxt = "n" )

	for(n in 2:num_indicators)
	{
		Indicator_data = ind_data[,n+1]
		start <- 1
		for (i in 1:bins)
        	{
                	end <- NumberPoints[i] + start - 1;
               		data_ind <- y[start:end];
                	data_ret <- Indicator_data[start:end];
                	Data[i] <- cor(data_ret,data_ind,method=type_lower);
                	start <- end + 1;
        	}
		lines(Data, type = "o", lty = 2, col = Indicator_Colour[n])
	}

        axis(1, at=1:bins, labels=axis_labels)
        usr <- par("usr")
        x_axis_unit <- format( round(min_price_increment,2), nsmall = 2)
        text( usr[2], usr[3], paste("Returns Tick Value = ",x_axis_unit,sep=""), adj = c( 1, 0 ), col = "lightgoldenrod4" )
	
	legend(x="topright",Indicator_Name[1:num_indicators],lty=c(1,2,2,2,2),col=Indicator_Colour[1:num_indicators], bg="white", cex = 0.75)
	cat('Type any key and press enter to exit: ')
        b <- scan("stdin", character(), n=1,quiet=TRUE)
        garabage <- dev.off()
}

GetFileteredRegData( dirname );

path <- paste (dirname,"filtered_regdata_filename", sep="");
ind_data <- read.table(path);

if(mode == 0) {
	PlotIndicatorsFromFile( ind_data );
} else {
	if(mode == 1) {
		PlotCorrelationDataIndicatorAxis(bins,ind_data,method);
	} else {
		if(mode == 2) {
			PlotCorrelationDataReturnAxis(bins,ind_data,method);
		} else {
			CompareIndicatorsOnReturnsAxis(ind_data,bins,method)
		}
	}
}

unlink(dirname, recursive = TRUE);        # comment this line to get the regdata data and plots in /spare/local/[USER]/temp[DATE][TIME] directory

