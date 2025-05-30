#!/usr/bin/perl

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $BASETRADEINFODIR="/spare/local/tradeinfo/";
my $PCAFILE_BASE_DIR=$BASETRADEINFODIR."PCAInfo";

my $SPARE_HOME="/spare/local/".$USER."/";
my $SPARE_LRDB_DIR=$SPARE_HOME."lrdbdata";
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $yyyymmdd_=`date +%Y%m%d`; chomp ( $yyyymmdd_ );

require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

# start 
my $USAGE="$0 [ my_pca_pairs_file_prefix ]";

my $pca_based_pairs_file_ = $PCAFILE_BASE_DIR."/my_pcapairs";
if ( $#ARGV >= 0 ) 
{
    $pca_based_pairs_file_ = $ARGV[0];
    
}

my $seconds_to_compare_ = 300; # 300 seconds convergence relationship estimated

my $this_pca_based_pairs_filename_ = $pca_based_pairs_file_.".txt";

#TODO{} make this file somewhere else
my $pca_out_ilist_filename_ = $pca_based_pairs_file_.".out.txt";

if ( !( -e $this_pca_based_pairs_filename_ ))
{
    printf STDOUT " $this_pca_based_pairs_filename_  doesnot exist \n";
    PrintStacktraceAndDie ( "File doesnot exits" );
}

#open the pca pairs file
open PCA_PAIRS_FILE, "< $this_pca_based_pairs_filename_ " or PrintStacktraceAndDie ( "Could not open $this_pca_based_pairs_filename_\n" );

open PCA_ILIST_FILE, "> $pca_out_ilist_filename_ " or PrintStacktraceAndDie ( "Could not open $pca_out_ilist_filename_ for writting \n" );

print PCA_ILIST_FILE "MODELINIT DEPBASE NONAME\n";
print PCA_ILIST_FILE "MODELMATH LINEAR CHANGE\n";
print PCA_ILIST_FILE "INDICATORSTART\n";


# Read the pca based pairs

my %indep_included_;

while ( my $this_pca_based_pairs_filename_line_ = <PCA_PAIRS_FILE> )
{
    my @this_pca_based_pairs_filename_line_words_ = split ( ' ', $this_pca_based_pairs_filename_line_ );
    if ( $#this_pca_based_pairs_filename_line_words_ >= 1 ) 
    {
	for ( my $idx = 0 ; $idx <= $#this_pca_based_pairs_filename_line_words_ ; $idx ++ )
	{
	    my $indep_shortcode_ = $this_pca_based_pairs_filename_line_words_[$idx];
	    if ( ! exists ( $indep_included_{$indep_shortcode_} ) )
	    {
		printf PCA_ILIST_FILE "INDICATOR 1.00 SimpleTrend %s %d MktSizeWPrice\n", $indep_shortcode_, $seconds_to_compare_;
		$indep_included_{$indep_shortcode_} = 1;
	    }
	}
    }

}

close PCA_PAIRS_FILE ;

print PCA_ILIST_FILE "INDICATOREND\n";
close PCA_ILIST_FILE;

#run the exec
my $out_file= $pca_based_pairs_file_.".out.datagen.txt";
my $tradingdate_ = CalcPrevWorkingDateMult ( $yyyymmdd_, 1 );
my $datagen_start_hhmm_utc_ = "EST_800";
my $datagen_end_hhmm_utc_ =  "EST_1600";
my $datagen_msecs_timeout_ = 1000;
my $datagen_l1events_timeout_ = 0;
my $datagen_num_trades_timeout_ = 0;
my $to_print_on_economic_times_ = 0;
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;


my $exec_cmd = "$LIVE_BIN_DIR/datagen $pca_out_ilist_filename_ $tradingdate_ ".$datagen_start_hhmm_utc_." ".$datagen_end_hhmm_utc_." $unique_gsm_id_ $out_file $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";


print STDOUT "$exec_cmd\n";
