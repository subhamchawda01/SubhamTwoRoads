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

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 2)
{
  stop ( "USAGE: <script> <infile> <outfile>\n" );
}

fname = args[1];
outfname = args[2];

dta = read.table(fname);

dates = unique ( dta[,1] );
for ( dt in dates )
{
  dta_dt = dta[dta[,1]==dt, 2:ncol(dta)];
  if ( ncol(dta) == 2 ) {
     mean_sds = mean( dta_dt );
  } else {
    mean_sds = apply ( dta_dt, 2, mean );
  }
  cat ( dt, mean_sds, "\n", file=outfname, append=TRUE );
}
if ( ncol(dta) == 2 ) {
  mean_sds = mean ( dta[,2] );
} else {
  mean_sds = apply ( dta[, 2:ncol(dta)], 2, mean );
}
cat ( mean_sds, "\n" );

