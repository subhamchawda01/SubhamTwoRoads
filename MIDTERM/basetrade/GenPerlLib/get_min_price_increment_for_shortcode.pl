# \file GenPerlLib/get_min_price_increment_for_shortcode.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
use strict;
use warnings;
use feature "switch";

sub GetMinPriceIncrementForShortcode
{
    my $shortcode_ = shift;

    my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );

    my $USER=$ENV{'USER'};
    my $HOME_DIR=$ENV{'HOME'};
    my $REPO="basetrade";

    my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
    if ( $USER eq "rkumar" ) 
    { 
	$LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
    }
    if ( $USER eq "sghosh" || $USER eq "ravi" || $USER eq "mayank" || $USER eq "ankit") 
    { 
	$LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
    }

    my $min_price_increment_ = `$LIVE_BIN_DIR/get_min_price_increment $shortcode_ $yyyymmdd_` ; chomp ( $min_price_increment_ );

    return $min_price_increment_
}

1
