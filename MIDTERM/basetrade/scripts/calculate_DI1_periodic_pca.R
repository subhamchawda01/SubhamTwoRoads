#! /usr/bin/env Rscript
#suppressPackageStartupMessages( library ( "numDeriv" , lib.loc="/apps/R/root/library/" ) );
.libPaths("/apps/R/root/library/")

DI_strip_shortcodes <- c("DI1F13", "DI1J13", "DI1N13", "DI1V13", "DI1F14", "DI1J14", "DI1N14", "DI1V14", "DI1F15", "DI1J15", "DI1N15", "DI1V15", "DI1F16", "DI1J16", "DI1N16", "DI1V16",
			"DI1F17", "DI1J17", "DI1N17", "DI1V17", "DI1F18", "DI1J18", "DI1N18", "DI1V18", "DI1F19", "DI1J19", "DI1N19", "DI1V19", "DI1F20", "DI1J20", "DI1N20", "DI1V20",
			"DI1F21", "DI1J21", "DI1N21", "DI1V21", "DI1F22", "DI1J22", "DI1N22", "DI1V22", "DI1F23", "DI1J23", "DI1N23", "DI1V23", "DI1F24", "DI1J24", "DI1N24", "DI1V24",
			"DI1F25", "DI1J25", "DI1N25", "DI1V25");

DI_strip_shortcodes <- c("DI1F17", "DI1F18", "DI1N18", "DI1F19", "DI1F21", "DI1F23" );

GetListofDates <- function ( )
{
	shortcode <- DI_strip_shortcodes[1];
        dates <- system ( sprintf("~/basetrade/scripts/get_list_of_dates_for_shortcode.pl %s %s %s", shortcode, current_date, num_days_lookback),intern=TRUE );
        list_of_dates <<- unlist(strsplit(dates, "\\ "));
}

GetVolumes <- function ( )
{
        for ( i in c(1:length(DI_strip_shortcodes) ) )
        {
                shc <- DI_strip_shortcodes[i];
                t_avg_volume_str_ <- system ( sprintf ("~/basetrade/scripts/get_avg_samples.pl %s %s 40 0000 2400 0 VOL", shc, current_date), intern=TRUE );
                t_avg_volume_ <- as.numeric(unlist(strsplit(t_avg_volume_str_, " "))[3]);
                avg_volumes <<- c(avg_volumes, t_avg_volume_ );
        }
}

GetEligibleContracts <- function ( )
{
        #sorted_indices <- sort(relative_weights, decreasing=TRUE, index.return=TRUE )$ix;

        for ( i in c(1:length(relative_weights)) )
        {
                if ( relative_weights[i] > 0.001 )
                {
                        selected_contracts <<- c(selected_contracts, i );
                }
        }
        relative_weights <<- relative_weights[selected_contracts];
        relative_weights <<- relative_weights/sum(relative_weights);
}

GetContractsForPCA <- function ( )
{
	sorted_indices <- sort(avg_volumes, decreasing=TRUE, index.return=TRUE)$ix;

	for ( i in c(1:length(sorted_indices)) )
	{
		if ( ( sorted_indices[i] %in% selected_contracts  ) && ! ( sorted_indices[i] %in% pca_contracts ) )
		{
			pca_contracts <<- c ( pca_contracts, sorted_indices[i] );	
		}	
	}
}

GetFirstFContract <- function ( )
{
	sorted_indices <- sort ( term_values, index.return=TRUE )$ix;
	for ( i in c(1:sorted_indices) )
	{
		if ( sorted_indices[i] %in% selected_contracts )
		{
			if ( grepl ( "F", DI_strip_shortcodes[sorted_indices[i]] ) )
			{
				first_F_contract <<- sorted_indices[i];
				pca_contracts <<- c ( pca_contracts, first_F_contract );
				break;
			}
		}
	}
}

