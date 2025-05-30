# \file GenPerlLib/get_pred_counters_for_this_pred_algo.pl.pl
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
my $REPO="infracore";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/get_avg_event_count_per_sec_for_shortcode.pl"; # GetAvgEventCountPerSecForShortcode

sub GetPredCountersForThisPredAlgo
{
    my ( $shortcode_ , $this_pred_duration_seconds_ , $this_predalgo_ , $this_day_timed_data_filename_ ) = @_;
    my $this_pred_counters_ = 0;
    given ( $this_predalgo_ )
    {
	when ("na_t1")
	{
	    $this_pred_counters_ = 1000 * $this_pred_duration_seconds_;
	}
	when ("na_t3")
	{
	    $this_pred_counters_ = 1000 * $this_pred_duration_seconds_;
	}
	when ("na_e1")
	{
	    my $average_event_count_per_second_ = 0.0;

	    if ( ! $shortcode_ )
	    {
		$average_event_count_per_second_ = `$MODELSCRIPTS_DIR/get_average_event_count_per_second.pl $this_day_timed_data_filename_`; chomp ( $average_event_count_per_second_ );
	    }
	    else
	    {
		$average_event_count_per_second_ = GetAvgEventCountPerSecForShortcode ( $shortcode_ );
	    }
	    
	    $this_pred_counters_ = $average_event_count_per_second_ * $this_pred_duration_seconds_;
	}
	when ("na_e3")
	{
	    my $average_event_count_per_second_ = 0.0;

	    if ( ! $shortcode_ )
	    {
		$average_event_count_per_second_ = `$MODELSCRIPTS_DIR/get_average_event_count_per_second.pl $this_day_timed_data_filename_`; chomp ( $average_event_count_per_second_ );
	    }
	    else
	    {
		$average_event_count_per_second_ = GetAvgEventCountPerSecForShortcode ( $shortcode_ );
	    }

	    $this_pred_counters_ = $average_event_count_per_second_ * $this_pred_duration_seconds_;
	}
	when ("na_e5")
	{
	    my $average_event_count_per_second_ = 0.0;

	    if ( ! $shortcode_ )
	    {
		$average_event_count_per_second_ = `$MODELSCRIPTS_DIR/get_average_event_count_per_second.pl $this_day_timed_data_filename_`; chomp ( $average_event_count_per_second_ );
	    }
	    else
	    {
		$average_event_count_per_second_ = GetAvgEventCountPerSecForShortcode ( $shortcode_ );
	    }

	    $this_pred_counters_ = $average_event_count_per_second_ * $this_pred_duration_seconds_;
	}
	when ("ac_e3")
	{
	    my $average_event_count_per_second_ = 0.0;

	    if ( ! $shortcode_ )
	    {
		$average_event_count_per_second_ = `$MODELSCRIPTS_DIR/get_average_event_count_per_second.pl $this_day_timed_data_filename_`; chomp ( $average_event_count_per_second_ );
	    }
	    else
	    {
		$average_event_count_per_second_ = GetAvgEventCountPerSecForShortcode ( $shortcode_ );
	    }

	    $this_pred_counters_ = $average_event_count_per_second_ * $this_pred_duration_seconds_;
	}
	default
	{
	    $this_pred_counters_ = 1000 * $this_pred_duration_seconds_;
	}
    }
    $this_pred_counters_;
}

1;
