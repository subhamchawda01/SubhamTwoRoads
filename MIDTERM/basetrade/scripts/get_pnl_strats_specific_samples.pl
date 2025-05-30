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

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie 
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList 
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/make_strat_vec_from_list_matchbase.pl"; #MakeStratVecFromListMatchBase
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage
require "$GENPERLLIB_DIR/results_db_access_manager.pl"; # FetchPnlSamples
require "$GENPERLLIB_DIR/sample_data_utils.pl"; # GetFilteredSamples
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats

my $USAGE = "$0 SHORTCODE STRAT_DIR/STRAT_LIST DATE NUM_DAYS START_TIME END_TIME ( VOL / STDEV / L1SZ / \"CORR <indep_shortcode>\" / AnyOtherFeature ) ( FRACTION ) HIGH / LOW";
if ( $#ARGV < 7 ) { print "$USAGE\n"; exit ( 0 ); }

my $shortcode_ = $ARGV[0];
my $strat_dir_ = $ARGV[1];
my $yyyymmdd_ = $ARGV[2];
my $num_days_ = $ARGV[3];
my $start_hhmm_ = $ARGV[4];
my $end_hhmm_ = $ARGV[5];

my $factor_string_ = $ARGV[6];
my @factor_vec_ = split(" ", $factor_string_);
my $factor_ = shift @factor_vec_;
my $factor_aux_ref_ = \@factor_vec_;

my $frac_ = $ARGV[7];

my $high_low_ = "HIGH";
if ( $#ARGV >= 8 ) { 
  $high_low_ = $ARGV[8];
}
if ( $frac_ < 0.0 || $frac_ > 1.0 ) {
  exit ( 0 );
}

my $LIST_OF_TRADING_DATES_SCRIPT = $SCRIPTS_DIR."/get_list_of_dates_for_shortcode.pl";
my $exec_cmd_ = "$LIST_OF_TRADING_DATES_SCRIPT $shortcode_ $yyyymmdd_ $num_days_";
print $exec_cmd_."\n";
my $exec_output_ = `$exec_cmd_`; chomp ( $exec_output_ );
my @dates_vec_ = split ( ' ', $exec_output_ ); chomp ( @dates_vec_ );
print "Dates: ".join(' ' , @dates_vec_ )."\n";

my @filtered_samples_ = ();
my @strats_base_ = ();

{
  my @strats_paths_ = ();
  if ( -d $strat_dir_ )
  {
    @strats_paths_ = MakeStratVecFromDir( $strat_dir_ );
  }
  elsif ( -f $strat_dir_ )
  {
    @strats_paths_ = MakeStratVecFromListMatchBase ( $strat_dir_, "ALL" );
  }
  else {
    print $strat_dir_.": No such directory or file exists\n"; exit(0);
  }
  @strats_base_ = map { basename( $_ ) } @strats_paths_;
}

GetFilteredSamples ( $shortcode_, \@dates_vec_, $frac_, $high_low_, $factor_, $factor_aux_ref_, \@filtered_samples_, $start_hhmm_, $end_hhmm_ );
my %filtered_samples_exists_ = map { $_ => 1 } @filtered_samples_;

my %sample_pnls_strats_vec_;
FetchPnlSamplesStrats ( $shortcode_, \@strats_base_, \@dates_vec_, \%sample_pnls_strats_vec_, $start_hhmm_, $end_hhmm_ );

PrintFilteredStratPnls ( );

sub PrintFilteredStratPnls
{
  my %strat_avg_pnl_;
  my %strat_sharpe_;
  for my $t_strat_ ( keys %sample_pnls_strats_vec_ )
  {
    my @filtered_slots_ = grep { exists $filtered_samples_exists_{ ConvertKeyToFormat1( $_ ) } } keys %{ $sample_pnls_strats_vec_{ $t_strat_ } };
    my @pnl_vals_ = @{ $sample_pnls_strats_vec_{ $t_strat_ } }{ @filtered_slots_ };
#    print join(' ', @pnl_vals_ )."\n";
    $strat_avg_pnl_ { $t_strat_ } = GetAverage ( \@pnl_vals_ ); 
    $strat_sharpe_ { $t_strat_ } = GetAverage ( \@pnl_vals_ ) / GetStdev ( \@pnl_vals_ ) ;
  }
  my @strats_sorted_ = sort { $strat_avg_pnl_{$b} <=> $strat_avg_pnl_{$a} } keys %strat_avg_pnl_;
  print "AveragePnl Sharpe Strat \n";
  for my $t_strat_ ( @strats_sorted_ )
  {
    print int($strat_avg_pnl_ { $t_strat_ })." ".sprintf("%.2f", $strat_sharpe_ { $t_strat_ })." ".$t_strat_."\n";
  }
}
