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

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage
require "$GENPERLLIB_DIR/sample_data_utils.pl"; # GetFeatureAverage, GetFeatureMap

my $USAGE = "$0 SHORTCODE DATE NUM_DAYS START_TIME END_TIME (0:AvgOverall / 1:AvgPerDay / 2:AvgPerTimeslot / 3 <percentile in 0-1> / 4:Event Traded Ezone)  (VOL / STDEV / L1SZ / TREND / <CORR indep_shortcode> / <other_features> )";
if ( $#ARGV < 5 ) { print "$USAGE\n"; exit ( 0 ); }

my @factors_vec_ = qw ( VOL STDEV L1SZ CORR );

my $shortcode_ = shift;
my $yyyymmdd_ = shift;
my $num_days_ = shift;
my $start_hhmm_ = shift;
my $end_hhmm_ = shift;
my $output_per_timeslot_ = shift;
#my $trade_ezone_ = shift;
my $percentile_;
my $traded_ezone_;
undef $traded_ezone_;

if ( ! looks_like_number($output_per_timeslot_) ){
    $traded_ezone_ = $output_per_timeslot_;
    print $traded_ezone_."\n";
    $output_per_timeslot_ = 0 ;
}

if ( $output_per_timeslot_ == 3 ) {
  $percentile_ = shift;
  if ( ! defined $percentile_ || ! looks_like_number( $percentile_ ) || $percentile_ < 0 || $percentile_ > 1 ) {
    print "$USAGE\n"; exit ( 0 );
  }
}


#if ( $#ARGV < 0 ) { print "3\n";print "$USAGE\n"; exit ( 0 ); }

my $factor_ = shift;
my $factor_aux_ref_ = \@ARGV;

my $LIST_OF_TRADING_DATES_SCRIPT = $SCRIPTS_DIR."/get_list_of_dates_for_shortcode.pl";

my @dates_vec_ = ( );

if ( $num_days_ eq "-1" && -f $yyyymmdd_ ) {
 @dates_vec_ = `cat $yyyymmdd_`; chomp ( @dates_vec_ );
 $yyyymmdd_ = max ( @dates_vec_ );
}
elsif(defined $traded_ezone_){
   my $end_date_ = $yyyymmdd_;
   my $start_date_cmd_=  $BIN_DIR."/calc_prev_week_day $yyyymmdd_ $num_days_";
   my $start_date_ = `$start_date_cmd_` ;
   chomp($start_date_);
   my $ebt_dates_cmd_ = $SCRIPTS_DIR."/get_dates_for_traded_ezone.pl $traded_ezone_ $start_date_ $end_date_";
   my $exec_output_ = `$ebt_dates_cmd_`; chomp ( $exec_output_ );
   @dates_vec_ = split ( ' ', $exec_output_ ); chomp ( @dates_vec_ );
}
else {
  my $exec_cmd_ = "$LIST_OF_TRADING_DATES_SCRIPT $shortcode_ $yyyymmdd_ $num_days_";

#print $exec_cmd_."\n";
  my $exec_output_ = `$exec_cmd_`; chomp ( $exec_output_ );
  @dates_vec_ = split ( ' ', $exec_output_ ); chomp ( @dates_vec_ );
}

my %factor_samples_vec_;

if ( $output_per_timeslot_ == 0 ) {
  my @feature_avg_vec_ = ();
  foreach my $date_ ( @dates_vec_ ) {
      
      #print $date_, $factor_,$start_hhmm_, $end_hhmm_,"\n"    ;
    my ($t_avg_, $is_valid_) = GetFeatureAverage ( $shortcode_, $date_, $factor_, $factor_aux_ref_, $start_hhmm_, $end_hhmm_ );
    if ( $is_valid_ ) {
      push ( @feature_avg_vec_, $t_avg_ );
    }
  }
  my $avg_overall_ = GetAverage ( \@feature_avg_vec_ );
  print "Avg (over_days_over_timeslots_): ".sprintf("%.8f", $avg_overall_)."\n";
}
elsif ( $output_per_timeslot_ == 1 ) {
  print "Avg (over_timeslots):\n";
  foreach my $date_ ( @dates_vec_ ) {
    my ($t_avg_, $is_valid_) = GetFeatureAverage ( $shortcode_, $date_, $factor_, $factor_aux_ref_, $start_hhmm_, $end_hhmm_ );
    if ( $is_valid_ ) {
      print $date_." ".sprintf("%.8f",$t_avg_)."\n";
    }
  }
}
elsif ( $output_per_timeslot_ == 2 ) {
  my %feature_timeslots_map_ = ();
  foreach my $date_ ( @dates_vec_ ) {
    my %t_timeslots_map_ = ();
    my $s_slot_ = `$BIN_DIR/get_utc_hhmm_str $start_hhmm_ $date_`;
    my $e_slot_ = `$BIN_DIR/get_utc_hhmm_str $end_hhmm_ $date_`;
    GetFeatureMap ( $shortcode_, $date_, $factor_, $s_slot_, $e_slot_, \%t_timeslots_map_, $factor_aux_ref_ );

    foreach my $t_slot_ ( keys %t_timeslots_map_ ) {
      push ( @{$feature_timeslots_map_{ $t_slot_ }}, $t_timeslots_map_{$t_slot_} );
    }
  }

  print "Avg (over_days):\n";
  foreach my $t_slot_ ( sort { $a <=> $b } keys %feature_timeslots_map_ ) {
    my $t_avg_ = GetAverage ( \@{$feature_timeslots_map_{ $t_slot_ }} );
    print PrintUTCFromSampleSlot($t_slot_)." ".sprintf("%.8f",$t_avg_)."\n";
  }
}
elsif ( $output_per_timeslot_ == 3 ) {
  my @feature_vals_vec_ = ( );
  foreach my $date_ ( @dates_vec_ ) {
    my %t_timeslots_map_ = ();
    my $s_slot_ = `$BIN_DIR/get_utc_hhmm_str $start_hhmm_ $date_`;
    my $e_slot_ = `$BIN_DIR/get_utc_hhmm_str $end_hhmm_ $date_`;
    GetFeatureMap ( $shortcode_, $date_, $factor_, $s_slot_, $e_slot_, \%t_timeslots_map_, $factor_aux_ref_ );

    push ( @feature_vals_vec_, values %t_timeslots_map_ );
  }
  @feature_vals_vec_ = map { abs($_) } @feature_vals_vec_;

  @feature_vals_vec_ = sort { $a <=> $b } @feature_vals_vec_;
  my $t_index_ = max(0, ($#feature_vals_vec_ + 1) * $percentile_ - 1);
  print sprintf("%.8f", $feature_vals_vec_[ $t_index_ ])."\n";
}




