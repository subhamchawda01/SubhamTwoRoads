# \file GenPerlLib/skip_combo_book_indicator.pl
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

sub SkipComboBookIndicator
{
    my $self_shortcode_ = shift;
    my $combo_shortcode_ = shift;
    my $indicator_filename_ = shift;

    my $retval = 0;

    if ( $indicator_filename_ =~ m/MultM*Combo/ )
    {
	$retval = 1;
	if( ( ( $self_shortcode_ =~ m/BR_[WI][IN][ND]_0/ ) && ( $combo_shortcode_ =~ m/IND2/ ) ) ||
	    ( ( $self_shortcode_ =~ m/NKM?_[0-4]/ )        && ( $combo_shortcode_ =~ m/OSENIK/ ) )
	    )
	{
	    $retval = 0;
	}
    }

    $retval;
}

1;
