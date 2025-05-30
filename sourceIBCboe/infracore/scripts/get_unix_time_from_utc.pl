#!/usr/bin/perl

# \file GenPerlLib/get_unix_time_from_utc.pl
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
use POSIX;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD

sub GetUnixtimeFromUTC
{
    my ( $yyyymmdd_ , $utc_hhmm_ ) = @_;

    my ( $yyyy_ , $mm_ , $dd_ ) = BreakDateYYYYMMDD ( $yyyymmdd_ );

    my $hour_ = substr ( $utc_hhmm_ , 0 , 2 );
    my $min_ = substr ( $utc_hhmm_ , 2 , 2 );

    if ( $utc_hhmm_ < 1000 ) # 3 digits
    {
	$hour_ = substr ( $utc_hhmm_ , 0 , 1 );
	$min_ = substr ( $utc_hhmm_ , 1 , 2 );
    }

    # mktime(sec, min, hour, mday, mon, year, wday = 0, yday = 0, isdst = 0)

    my $unix_timestamp_ = mktime ( 0 , $min_ , $hour_ , $dd_ , ( $mm_ - 1 ) , ( $yyyy_ - 1900 ) );

    return $unix_timestamp_;
}

1;
