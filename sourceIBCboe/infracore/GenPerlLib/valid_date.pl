
# \file GenPerlLib/valid_date.pl
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

sub ValidDate 
{
    my $input_date = shift;
    my $MINVALIDDATE_ = 20090101;
    my $MAXVALIDDATE_ = 20150101;
    if ( ( $input_date > $MINVALIDDATE_ ) &&
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
