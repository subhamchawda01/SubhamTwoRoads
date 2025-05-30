# \file GenPerlLib/find_number_from_vec.pl
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

sub FindNumberFromVec {
    my $substr_ = shift;
    my @textvec_ = @_ ;

    my $found_text_ = "";
    foreach my $t_text_ (@textvec_)
    {
	if ( $t_text_ == $substr_ )
	{
#	    print STDERR "found in $t_text_\n";
	    $found_text_ = $t_text_;
	    last;
	}
    }
    $found_text_;
}

1;
