# \file GenPerlLib/break_date_yyyy_mm_dd.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#

sub BreakDateYYYYMMDD {
    my $tradingdate_ = shift;
    my $yyyy_ = substr ( $tradingdate_, 0, 4 );
    my $mm_ = substr ( $tradingdate_, 4, 2 );
    my $dd_ = substr ( $tradingdate_, 6, 2 );
    $yyyy_, $mm_, $dd_ ;
}

1;
