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


require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate 
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday 
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate 
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate 
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult 
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec 
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie 
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList 
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage
require "$GENPERLLIB_DIR/sample_data_utils.pl"; # GetFeatureAverage
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats

sub GetDailyFeatureAndPnl
{
  my $dates_vec_ref_ = shift;
  my $features_vec_ref_ = shift;
  my $strats_vec_ref_ = shift;
  my $feature_values_ref_ = shift;
  my $start_hhmm_ = shift;
  my $end_hhmm_ = shift;
  my $is_config_;
  if ( $#_ >= 0 ) {
    $is_config_ = shift;
  }
  
  if ( !(defined $start_hhmm_ && defined $end_hhmm_) && $#$strats_vec_ref_ >= 0 ) {
    my $strat_ = $$strats_vec_ref_[0][1];
    $is_config_ = IsValidConfig( $strat_ );

    if ( $is_config_ ) {
      ($start_hhmm_, $end_hhmm_) = GetConfigStartEndHHMM ( $strat_ );
    } else {
      $start_hhmm_ = `cat /home/dvctrader/modelling/\*strats/\*/\*/$strat_ | head -1 | cut -d' ' -f6`; chomp ( $start_hhmm_ );
      $end_hhmm_ = `cat /home/dvctrader/modelling/\*strats/\*/\*/$strat_ | head -1 | cut -d' ' -f7`; chomp ( $end_hhmm_ );
    }
  }

  foreach my $feature_ref_ ( @$features_vec_ref_ ) {
    my %daily_features_ = ( );
    GetDailyFeatureAvgs ( $dates_vec_ref_, $feature_ref_, \%daily_features_, $start_hhmm_, $end_hhmm_ );
    $$feature_values_ref_{ join(" ", @$feature_ref_ ) } = \%daily_features_;
  }
  foreach my $strat_ref_ ( @$strats_vec_ref_ ) {
    my ( $shc_, $strat_ ) = @$strat_ref_;
    my %daily_pnl_ = ( );
    FetchPnlDaysStrats ( $shc_, [$strat_], $dates_vec_ref_, \%daily_pnl_, $start_hhmm_, $end_hhmm_, $is_config_);
    $$feature_values_ref_{ join(" ", @$strat_ref_ ) } = \%{ $daily_pnl_{$strat_} };
  }
}

sub GetSampleFeatureAndPnl
{
  my $dates_vec_ref_ = shift;
  my $features_vec_ref_ = shift;
  my $strats_vec_ref_ = shift;
  my $feature_values_ref_ = shift;
  my $start_hhmm_ = shift;
  my $end_hhmm_ = shift;
  my $slot_count_map_ref_ = shift;
  my $is_config_;
  if ( $#_ >= 0 ) {
    $is_config_ = shift;
  }

  if ( !(defined $start_hhmm_ && defined $end_hhmm_) && $#$strats_vec_ref_ >= 0 ) {
    my $strat_ = $$strats_vec_ref_[0][1];
    $is_config_ = IsValidConfig( $strat_ );
    
    if ( $is_config_ ) {
      ($start_hhmm_, $end_hhmm_) = GetConfigStartEndHHMM ( $strat_ );
    } else {
      $start_hhmm_ = `cat /home/dvctrader/modelling/\*strats/\*/\*/$strat_ | head -1 | cut -d' ' -f6`; chomp ( $start_hhmm_ );
      $end_hhmm_ = `cat /home/dvctrader/modelling/\*strats/\*/\*/$strat_ | head -1 | cut -d' ' -f7`; chomp ( $end_hhmm_ );
    }
  }

  my %features_vals_map_ = ( );
  GetSlotFeatureAvgs ( $dates_vec_ref_, $features_vec_ref_, $feature_values_ref_, $start_hhmm_, $end_hhmm_, $slot_count_map_ref_);

  foreach my $strat_ref_ ( @$strats_vec_ref_ ) {
    my ( $shc_, $strat_ ) = @$strat_ref_;
    my %sample_pnls_ = ( );
    FetchPnlSamplesStrats ( $shc_, [$strat_], $dates_vec_ref_, \%sample_pnls_, $start_hhmm_, $end_hhmm_, $is_config_ );
    my $stratkey_ = join(" ", @$strat_ref_ );
    foreach my $key_ ( keys %{$sample_pnls_{$strat_}} ) {
      my $key_converted_ = ConvertKeyToFormat1 ( $key_ );
      $$feature_values_ref_{ $stratkey_ }{ $key_converted_ } = $sample_pnls_{ $strat_ }{ $key_ };

      if ( ! exists $$slot_count_map_ref_{ $key_converted_ } ) {
        $$slot_count_map_ref_{ $key_converted_ } = 0;
      }
      $$slot_count_map_ref_{ $key_converted_ } += 1;
    }
  }
}

sub GetSlotFeatureAvgs
{
  my ( $dates_vec_ref_, $features_vec_ref_, $features_map_ref_, $start_hhmm_, $end_hhmm_, $slot_count_map_ref_ )  = @_;

  foreach my $this_date_ ( @$dates_vec_ref_ ) {
    my ($s_slot_, $e_slot_);

    if ( defined $start_hhmm_ ) { $s_slot_ = `$BIN_DIR/get_utc_hhmm_str $start_hhmm_ $this_date_`; chomp($s_slot_); }
    if ( defined $end_hhmm_ ) { $e_slot_ = `$BIN_DIR/get_utc_hhmm_str $end_hhmm_ $this_date_`; chomp($e_slot_); }

    foreach my $feature_ref_ ( @$features_vec_ref_ ) {
      my %t_timeslots_map_ = ();
      GetFeatureMap ( $feature_ref_->[0], $this_date_, $feature_ref_->[1], $s_slot_, $e_slot_, \%t_timeslots_map_, [ @$feature_ref_[2..$#$feature_ref_] ]);
      foreach my $t_slot_ ( keys %t_timeslots_map_ ) {
        my $slot_ = GetDaySlotKey ( $this_date_, $t_slot_ );
        $$features_map_ref_{ join(" ", @$feature_ref_) }{ $slot_ } = $t_timeslots_map_{ $t_slot_ };
        if ( ! exists $$slot_count_map_ref_{ $slot_ } ) {
          $$slot_count_map_ref_{ $slot_ } = 0;
        }
        $$slot_count_map_ref_{ $slot_ } += 1;
      }
    }
  }
}

sub GetDailyFeatureAvgs
{
  my ( $dates_vec_ref_, $feature_ref_, $daily_features_ref_, $start_hhmm_, $end_hhmm_ )  = @_;

  foreach my $this_date_ ( @$dates_vec_ref_ ) {
    my ( $t_feature_avg_, $is_valid_ ) = GetFeatureAverage ( $feature_ref_->[0], $this_date_, $feature_ref_->[1], [ @$feature_ref_[2..$#$feature_ref_] ], $start_hhmm_, $end_hhmm_ );
    if ( $is_valid_ ) {
      $$daily_features_ref_{ $this_date_ } = $t_feature_avg_;
    }
  }
}

1
