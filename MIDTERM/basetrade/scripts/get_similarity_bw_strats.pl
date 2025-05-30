#!/usr/bin/perl

# \file ModelScripts/summarize_strats_for_specific_days.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

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

my $MODELING_BASE_DIR = $HOME_DIR."/modelling";
if ( $USER eq "hagarwal" ) {
  my $MODELING_BASE_DIR = "/home/dvctrader/modelling";
}
my $MODELING_STRATS_DIR = $MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";

my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/stratstory_db_access_manager.pl"; # FetchCorrelationForPair
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats
require "$GENPERLLIB_DIR/sample_pnl_corr_utils.pl"; # GetPnlSamplesCorrelation

my $USAGE = "$0 SHORTCODE STRAT1 STRAT2 [0/1: OFFLINE, default:1] [DATE=TODAY-1] [NUM_DAYS=60] [START_TIME] [END_TIME]";
if ( $#ARGV < 2 ) { print "$USAGE\n"; exit ( 0 ); }

my $shortcode_ = shift;
my $strat1_ = shift;
my $strat2_ = shift;
my $use_offline_ = 1;
if ( $#ARGV >= 0 ) { $use_offline_ = shift; }

my $yyyymmdd_ = "TODAY-1";
if ( $#ARGV >= 0 ) { $yyyymmdd_ = shift; }
$yyyymmdd_ = GetIsoDateFromStrMin1($yyyymmdd_);

my $num_days_ = shift || 60;

my $start_hhmm_ = shift;
my $end_hhmm_ = shift;

my $LIST_OF_TRADING_DATES_SCRIPT = $SCRIPTS_DIR."/get_list_of_dates_for_shortcode.pl";
my $exec_cmd_ = "$LIST_OF_TRADING_DATES_SCRIPT $shortcode_ $yyyymmdd_ $num_days_";
#print $exec_cmd_."\n";
my $exec_output_ = `$exec_cmd_`; chomp ( $exec_output_ );
my @dates_vec_ = split ( ' ', $exec_output_ ); chomp ( @dates_vec_ );
#print "Dates: ".join(' ' , @dates_vec_ )."\n";

my @strats_base_ = ( $strat1_, $strat2_ );
my %sample_pnls_strats_vec_;

my $corr_ = -1;
if ( $use_offline_ == 1 ) {
  $corr_ = FetchCorrelationForPair ( $strat1_ , $strat2_ );
  if ( ! defined $corr_ || $corr_ == -1 ) {
    print "Error in Offline Entry Retrieval.. Computing Correlation now..\n";
    FetchPnlSamplesStrats ( $shortcode_, \@strats_base_, \@dates_vec_, \%sample_pnls_strats_vec_, $start_hhmm_, $end_hhmm_ );
    $corr_ = GetPnlSamplesCorrelation ( $strat1_, $strat2_, \%sample_pnls_strats_vec_ );
  }
} else {
  FetchPnlSamplesStrats ( $shortcode_, \@strats_base_, \@dates_vec_, \%sample_pnls_strats_vec_, $start_hhmm_, $end_hhmm_ );
  $corr_ = GetPnlSamplesCorrelation ( $strat1_, $strat2_, \%sample_pnls_strats_vec_ );
}

print "Correlation: ".$corr_."\n";
