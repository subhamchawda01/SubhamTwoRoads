# \file GenPerlLib/get_default_timeout_values_for_shortcode.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetDefaultTimeoutValuesForShortcode {
    my $shortcode_ = shift;
    
    my $datagen_msecs_timeout_ = 1000;
    my $datagen_l1events_timeout_ = 10;

    if ( $shortcode_ )
    {
	given ( $shortcode_ )
	{
	    when ( "ZF_0" )
	    {
		$datagen_l1events_timeout_ = 8 ;
	    }
	    when ( "ZN_0" )
	    {
		$datagen_l1events_timeout_ = 14 ;
	    }
	    when ( "ZB_0" )
	    {
		$datagen_l1events_timeout_ = 9 ;
	    }
	    when ( "UB_0" )
	    {
		$datagen_l1events_timeout_ = 6 ;
	    }
	    when ( "ES_0" )
	    {
		$datagen_l1events_timeout_ = 27 ;
	    }
	    when ( "FGBL_0" )
	    {
		$datagen_l1events_timeout_ = 12 ;
	    }
	    when ( "FGBM_0" )
	    {
		$datagen_l1events_timeout_ = 9 ;
	    }
	    when ( "FGBS_0" )
	    {
		$datagen_l1events_timeout_ = 3 ;
	    }
	    when ( "FESX_0" )
	    {
		$datagen_l1events_timeout_ = 16 ;
	    }
	    when ( "FDAX_0" )
	    {
		$datagen_l1events_timeout_ = 30 ;
	    }
	}
    }

    $datagen_msecs_timeout_, $datagen_l1events_timeout_;
}

1;
