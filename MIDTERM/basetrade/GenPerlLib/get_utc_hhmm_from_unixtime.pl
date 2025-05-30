# \file GenPerlLib/get_utc_hhmm_from_unixtime.pl
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

sub GetUTCHHMMFromUnixTime
{
    my $unixtime_ = shift;

    my ($t_sec_,$t_min_,$t_hour_,$t_mday_,$t_mon_,$t_year_,$t_wday_,$t_yday_,$t_isdst_) = gmtime($unixtime_);

# TODO ... move to second precicion
#    my $hhmm_ = sprintf ( "UTC_%02d%02d.%2d", $t_hour_, $t_min_, $t_sec_ ) ;
    my $hhmm_ = sprintf ( "UTC_%02d%02d", $t_hour_, $t_min_ ) ;
    $hhmm_;
}

1
