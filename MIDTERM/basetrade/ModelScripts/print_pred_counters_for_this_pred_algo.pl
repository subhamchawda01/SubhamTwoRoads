#!/usr/bin/perl

# \file ModelScripts/print_pred_counters_for_this_pred_algo.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
# For time based prediction it converts seconds to milliseconds.
# For events based prediction it finds the number of events per second and multiplies byt preddur in seconds

use feature "switch"; # for given, when
use File::Basename;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_avg_event_count_per_sec_for_shortcode.pl"; # GetAvgEventCountPerSecForShortcode
require "$GENPERLLIB_DIR/sample_data_utils.pl"; # GetFeatureAverage
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1 

if ( @ARGV<4 ) {
    print "USAGE : Script Shorcode Pred_Duration_in_secs Pred_Algo Timed_data_file Start_hhmm End_hhmm\n";
    exit ( 0 );
}

my $shortcode_ = $ARGV[0];
my $this_pred_duration_seconds_ = $ARGV[1];
my $this_predalgo_ = $ARGV[2];
my $this_day_timed_data_filename_ = $ARGV[3];
my $start_hhmm_ = $ARGV[4];
my $end_hhmm_ = $ARGV[5];

my $this_pred_counters_ = 1000 * $this_pred_duration_seconds_;

if ( ( $this_predalgo_ eq "na_t1" ) ||
    ( $this_predalgo_ eq "na_t3" ) ||
    ( $this_predalgo_ eq "na_t5" ) ||
    ( $this_predalgo_ eq "na_s4" ) )
{
  $this_pred_counters_ = 1000 * $this_pred_duration_seconds_;
}
if ( ( $this_predalgo_ eq "na_e1" ) ||
    ( $this_predalgo_ eq "na_e3" ) ||
    ( $this_predalgo_ eq "na_e5" ) ||
    ( $this_predalgo_ eq "ac_e3" ) ||
    ( $this_predalgo_ eq "ac_e5" ) ||
    ( $this_predalgo_ eq "na_m4" ) )
{
  my $average_event_count_per_second_ = 1.0;
  my $is_valid_ = 0;

  if ( $shortcode_ && $start_hhmm_ && $end_hhmm_ ) {
    my $date_ = GetIsoDateFromStrMin1("TODAY-1");
    my ($t_avg_, $is_valid_) = GetFeatureAverageDays ( $shortcode_, $date_, 30, "L1EVPerSec", undef, $start_hhmm_, $end_hhmm_, 1);
    $average_event_count_per_second_ = $t_avg_ if $is_valid_ > 0;
  }
  if ( $is_valid_ <= 0 && -e $this_day_timed_data_filename_ ) {
    $average_event_count_per_second_ = `$MODELSCRIPTS_DIR/get_average_event_count_per_second.pl $this_day_timed_data_filename_`; chomp ( $average_event_count_per_second_ );
  }
  $this_pred_counters_ = $average_event_count_per_second_ * $this_pred_duration_seconds_;
}
print "$this_pred_counters_\n";
