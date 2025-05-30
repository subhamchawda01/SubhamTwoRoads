# \file GenPerlLib/is_date_holiday.pl
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

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/holiday.pl"; # Holiday

sub IsDateHoliday
{
    my $tradingdate_ = shift;
    my ($tradingdate_YYYY_, $tradingdate_MM_, $tradingdate_DD_) = BreakDateYYYYMMDD ( $tradingdate_ );
    Holiday ( $tradingdate_YYYY_, $tradingdate_MM_, $tradingdate_DD_ ) 
}

1
