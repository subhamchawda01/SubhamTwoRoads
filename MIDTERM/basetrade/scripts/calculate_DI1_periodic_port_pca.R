#! /usr/bin/env Rscript
#suppressPackageStartupMessages( library ( "numDeriv" , lib.loc="/apps/R/root/library/" ) );
.libPaths("/apps/R/root/library/")

GetListofShortcodes <- function ( )
{
        lines <- readLines (offline_stir_filename);
        for ( i in c(1:length(lines)) )
        {
            if ( grepl(pattern=port, x = lines[i] ) )
            {
                t_line_words <- unlist(strsplit(lines[i], "\\ "));
                for ( j in c(2:length(t_line_words)) )
                {
                     DI_strip_shortcodes <<- c(DI_strip_shortcodes, t_line_words[j]);
                }
                break;
            }
        }        
}

GetListofDates <- function ( )
{
	shortcode <- DI_strip_shortcodes[1];
        dates <- system ( sprintf("~/basetrade/scripts/get_list_of_dates_for_shortcode.pl %s %s %s", shortcode, current_date, num_days_lookback),intern=TRUE );
        list_of_dates <<- unlist(strsplit(dates, "\\ "));
}

BuildModelFile <- function ( )
{
        cat ("MODELINIT DEPBASE NONAME\n",file=modelfile, append=TRUE );
        cat ("MODELMATH LINEAR CHANGE\n", file=modelfile, append=TRUE );
        cat ("INDICATORSTART\n", file=modelfile, append=TRUE );
        for ( i in c(1:length(pca_contracts) ) )
        {
                shc <- pca_contracts[i];
                cat ("INDICATOR 1.00 SimpleTrend", shc, period, price_type, "\n", file=modelfile, append=TRUE );
        }
        cat ("INDICATOREND\n", file=modelfile, append=TRUE );
}

GenerateData <- function ( )
{
        for ( tradingdate in list_of_dates )
        {
                system ( sprintf ( "~/basetrade_install/bin/datagen %s %s %s %s %s %s %s 0 0 0 0", modelfile, tradingdate, start_time, end_time, progid, datagenfile, msecs ), intern=FALSE  );
                t_data <- as.matrix(read.table(datagenfile));
                t_X <- t_data[,3:ncol(t_data)];
                X <<-rbind ( X, t_X );
        }
}

GenerateAndWritePCAFile <- function ( )
{
        pc_data <- X ;
        pc_comps <- princomp ( pc_data, scale=TRUE );
        pc1 <- pc_comps$loadings[,1];
        pc2 <- pc_comps$loadings[,2];
        pc3 <- pc_comps$loadings[,3];

        stdevs <- c(); 

        for ( i in c(1:ncol(pc_data)) )
        {
            stdevs <- c(stdevs,sd(pc_data[,i]));
        }

        sdev <- pc_comps$sdev;
        scores <- sdev*sdev;
        scores <- scores / sum ( scores );

        cat ( port, pca_contracts,"\n", file=pcafile, append=TRUE );
        cat ( port, "PC1", scores[1], sdev[1], pc1, "\n", file=pcafile, append=TRUE );
        cat ( port, "PC2", scores[2], sdev[2], pc2, "\n", file=pcafile, append=TRUE );
        cat ( port, "PC3", scores[3], sdev[3], pc3, "\n", file=pcafile, append=TRUE );
        cat ( port, "STDEV", stdevs, "\n", file=pcafile, append=TRUE ); 
}

list_of_dates <- c();
last_day_prices <- c();
avg_volumes <- c();
dv01_values <- c();
term_values <- c();

relative_weights <- c();

current_date <- Sys.Date() - 1;
current_date <- gsub( "-", "", current_date );

USER <- Sys.getenv("USER");

start_time <- "BRT_910"
end_time <- "BRT_1540";

progid <- 22222;
modelfile = paste ("/spare/local/",USER, "/tmp_modelfile", sep="");
datagenfile = paste ( "/spare/local/", USER, "/tmp_datagenfile",sep = "" );
pcafile = paste ( "/spare/local/", USER ,"/tmp_pcafile",sep = "" );

system ( sprintf ( "> %s", modelfile ), intern=FALSE );
system ( sprintf ( "> %s", datagenfile ), intern=FALSE );
system ( sprintf ( "> %s", pcafile ), intern=FALSE );

msecs = 1000;

period <- 600;

price_type <- "MktSizeWPrice";

offline_stir_filename <- "/spare/local/tradeinfo/StructureInfo/structured_trading.txt";

DI_strip_shortcodes <- c();
pca_contracts <- c();

X <- c();

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 1 ) {
	stop ("USAGE : <script> <port> <num_days_lookback> [start_date=current_date]\n");	
}

port <- args[1];
num_days_lookback <- args[2];

if ( length(args) > 2 )
{
	current_date <- args[3];
}

#GetListofShortcodes ( );

DI_strip_shortcodes <- c("DI1F17", "DI1F18", "DI1N18", "DI1F19", "DI1F21", "DI1F23" );

cat ( DI_strip_shortcodes,"\n");
GetListofDates ( );

pca_contracts <- DI_strip_shortcodes;

BuildModelFile ( );
GenerateData ( );
GenerateAndWritePCAFile ( );
cat ( "pca file " , pcafile, "\n" );
