
# \file GenPerlLib/valid_date.pl
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
use Scalar::Util qw(looks_like_number);

sub ValidDate 
{
    my $input_date = shift;
    my $MINVALIDDATE_ = 20090101;
    my $MAXVALIDDATE_ = 20500101;
    if ( looks_like_number($input_date) && ( $input_date > $MINVALIDDATE_ ) &&
	 ( $input_date < $MAXVALIDDATE_ ) )
    {
	1;
    }
    else 
    {
	0;
    }
}

1;
