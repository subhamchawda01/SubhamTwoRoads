# \file GenPerlLib/is_date_holiday.pl
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

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/holiday.pl"; # Holiday

sub IsDateHoliday
{
    my $tradingdate_ = shift;
    my ($tradingdate_YYYY_, $tradingdate_MM_, $tradingdate_DD_) = BreakDateYYYYMMDD ( $tradingdate_ );
    Holiday ( $tradingdate_YYYY_, $tradingdate_MM_, $tradingdate_DD_ ) 
}

1
