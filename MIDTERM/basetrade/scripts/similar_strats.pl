#!/usr/bin/perl

# Script to compute the pnl-sample correlations b/w all strat-pairs 
# for all pools for a shortcode
# This script is supposed to be crontabbed to run periodically 
# to maintain the correlations in DB

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use Math::Complex; # sqrt
use FileHandle;
use POSIX;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";

my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats
require "$GENPERLLIB_DIR/sample_pnl_corr_utils.pl";
require "$GENPERLLIB_DIR/stratstory_db_access_manager.pl"; #AddCorrToDB

my $USAGE = "$0 SHORTCODE [0/1:Update_existing_results] [START_TIME-END_TIME/-1 for all SHC dirs] [DATE=TODAY-1] [NUM_DAYS=200] ";
if ( $#ARGV < 1 ) { print "$USAGE $#ARGV\n"; exit ( 0 ); }

my $shortcode_ = shift;
my $update_existing_ = shift || 1;
my $timeperiod_ = shift;
my $yyyymmdd_ = shift || "TODAY-1";
$yyyymmdd_ = GetIsoDateFromStrMin1($yyyymmdd_);
my $num_days_ = shift || 150;
if ( $update_existing_ != 0 && $update_existing_ != 1 ) {
  print "Error: Update_existing_results has to be 0 or 1.. Exiting..\n";
  exit(1);
}

my @pools_ = ( );
if ( $timeperiod_ ne "-1" ) {
  push ( @pools_, $timeperiod_ );
} else {
  my $pool_fetch_cmd_ = "$HOME_DIR/basetrade/walkforward/get_pools_for_shortcode.py -shc $shortcode_";
  @pools_ = `$pool_fetch_cmd_ 2>/dev/null`; chomp ( $pool_fetch_cmd_ );
}

foreach my $timeperiod_ ( @pools_ ) {
  my $fetch_configs_cmd_ = "$HOME_DIR/basetrade/walkforward/get_pool_configs.py -m POOL -shc $shortcode_ -tp $timeperiod_ -type N";
  my @all_strats_ = `$fetch_configs_cmd_ 2>/dev/null`; chomp ( @all_strats_ );
  
  $fetch_configs_cmd_ = "$HOME_DIR/basetrade/walkforward/get_pool_configs.py -m POOL -shc $shortcode_ -tp $timeperiod_ -type S";
  my @all_staged_strats_ = `$fetch_configs_cmd_ 2>/dev/null`; chomp ( @all_staged_strats_ );
  push ( @all_strats_, @all_staged_strats_ );
  
  my $LIST_OF_TRADING_DATES_SCRIPT = $SCRIPTS_DIR."/get_list_of_dates_for_shortcode.pl";
  my $exec_cmd_ = "$LIST_OF_TRADING_DATES_SCRIPT $shortcode_ $yyyymmdd_ $num_days_";
  my $exec_output_ = `$exec_cmd_`; chomp ( $exec_output_ );
  my @dates_vec_ = split ( ' ', $exec_output_ ); chomp ( @dates_vec_ );
  
  my %sample_pnls_strats_vec_;
  FetchPnlSamplesStrats ( $shortcode_, \@all_strats_, \@dates_vec_, \%sample_pnls_strats_vec_ );

  my %similarity_matrix_ = ( );
  foreach my $sti1_ ( 0 .. ($#all_strats_-1) ) {
    my %db_strat_pairs_ = ();
    if($update_existing_ == 0) {
       FetchAllCorrelationForStratsUpdatedRecently($all_strats_[$sti1_], \%db_strat_pairs_);
    }
    foreach my $sti2_ ( ($sti1_+1) .. $#all_strats_ ) {
      my ($strat1_, $strat2_) = ( $all_strats_[$sti1_], $all_strats_[$sti2_] );

      if ( ! exists $similarity_matrix_ { $strat1_ } { $strat2_ } && ! exists $db_strat_pairs_{$strat2_} ) {
        $similarity_matrix_ { $strat1_ } { $strat2_ } = GetPnlSamplesCorrelation ( $strat1_, $strat2_, \%sample_pnls_strats_vec_ );
        InsertCorrelation($shortcode_, $strat1_, $strat2_, $similarity_matrix_ {$strat1_} {$strat2_}, 1);       
      }
    }
  }

}

RemovePrunedStratsCorrelation();

sub RemovePrunedStratsCorrelation {
  my @pruned_strats = ();
  PrunedStrats( $shortcode_, \@pruned_strats );
  RemoveStratCorrelations(\@pruned_strats);
}
