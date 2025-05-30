# \file GenPerlLib/find_item_from_vec_ref.pl
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

sub FindItemFromVecRef 
{
    my $substr_ = shift ;
    my $array_ref_ = shift ;

    my $found_text_ = "";
    foreach my $t_text_ ( @$array_ref_ )
    {
	if ( $t_text_ eq $substr_ )
	{
	    $found_text_ = $t_text_;
	    last;
	}
    }
    $found_text_;
}

1;
