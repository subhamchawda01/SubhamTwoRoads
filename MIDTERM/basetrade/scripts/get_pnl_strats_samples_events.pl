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

my $GLOBALPNLSAMPLESDIR=$HOME_DIR."/ec2_pnl_samples";

my $hostname_s_ = `hostname -s`; chomp ( $hostname_s_ );
if ( ! ( $USER eq "dvctrader" && ( ( $hostname_s_ eq "sdv-ny4-srv11" ) || ( $hostname_s_ eq "sdv-crt-srv11" ) ) ) )
{
  $GLOBALPNLSAMPLESDIR="/NAS1/ec2_pnl_samples";
}

require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult 
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec 
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie 
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList 
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/make_strat_vec_from_list_matchbase.pl"; #MakeStratVecFromListMatchBase
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats, PnlSamplesGetStats

my $USAGE = "$0 SHORTCODE STRAT_DIR/STRAT_LIST DATE NUM_DAYS ECO_EVENT_NAME START_TIME END_TIME [ DETAILED[0/1] ]";
if ( $#ARGV < 5 ) { print "$USAGE\n"; exit ( 0 ); }

my @factors_vec_ = qw ( VOL STDEV L1SZ CORR );

my $shortcode_ = $ARGV[0];
my $strat_dir_ = $ARGV[1];
my $yyyymmdd_ = $ARGV[2];
my $num_days_ = $ARGV[3];
my $event_name_ = $ARGV[4];
my $start_hhmm_ = $ARGV[5];
my $end_hhmm_ = $ARGV[6];
my $detailed_ = 0;
if ( $#ARGV > 6 ) {
  $detailed_ = $ARGV[7];
}

my $exec_cmd_ = "grep -h \"$event_name_\" ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_*_processed.txt | tac";
print $exec_cmd_."\n";

my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

my @dates_vec_all_ = ( );
foreach my $eco_line_ ( @exec_cmd_output_ )
{
  my @eco_line_words_ = split ( ' ' , $eco_line_ ); chomp ( @eco_line_words_ );

  if ( $#eco_line_words_ >= 4 )
  {
    my $date_time_word_ = $eco_line_words_ [ $#eco_line_words_ ];

    my @date_time_words_ = split ( '_' , $date_time_word_ ); chomp ( @date_time_words_ );
    if ( $#date_time_words_ >= 0 )
    {
      my $tradingdate_ = $date_time_words_ [ 0 ];
      push ( @dates_vec_all_, $tradingdate_ );
    }
  }
}
@dates_vec_all_ = grep { $_ <= $yyyymmdd_ } @dates_vec_all_;
my $ndates_ = min( $num_days_, scalar @dates_vec_all_ );
@dates_vec_all_ = reverse sort @dates_vec_all_;

my @dates_vec_ = @dates_vec_all_[ 0..($ndates_ - 1) ];

print "Dates: ".join(' ' , @dates_vec_ )."\n";

my @strats_paths_ = ();
my @strats_base_ = ();
if ( -d $strat_dir_ )
{
  @strats_paths_ = MakeStratVecFromDir( $strat_dir_ );
}
elsif ( -f $strat_dir_ )
{
  @strats_paths_ = MakeStratVecFromListMatchBase ( $strat_dir_, "ALL" );
}
else {
  print $strat_dir_.": No such directory or file exists\n";
}

for my $file_path_ ( @strats_paths_ ) {
  push ( @strats_base_, basename ( $file_path_ ) );
}

my %sample_pnls_strats_vec_;
FetchPnlSamplesStrats ( $shortcode_, \@strats_base_, \@dates_vec_, \%sample_pnls_strats_vec_, $start_hhmm_, $end_hhmm_);

my %strat2avgpnl_ = ();
my %strat2sharpe_ = ();
my %strat2avg_highqdd_ = ();

foreach my $t_strat_ ( @strats_base_ ) {
  if ( ! exists $sample_pnls_strats_vec_{$t_strat_} ) { next; }

  my %t_stats_map_;
  PnlSamplesGetStats ( \%{$sample_pnls_strats_vec_{$t_strat_}}, \%t_stats_map_, \@dates_vec_ );
  $strat2avgpnl_{ $t_strat_ } = $t_stats_map_ { "PNL" };
  $strat2sharpe_{ $t_strat_ } = $t_stats_map_ { "DAILY_SHARPE" };
  $strat2avg_highqdd_{ $t_strat_ } = $t_stats_map_ { "DD_HIGHAVG" };
}

my @strats_sorted_ = sort { $strat2sharpe_{$b} <=> $strat2sharpe_{$a} } keys %strat2sharpe_;
print "DailyAveragePnl Sharpe Avg_HighQuartile_DrawDown Strat ..\n";
for my $t_strat_ ( @strats_sorted_ )
{
  if ( ! $detailed_ ) {
    print int($strat2avgpnl_ { $t_strat_ })." ".sprintf("%.2f", $strat2sharpe_ { $t_strat_ })." ".int($strat2avg_highqdd_ { $t_strat_ })." ".$t_strat_."\n";
  }
  else {
    my %sample_pnl_series_ = ( );
    my %slot_pnls_ = ( );

    foreach my $t_sample_ ( keys %{$sample_pnls_strats_vec_{$t_strat_}} ) {
      my ( $t_date_, $t_slot_ ) = split ( "_", $t_sample_ );
      $sample_pnl_series_{ $t_date_ }{ $t_slot_ } = $sample_pnls_strats_vec_{$t_strat_}{$t_sample_};
      push ( @{ $slot_pnls_{ $t_slot_ } }, $sample_pnls_strats_vec_{$t_strat_}{$t_sample_} );
    }

    print "STRAT ".$t_strat_."\n";
    foreach my $t_date_ ( sort keys %sample_pnl_series_ ) {
      my $date_pnl_str_ = $t_date_." ";
      foreach my $t_slot_ ( sort keys %{ $sample_pnl_series_{ $t_date_ } } ) {
        $date_pnl_str_ .= $t_slot_.":".$sample_pnl_series_{ $t_date_ }{ $t_slot_ }." ";
      } 
      print $date_pnl_str_."\n"; 
    }
    my $date_pnl_str_ =  "STATISTICS ";
    foreach my $t_slot_ ( sort keys %slot_pnls_ ) {
      $date_pnl_str_ .= $t_slot_.":".GetAverage( \@{ $slot_pnls_{ $t_slot_ } } )." ";
    }
    print $date_pnl_str_."\n\n"; 
  }
}

