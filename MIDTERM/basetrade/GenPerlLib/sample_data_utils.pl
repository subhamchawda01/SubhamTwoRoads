#!/usr/bin/perl
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

my $DATA_DIR="/NAS1/SampleData/";

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; 
require "$GENPERLLIB_DIR/sample_data_fetch.pl"; # GetFeatureMap, getSampleSlotFromHHMM

sub GetFeatureSum
{
  my ($this_shc_, $this_date_, $factor_, $factor_aux_, $s_slot_, $e_slot_, $verbose_, $sampling_dur_) = @_;

  if ( ! defined $factor_aux_ ) { $factor_aux_ = []; }
  if ( ! defined $verbose_ ) { $verbose_ = 0; }
  if ( ! defined $sampling_dur_ ) { $sampling_dur_ = 15; }

  if ( defined $s_slot_ ) { $s_slot_ = `$BIN_DIR/get_utc_hhmm_str $s_slot_ $this_date_`; chomp($s_slot_); }
  if ( defined $e_slot_ ) { $e_slot_ = `$BIN_DIR/get_utc_hhmm_str $e_slot_ $this_date_`; chomp($e_slot_); }

  my %factor_samples_vec_ = ( );
  GetFeatureMap ( $this_shc_, $this_date_, $factor_, $s_slot_, $e_slot_, \%factor_samples_vec_, $factor_aux_, $verbose_ );

  my @factor_samples_list_ = values %factor_samples_vec_;

  my $is_valid_ = 0;
  if ( @factor_samples_list_ > 0 ) { $is_valid_ = 1; }

  return ( GetSum(\@factor_samples_list_), $is_valid_ );
}

sub GetFeatureAverage
{
  my ($this_shc_, $this_date_, $factor_, $factor_aux_, $s_slot_, $e_slot_, $verbose_, $sampling_dur_) = @_;

  if ( ! defined $factor_aux_ ) { $factor_aux_ = []; }
  if ( ! defined $verbose_ ) { $verbose_ = 0; }
  if ( ! defined $sampling_dur_ ) { $sampling_dur_ = 15; }

  if ( defined $s_slot_ ) { $s_slot_ = `$BIN_DIR/get_utc_hhmm_str $s_slot_ $this_date_`; chomp($s_slot_); }
  if ( defined $e_slot_ ) { $e_slot_ = `$BIN_DIR/get_utc_hhmm_str $e_slot_ $this_date_`; chomp($e_slot_); }

  my $factor_orig_ = $factor_;
  if ( $factor_ eq "MAXMINDIFF" || $factor_ eq "CLOSEOPENDIFF" ) {
    $factor_ = "AvgPrice";
  }

  my %factor_samples_vec_ = ( );
  GetFeatureMap ( $this_shc_, $this_date_, $factor_, $s_slot_, $e_slot_, \%factor_samples_vec_, $factor_aux_, $verbose_ );

  my @factor_samples_list_ = values %factor_samples_vec_;

  my $is_valid_ = 0;
  if ( @factor_samples_list_ > 0 ) { $is_valid_ = 1; }

  my $factor_average_ = GetAverage(\@factor_samples_list_);

  if ( $factor_orig_ eq "MAXMINDIFF" && $is_valid_ ) {
    $factor_average_ = max(@factor_samples_list_) - min(@factor_samples_list_);
  }
  elsif ( $factor_orig_ eq "CLOSEOPENDIFF" && $is_valid_ ) {
    @factor_samples_list_ = @factor_samples_vec_{ sort { $a <=> $b } keys %factor_samples_vec_ };
    $factor_average_ = $factor_samples_list_[-1] - $factor_samples_list_[0];
  }

  return ( $factor_average_, $is_valid_ );
}

sub GetFeatureAverageDays
{
  my ($this_shc_, $this_date_, $lookback_, $factor_, $factor_aux_, $s_slot_, $e_slot_, $verbose_, $sampling_dur_) = @_;

  if ( ! defined $factor_aux_ ) { $factor_aux_ = []; }
  if ( ! defined $verbose_ ) { $verbose_ = 0; }
  if ( ! defined $sampling_dur_ ) { $sampling_dur_ = 15; }

  my @dates_vec_ = GetDatesFromNumDays ( $this_shc_, $this_date_, $lookback_ );

  my @feature_avg_vec_ = ();
  foreach my $tdate_ ( @dates_vec_ ) {
    my ($t_avg_, $is_valid_) = GetFeatureAverage ( $this_shc_, $tdate_, $factor_, $factor_aux_, $s_slot_, $e_slot_, $verbose_, $sampling_dur_ );
    if ( $is_valid_ ) {
      push ( @feature_avg_vec_, $t_avg_ );
    }
  }
  return (GetAverage ( \@feature_avg_vec_ ), scalar @feature_avg_vec_);
}

