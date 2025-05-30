# \file GenPerlLib/skip_weird_date.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#        Suite 217, Level 2, Prestige Omega,
#        No 104, EPIP Zone, Whitefield,
#        Bangalore - 560066, India
#        +91 80 4060 0717
#

use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";
my $PSCRIPT_DIR="/home/pengine/prod/live_scripts";

require "$PSCRIPT_DIR/holiday_manager.pl";

sub IsBMFHoliday 
{
    my $input_date = shift;
    $input_date = int ( $input_date ) ;
    return IsExchangeHoliday("BMF", $input_date);
}

sub IsBMFHolidayIncludingWeekends
{
    my $input_date = shift;
    $input_date = int ( $input_date );
    return IsExchangeHolidayIncludingWeekends("BMF", $input_date);
}

1;