GetDV01 <- function ( )
{
        for ( i in c(1:length(DI_strip_shortcodes) ) )
        {
                shc <- DI_strip_shortcodes[i];
                t_dv01 <- system ( sprintf ( "~/basetrade_install/bin/p2y %s %s  2>/dev/null", shc, current_date ), intern=TRUE );                
                if ( is.na(t_dv01) )
                {
                     t_dv01 <- 0;
                }
                cat ( "shortcode " , shc , " dv01 ", t_dv01, "\n" );
                dv01_values <<- c(dv01_values, t_dv01);
        }
}

GetTerms <- function ( )
{
	for ( i in c(1:length(DI_strip_shortcodes) ) )
	{
		shc <- DI_strip_shortcodes[i];
		t_term <- system ( sprintf ( "~/basetrade_install/bin/get_di_term %s %s 2>/dev/null", current_date, shc ), intern = TRUE ); 
                if ( is.na(t_term) )
                {
                     t_term <- 0;
                }
		term_values <<- c(term_values, t_term );
	}
}

BuildModelFile <- function ( )
{
        cat ("MODELINIT DEPBASE NONAME\n",file=modelfile, append=TRUE );
        cat ("MODELMATH LINEAR CHANGE\n", file=modelfile, append=TRUE );
        cat ("INDICATORSTART\n", file=modelfile, append=TRUE );
        for ( i in c(1:length(pca_contracts) ) )
        {
                shc <- DI_strip_shortcodes[pca_contracts[i]];
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
#	for ( i in c(1:ncol(X) ) )
#	{
#		pc_data[,i] <- X[,i]*relative_weights[i];
#	}
        pc_comps <- princomp ( pc_data, scale=TRUE );
        pc1 <- pc_comps$loadings[,1];
        pc2 <- pc_comps$loadings[,2];
        pc3 <- pc_comps$loadings[,3];

        sdev <- pc_comps$sdev;
        scores <- sdev*sdev;
        scores <- scores / sum ( scores );

        cat ( "DI1ALL", DI_strip_shortcodes[pca_contracts],"\n", file=pcafile, append=TRUE );
        cat ( "DI1ALL", "PC1", scores[1], pc1, "\n", file=pcafile, append=TRUE );
        cat ( "DI1ALL", "PC2", scores[2], pc2, "\n", file=pcafile, append=TRUE );
        cat ( "DI1ALL", "PC3", scores[3], pc3, "\n", file=pcafile, append=TRUE );
	cat ( "DI1ALL", "RW", relative_weights, "\n", file=pcafile, append=TRUE );
}

list_of_dates <- c();
last_day_prices <- c();
avg_volumes <- c();
dv01_values <- c();
term_values <- c();

relative_weights <- c();

current_date <- Sys.Date() - 1;
current_date <- gsub( "-", "", current_date );

start_time <- "BRT_910"
end_time <- "BRT_1540";

progid <- 22222;
modelfile = "/spare/local/ankit/tmp_modelfile";
datagenfile = "/spare/local/ankit/tmp_datagenfile"; 
pcafile = "/spare/local/ankit/tmp_pcafile";

system ( sprintf ( "> %s", modelfile ), intern=FALSE );
system ( sprintf ( "> %s", datagenfile ), intern=FALSE );
system ( sprintf ( "> %s", pcafile ), intern=FALSE );

msecs = 1000;

period <- 600;

price_type <- "MktSizeWPrice";

selected_contracts <- c();
pca_contracts <- c();

X <- c();

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 1 ) {
	stop ("USAGE : <script> <num_days_lookback> [start_date=current_date]\n");	
}

num_days_lookback <- args[1];

if ( length(args) > 1 )
{
	current_date <- args[2];
}

GetTerms( );
GetListofDates ( );
GetVolumes ( );

cat ( "volumes\n");
cat ( avg_volumes,"\n");

GetDV01 ( );

cat ( length(avg_volumes), length(dv01_values), "\n");

dv01_vol_values <- as.numeric(avg_volumes) * as.numeric(dv01_values);

relative_weights <- dv01_vol_values / sum ( dv01_vol_values );


GetEligibleContracts ( );
GetFirstFContract ( );

GetContractsForPCA ( );

BuildModelFile ( );
GenerateData ( );
GenerateAndWritePCAFile ( );
