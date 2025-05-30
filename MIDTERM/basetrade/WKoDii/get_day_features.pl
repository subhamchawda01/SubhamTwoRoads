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
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/sample_data_utils.pl"; # GetFeatureAverage
require "$GENPERLLIB_DIR/get_sample_features_pnls.pl"; # GetDailyFeatureAndPnl
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/config_utils.pl"; #IsValidConfig
if ( $#ARGV < 0 )
{
  print "$0 <SHC> <END-DATE> [Lookback_days=1/START_DATE] [Features-Config-File/-1] [DAY(default)/SAMPLE] [start_hhmm end_hhmm] [printNaN = 0]\n";
  exit(0);
}

my $shc_ = $ARGV[0];
my $date_ = $ARGV[1];
my $lookback_OR_start_date_ = 1;
if ( $#ARGV > 1 ) { $lookback_OR_start_date_ = $ARGV[2]; }

my $features_config_file_ = "/spare/local/tradeinfo/day_features/product_configs/".$shc_."_config.txt";
if ( $#ARGV > 2 && $ARGV[3] ne "-1" ) { $features_config_file_ = $ARGV[3]; }

my $granularity_ = "DAY";
if ( $#ARGV > 3 ) { $granularity_ = $ARGV[4]; }

my $start_hhmm_;
my $end_hhmm_;
if ( $#ARGV > 4 ) { $start_hhmm_ = $ARGV[5]; }
if ( $#ARGV > 5 ) { $end_hhmm_ = $ARGV[6]; }

my $printnan_ = 0;
if ( $#ARGV > 6 ) { $printnan_ = $ARGV[7]; }

my @feature_config_vec_ = ( );
my @strats_vec_ = ( );
my @colnames_vec_ = ( );
my $is_config_ = 0;

open CONFIGHANDLE, "< $features_config_file_" or PrintStacktraceAndDie ( "Could not open the Features Vector File: ".$features_config_file_."\n" );
while ( my $thisline_ = <CONFIGHANDLE> )
{
  chomp ( $thisline_ );
  my @this_words_ = split ( ' ', $thisline_ );
  if ( $#this_words_ < 1 ) {
    next;
  }
  my $feature_string_ = "";

  my $dep_shc_ = shift @this_words_;
  my $factor_ = shift @this_words_;
  if ( $factor_ ne "STRAT" ) {
    my @feature_ = ( $dep_shc_, $factor_, @this_words_ );
    push ( @feature_config_vec_, \@feature_ );
    $feature_string_ = join(" ", @feature_ );
  }
  else {
    my $strat_ = shift @this_words_;
    my @shc_strat_ = ( $dep_shc_, $strat_ );
    if (IsValidConfig($strat_) > 0 ){
    	$is_config_ = 1;
    }
    push ( @strats_vec_, \@shc_strat_ );
    $feature_string_ = join(" ", @shc_strat_);
  }
  push ( @colnames_vec_, $feature_string_ );
}

my %feature_values_ = ( );
my @dates_vec_ = ( );
GetListofDates ( $shc_, $date_, $lookback_OR_start_date_, \@dates_vec_ );

my @slots_vec_ = ( );
if ( $granularity_ eq "DAY" ) {
  GetDailyFeatureAndPnl ( \@dates_vec_, \@feature_config_vec_, \@strats_vec_, \%feature_values_, $start_hhmm_, $end_hhmm_, $is_config_);
  @slots_vec_ = sort @dates_vec_;
}
elsif ( $granularity_ eq "SAMPLE" )  {
  my %slots_count_map_ = ( );
  GetSampleFeatureAndPnl ( \@dates_vec_, \@feature_config_vec_, \@strats_vec_, \%feature_values_, $start_hhmm_, $end_hhmm_, \%slots_count_map_, $is_config_);

  my @t_slots_vec_ = ( );
  if ( $printnan_ ) { @t_slots_vec_ = keys %slots_count_map_; }
  else { @t_slots_vec_ = grep { $slots_count_map_{ $_ } == (scalar @feature_config_vec_ + scalar @strats_vec_) } keys %slots_count_map_; }

  my @slots_splits_vec_ = map { [split("_",$_)] } @t_slots_vec_;
  my @dates_ = map { $$_[0] } @slots_splits_vec_;
  my @slots_ = map { $$_[1] } @slots_splits_vec_;
  my @dates_uniq_ = GetUniqueList(@dates_);
  foreach my $tdate_ ( sort @dates_uniq_ ) {
    my @tslots_ = @slots_[ grep { $dates_[$_] eq $tdate_ } 0..$#dates_ ];
    foreach my $tslot_ ( sort { $a <=> $b } @tslots_ ) {
      push ( @slots_vec_, $tdate_."_".$tslot_ );
    }
  }
}
else {
  print "Error: granularity has to DAY or SAMPLE\n";
  exit ( 0 );
}

foreach my $t_slot_ ( @slots_vec_ ) {
  my $is_valid_ = 1;
  my $t_feature_str_ = "";
  foreach my $feature_ ( @colnames_vec_ ) {
    if ( defined $feature_values_{ $feature_ }{ $t_slot_ } ) {
      $t_feature_str_ = $t_feature_str_.sprintf("%.4f ", $feature_values_{ $feature_ }{ $t_slot_ });
    } 
    else {
      $is_valid_ = 0;
      $t_feature_str_ = $t_feature_str_."NaN ";
    }
  }
  if ( $is_valid_ || $printnan_ ) {
    print $t_slot_." ".$t_feature_str_."\n";
  }
}

sub GetListofDates 
{
  my $shc_ = shift;
  my $date_ = shift;
  my $lookback_OR_start_date_ = shift;
  my $list_of_dates_ref_ = shift;

  my ($lookback_, $start_date_);
  if ( $#ARGV > 1 ) {
    if ( ValidDate( $lookback_OR_start_date_ ) ) {
      $start_date_ = $lookback_OR_start_date_;
      $lookback_ = 1000;
    } else {
      $lookback_ = $lookback_OR_start_date_;
      $start_date_ = -1;
    }
  }


  for ( my $days_ = $lookback_ ; $days_ != 0 && $date_ >= $start_date_ ; )
  {
    if ( ! ValidDate ( $date_ ) )
    {
      PrintStacktraceAndDie ( "Invalid date : $date_\n" );
      return;
    }

    if ( SkipWeirdDate ( $date_ ) || IsDateHoliday ( $date_ ) )
    {
      $date_ = CalcPrevWorkingDateMult ( $date_ , 1 );
      next;
    }
    push ( @$list_of_dates_ref_, $date_ );
    $date_ = CalcPrevWorkingDateMult ( $date_ , 1 );
    $days_--;
  }
}
