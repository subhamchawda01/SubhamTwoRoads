# \file GenPerlLib/skip_weird_date.pl
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

sub SkipWeirdDate 
{
    my $input_date = shift;

    if ( $input_date == 20110704 
	 || $input_date == 20110712  
	 || $input_date == 20110728  
	 || $input_date == 20110826 # lost data again
	 || $input_date == 20110905 # labor day
	 || $input_date == 20111010 # columbus day
	 || $input_date == 20111111 # veteran's day
	 || $input_date == 20111124 # thanksgiving
	 || $input_date == 20111125 # thanksgiving
	 || $input_date == 20111222 # christmas
	 || $input_date == 20111223 # christmas
	 || $input_date == 20111226 # christmas
	 || $input_date == 20111227 # christmas
	 || $input_date == 20111229 # new years
	 || $input_date == 20111230 # new years
	 || $input_date == 20120102 # new years
	 || $input_date == 20120116 # martin luther king day
	 || $input_date == 20120220 # president's day
	 || $input_date == 20120228 # bad data day 1
	 || $input_date == 20120314 # some data missing
	 || $input_date == 20120406 # good friday
	 || $input_date == 20120409 # easter monday
	 || $input_date == 20120501 # europe labor day
	 || $input_date == 20120528 # memorial day
	 || $input_date == 20120704 # july 4th
	)
    {
	1;
    }
    else 
    {
	0;
    }
}

1;