sub GetFilteredDays
{
  my ($this_shc_, $dates_vec_ref_, $frac_, $high_low_, $factor_, $factor_aux_, $filtered_dates_ref_, $start_hhmm_, $end_hhmm_) = @_;

  my %feature_avg_map_ = ();
  foreach my $tdate_ ( @$dates_vec_ref_ ) {
    my ($t_avg_, $is_valid_) = GetFeatureAverage ( $this_shc_, $tdate_, $factor_, $factor_aux_, $start_hhmm_, $end_hhmm_ );
    if ( $is_valid_ ) {
      $feature_avg_map_{ $tdate_ } = $t_avg_;
    }
  }

  my @dates_sorted_ = sort { $feature_avg_map_{ $a } <=> $feature_avg_map_{ $b } } keys %feature_avg_map_;

  if ( $high_low_ eq "LOW" ) {
    my $filt_idx_ = max ( 0, int ( $frac_*( $#dates_sorted_ + 1 ) - 1 ) );
    @$filtered_dates_ref_ = @dates_sorted_ [ 0 .. $filt_idx_ ];
  }
  elsif ( $high_low_ eq "HIGH" ) {
    my $filt_idx_ = $#dates_sorted_ - max ( 0, min ( $frac_*( $#dates_sorted_ + 1 ) - 1 ) ) ;
    @$filtered_dates_ref_ = @dates_sorted_ [ $filt_idx_ .. $#dates_sorted_ ];
  }
  else {
    print "No HIGH or LOW selected";
    @$filtered_dates_ref_ = ( );
  }
}

sub GetFilteredDaysInPercRange
{
  my ($this_shc_, $dates_vec_ref_, $lfrac_, $hfrac_, $factor_, $factor_aux_, $filtered_dates_ref_, $start_hhmm_, $end_hhmm_) = @_;

  my %feature_avg_map_ = ();
  foreach my $tdate_ ( @$dates_vec_ref_ ) {
    my ($t_avg_, $is_valid_) = GetFeatureAverage ( $this_shc_, $tdate_, $factor_, $factor_aux_, $start_hhmm_, $end_hhmm_ );
    if ( $is_valid_ ) {
      $feature_avg_map_{ $tdate_ } = $t_avg_;
    }
  }

  my @dates_sorted_ = sort { $feature_avg_map_{ $a } <=> $feature_avg_map_{ $b } } keys %feature_avg_map_;

  my $l_idx_ = max ( 0, int ( $lfrac_*( $#dates_sorted_ + 1 ) - 1 ) );
  my $h_idx_ = max ( 0, int ( $hfrac_*( $#dates_sorted_ + 1 ) - 1 ) );
  @$filtered_dates_ref_ = @dates_sorted_ [ $l_idx_ .. $h_idx_ ];
}

sub GetFilteredDaysOnSampleBounds
{
  my ($this_shc_, $dates_vec_ref_, $lbound_, $ubound_, $factor_, $factor_aux_, $filtered_dates_ref_, $start_hhmm_, $end_hhmm_) = @_;

  my %feature_avg_map_ = ();
  foreach my $tdate_ ( @$dates_vec_ref_ ) {
    my ($t_avg_, $is_valid_) = GetFeatureAverage ( $this_shc_, $tdate_, $factor_, $factor_aux_, $start_hhmm_, $end_hhmm_ );
    if ( $is_valid_ ) {
	if ( $t_avg_ > $lbound_ && $t_avg_ < $ubound_ ) {
	    push ( @$filtered_dates_ref_, $tdate_ );
	}
    }
  }
}

sub GetFilteredSamples
{
  my ($this_shc_, $dates_vec_ref_, $frac_, $high_low_, $factor_, $factor_aux_, $filtered_samples_ref_, $start_hhmm_, $end_hhmm_) = @_;

  my %feature_map_ = ();
  foreach my $tdate_ ( @$dates_vec_ref_ ) {
    my ($s_slot_, $e_slot_);

    if ( defined $start_hhmm_ ) { $s_slot_ = `$BIN_DIR/get_utc_hhmm_str $start_hhmm_ $tdate_`; chomp($s_slot_); }
    if ( defined $end_hhmm_ ) { $e_slot_ = `$BIN_DIR/get_utc_hhmm_str $end_hhmm_ $tdate_`; chomp($e_slot_); }

    my %factor_samples_vec_ = ( );
    GetFeatureMap ( $this_shc_, $tdate_, $factor_, $s_slot_, $e_slot_, \%factor_samples_vec_, $factor_aux_ );

    foreach my $t_slot_ ( keys %factor_samples_vec_ ) {
      my $slot_key_ = GetDaySlotKey ( $tdate_, $t_slot_ );
      $feature_map_{ $slot_key_ } = $factor_samples_vec_{ $t_slot_ };
    }
  }

  my @samples_sorted_ = sort { $feature_map_{ $a } <=> $feature_map_{ $b } } keys %feature_map_;

  if ( $high_low_ eq "LOW" ) {
    my $filt_idx_ = max ( 0, int ( $frac_*( $#samples_sorted_ + 1 ) - 1 ) );
    @$filtered_samples_ref_ = @samples_sorted_ [ 0 .. $filt_idx_ ];
  }
  elsif ( $high_low_ eq "HIGH" ) {
    my $filt_idx_ = $#samples_sorted_ - max ( 0, min ( $frac_*( $#samples_sorted_ + 1 ) - 1 ) ) ;
    @$filtered_samples_ref_ = @samples_sorted_ [ $filt_idx_ .. $#samples_sorted_ ];
  }
  else {
    print "No HIGH or LOW selected";
    @$filtered_samples_ref_ = ( );
  }
}

sub PrintUTCFromSampleSlot
{
  my $slot_ = shift;
  my $sample_mins_ = shift || 15;

  my $curr_mins_ = $slot_ * $sample_mins_;
  my $prev_ = 0;
  if ( $curr_mins_ < 0 ) { 
    $prev_ = 1; 
    $curr_mins_ = 1440 + $curr_mins_; 
  }

  my $hhmm_ = sprintf("%04d", ( ( int( $curr_mins_ / 60 ) * 100 ) + ( $curr_mins_ % 60 ) ) );
  if ( $prev_ ) {
    return "PREV_UTC_".$hhmm_;
  } else {
    return "UTC_".$hhmm_;
  }
}

1
