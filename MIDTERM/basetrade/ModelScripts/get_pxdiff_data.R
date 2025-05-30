#!/usr/bin/env Rscript
#suppressPackageStartupMessages( library ( "numDeriv" , lib.loc="/apps/R/root/library/" ) );
.libPaths("/apps/R/root/library/")

GetListofDates <- function ( )
{
        cat(sprintf("~/basetrade/scripts/get_list_of_dates_for_shortcode.pl %s %s %s", shortcode, start_date, num_days_lookback),"\n" );
        dates <- system ( sprintf("~/basetrade/scripts/get_list_of_dates_for_shortcode.pl %s %s %s", shortcode, start_date, num_days_lookback),intern=TRUE );
        list_of_dates <<- unlist(strsplit(dates, "\\ "));
        print (list_of_dates);
}

start_date <- c();
num_days_lookback <- c();

shortcode <- c();

list_of_dates <- c();

px1d <- c();
px2d <- c();

px1 <- c();
px2 <- c();

args = commandArgs( trailingOnly=TRUE )

if ( length(args) < 4 ) {
        stop ("USAGE : <script> <shortcode> <ilist> <start_date> <num_days_lookback>\n");
}

shortcode <- args[1];
ilist <- args[2];
start_date <- args[3];
num_days_lookback <- args[4];

GetListofDates();

cat ( list_of_dates );

for ( date in list_of_dates )
{

    system (sprintf('source /home/dvctrader/.profile ;~/basetrade_install/bin/datagen %s %s EST_915 EST_1614 22222 ~/dgen_out 10000 0 0 0 0', ilist, date ));

    if ( file.info("~/dgen_out")$size <= 1 )
    {
       next;
    }

    dgen <- as.matrix(read.table ( "~/dgen_out" ));

    row_inds <- seq(1,nrow(dgen));
    rows <- row_inds[dgen[,5]*dgen[,6]!=0];

    t_px1 <- dgen[rows,5];
    t_px2 <- dgen[rows,6];

    t_px1d <- diff(t_px1);
    t_px2d <- diff(t_px2);

    px1d <- c(px1d, t_px1d);
    px2d <- c(px2d, t_px2d);

    px1 <- c( px1, t_px1[1,(length(t_px1)-1)] );
    px2 <- c( px2, t_px2[1,(length(t_px2)-1)] );
}

save ( px1, px2, px1d, px2d, file="~/pxdiff_data" );
