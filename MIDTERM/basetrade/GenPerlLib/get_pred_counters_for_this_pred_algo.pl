# \file GenPerlLib/get_pred_counters_for_this_pred_algo.pl.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
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

sub GetPredCountersForThisPredAlgo
{
    my ( $shortcode_ , $this_pred_duration_seconds_ , $this_predalgo_ , $this_day_timed_data_filename_ ) = @_;
    my $this_pred_counters_ = 1000 * $this_pred_duration_seconds_; # default

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
	if ( ( ! $shortcode_ ) && ( -e $this_day_timed_data_filename_ ) )
	{
	    $average_event_count_per_second_ = `$MODELSCRIPTS_DIR/get_average_event_count_per_second.pl $this_day_timed_data_filename_`; chomp ( $average_event_count_per_second_ );
	}
	else
	{
	    $average_event_count_per_second_ = GetAvgEventCountPerSecForShortcode ( $shortcode_ );
	}
	
	$this_pred_counters_ = $average_event_count_per_second_ * $this_pred_duration_seconds_;
    }
    $this_pred_counters_;
}

1;
