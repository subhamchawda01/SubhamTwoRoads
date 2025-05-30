
# \file GenPerlLib/holiday.pl
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

use POSIX qw(strftime);
#exchange holidays and other days to delete can also be added here - TODO
sub Holiday
{
    my ($year, $month, $day ) = @_;

    my $yyyymmdd_ = $year.$month.$day;

    my $wday = strftime( "%u", 0, 0, 0, $day, $month -1, $year - 1900, -1, -1, -1 );
    if ($wday == 6 || $wday == 7)
    {
	1;
    }
    else 
    {
	0;
    }
}

1;
