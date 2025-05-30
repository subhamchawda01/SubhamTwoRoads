# \file GenPerlLib/break_date_yyyy_mm_dd.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#

sub BreakDateYYYYMMDD {
    my $tradingdate_ = shift;
    my $yyyy_ = substr ( $tradingdate_, 0, 4 );
    my $mm_ = substr ( $tradingdate_, 4, 2 );
    my $dd_ = substr ( $tradingdate_, 6, 2 );
    $yyyy_, $mm_, $dd_ ;
}

1;
