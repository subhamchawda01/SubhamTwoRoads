# \file GenPerlLib/find_item_from_vec_ref.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#

use strict;
use warnings;

sub FindItemFromVecRef {
    my $substr_ = shift ;
    my $array_ref_ = shift ;

    my $found_text_ = "";
    foreach my $t_text_ ( @$array_ref_ )
    {
	if ( $t_text_ eq $substr_ )
	{
#	    print STDERR "found in $t_text_\n";
	    $found_text_ = $t_text_;
	    last;
	}
    }
    $found_text_;
}

1;
