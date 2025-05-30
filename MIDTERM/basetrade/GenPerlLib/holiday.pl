
# \file GenPerlLib/holiday.pl
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

use POSIX qw(strftime);
#this is only a weekend check, exch/prod holidays are coded in IsExchHoliday && IsProductHoliday
sub Holiday
{
    my ($year, $month, $day ) = @_;

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
