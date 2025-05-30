# \file GenPerlLib/get_yyyymmdd_from_unixtime.pl
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
use feature "switch";

sub GetYYYYMMDDFromUnixTime
{
    my $unixtime_ = shift;
    
    my ($dd_, $mm_, $yyyy_) = (localtime($unixtime_))[3,4,5];
    my $yyyymmdd_ = ( 10000 * ( $yyyy_ + 1900 ) ) + ( 100 * ( $mm_ + 1 ) ) + $dd_;

    $yyyymmdd_;
}

#if ( $0 eq __FILE__ ) {
#    print GetYYYYMMDDFromUnixTime ( $ARGV[0] );
#}

1
