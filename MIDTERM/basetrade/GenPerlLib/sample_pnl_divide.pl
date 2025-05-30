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
use List::Util qw/max min first/; # for max
use Math::Complex; # sqrt
use FileHandle;
use POSIX;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $MODELING_BASE_DIR = $HOME_DIR."/modelling";
my $MODELING_STRATS_DIR = $MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";

my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate 
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday 
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday 
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate 
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate 
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult 
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize 
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec 
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie 
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList 
require "$GENPERLLIB_DIR/array_ops.pl"; # GetStdev
require "$GENPERLLIB_DIR/sample_data_utils.pl"; 
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats

my $USAGE = "$0 SHORTCODE DATE NUM_DAYS START_TIME END_TIME NUM_BUCKETS STRAT_NAME (VOL / STDEV / L1SZ / TREND / <CORR indep_shortcode> / <other_features> )";
if ( $#ARGV < 7 ) { print "$USAGE\n"; exit ( 0 ); }

my @factors_vec_ = qw ( VOL STDEV L1SZ CORR );

my $shortcode_ = shift;
my $yyyymmdd_ = shift;
my $num_days_ = shift;
my $start_hhmm_ = shift;
my $end_hhmm_ = shift;
my $num_buckets_ = shift;
my $strat_name_ = shift;
my $factor_ = shift;
my $factor_aux_ref_ = \@ARGV;
my %sample_pnls_strats_vec_;
my %sample_feature_vec_;

my $max_feature_value_ = -9**9**9;
my $min_feature_value_ = 9**9**9;
my @feature_bucket_pnl_vec_ = ();
my @feature_bucket_count_vec_ = ();
my @feature_val_vec_ = ();

if($num_buckets_ < 3) {
  $num_buckets_ = 3;
}

for my $i (0 .. $num_buckets_)
{
  $feature_bucket_pnl_vec_[$i] = 0;
  $feature_bucket_count_vec_[$i] = 0;
}

my $LIST_OF_TRADING_DATES_SCRIPT = $SCRIPTS_DIR."/get_list_of_dates_for_shortcode.pl";
my $exec_cmd_ = "$LIST_OF_TRADING_DATES_SCRIPT $shortcode_ $yyyymmdd_ $num_days_";
#print $exec_cmd_."\n";
my $exec_output_ = `$exec_cmd_`; chomp ( $exec_output_ );
my @dates_vec_ = split ( ' ', $exec_output_ ); chomp ( @dates_vec_ );
#print "Dates: ".join(' ' , @dates_vec_ )."\n";

FetchPnlSamplesStrats ( $shortcode_, [$strat_name_] , \@dates_vec_, \%sample_pnls_strats_vec_, $start_hhmm_, $end_hhmm_ );

foreach my $date_ ( @dates_vec_ ) {
  my %t_timeslots_map_ = ();
  my $s_slot_ = `$BIN_DIR/get_utc_hhmm_str $start_hhmm_ $date_`;
  my $e_slot_ = `$BIN_DIR/get_utc_hhmm_str $end_hhmm_ $date_`;
  GetFeatureMap ( $shortcode_, $date_, $factor_, $s_slot_, $e_slot_, \%t_timeslots_map_, $factor_aux_ref_ );

  foreach my $t_slot_ ( keys %t_timeslots_map_ ) {
    my $slot_key_ = GetDaySlotKey ( $date_, $t_slot_ );
    $sample_feature_vec_{$slot_key_} = $t_timeslots_map_{$t_slot_} ;
   push(@feature_val_vec_,$t_timeslots_map_{$t_slot_});
  }
}

@feature_val_vec_ = sort { $a <=> $b } @feature_val_vec_;
my $twenty_percentile_point_ = $feature_val_vec_[int ($#feature_val_vec_ * 0.2)];
my $eighty_percentile_point_ = $feature_val_vec_[int ($#feature_val_vec_ * 0.8)];

#print "Twenty percentile point : ".$twenty_percentile_point_."\nEighty percentile point : ".$eighty_percentile_point_."\n";

my $step_increment_ = ($eighty_percentile_point_ - $twenty_percentile_point_) / ($num_buckets_ - 2) ; 
my $total_pnl_ = 0;
my @pnl_series_ = ( );
 
foreach my $key_ ( keys %{$sample_pnls_strats_vec_{$strat_name_}} ) {
  my $key_converted_ = ConvertKeyToFormat1 ( $key_ );
  if ( exists $sample_feature_vec_{$key_converted_} ) {
    my $val = $sample_feature_vec_{$key_converted_};
    my $bucket_num_;

    if($val < $twenty_percentile_point_) {
      $bucket_num_ = 0;
    }
    elsif($val >= $eighty_percentile_point_) {
      $bucket_num_ = $num_buckets_ - 1;
    }    
    else {
      $bucket_num_ = int(($val - $twenty_percentile_point_)/$step_increment_) + 1;
    }
    $feature_bucket_count_vec_[$bucket_num_]++;
    $total_pnl_ +=  $sample_pnls_strats_vec_{$strat_name_}{$key_};
    $feature_bucket_pnl_vec_[$bucket_num_] +=  $sample_pnls_strats_vec_{$strat_name_}{$key_};
    push ( @pnl_series_, $sample_pnls_strats_vec_{$strat_name_}{$key_} );
  }
}

my $pnl_sd_ = GetStdev ( \@pnl_series_ );
my $sum_start_ = 0;
my $sum_end_ = 0;
my $count_start_ = 0;
my $count_end_ = 0;

for my $bucket_num_ (0 .. $num_buckets_-1)
{
  #my $start_val_ = $twenty_percentile_point_ + $step_increment_*($bucket_num_ - 1);
  #my $end_value_ = $twenty_percentile_point_ + $step_increment_*($bucket_num_);
  #print $start_val_." ".$end_value_." : ".$feature_bucket_pnl_vec_[$bucket_num_]."\n";
  $sum_end_ += $feature_bucket_pnl_vec_[$bucket_num_];
  $count_end_ += $feature_bucket_count_vec_[$bucket_num_];
}

my $max_diff_ = 0;
my $max_diff_index_ = -1;
my $sum_less_ = 0;
my $sum_greater_equal_ = 0;
my $count_start_threshold_ = 0;
my $count_end_threshold_ = $count_end_;

for my $bucket_num_ (0 .. $num_buckets_-2)
{
  $sum_start_ += $feature_bucket_pnl_vec_[$bucket_num_];
  $sum_end_ -= $feature_bucket_pnl_vec_[$bucket_num_];
  $count_start_ += $feature_bucket_count_vec_[$bucket_num_];
  $count_end_ -= $feature_bucket_count_vec_[$bucket_num_];
  my $diff_ = abs ($sum_start_/$count_start_ - $sum_end_/$count_end_);
  if($diff_ > $max_diff_)
  {
    $max_diff_ = $diff_;
    $max_diff_index_ = $bucket_num_;
    $sum_less_ = $sum_start_/$count_start_;
    $sum_greater_equal_ = $sum_end_/$count_end_;
    $count_start_threshold_ = $count_start_;
    $count_end_threshold_ = $count_end_;
  }
}

my $threshold_value_ = $twenty_percentile_point_ + $step_increment_*($max_diff_index_);
my $total_mean_sum_ = $total_pnl_/($count_start_ + $count_end_);
my $percentile_start_set_ = ($count_start_threshold_/($count_start_threshold_ + $count_end_threshold_))*100;
my $percentile_end_set_ = ($count_end_threshold_/($count_start_threshold_ + $count_end_threshold_))*100;

print "Stdev of PnL: ".$pnl_sd_."\n";
print "Mean of PnL when ".$factor_." is less than ".$threshold_value_." : ".$sum_less_."\n";
print "Mean of PnL when ".$factor_." is greater than or equal to ".$threshold_value_." : ".$sum_greater_equal_."\n";
print "Mean PnL Difference for 15 min sample : ".$max_diff_."\nValue of feature at which difference occurs : ".$threshold_value_."\n";
print "Mean Overall PnL : ".$total_mean_sum_."\n";
printf "Data Split : %.2f%% - %.2f%%\n",$percentile_start_set_,$percentile_end_set_;
