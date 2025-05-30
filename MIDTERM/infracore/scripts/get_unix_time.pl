#!/usr/bin/perl

# \file GenPerlLib/get_unix_time.pl
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
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."/scripts";

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD


sub GetUnixtimeFromUTC ;
sub TimeZoneToCountryCodeHHMM ;


if ( scalar ( @ARGV ) != 2 ) 
{
    print "$0 yyyymmdd xxx_hhmm \n" ;
    exit ( 0  ) ;
}

# this is not really needed ... 
my ( $cc_ , $hhmm_ ) = TimeZoneToCountryCodeHHMM ( $ARGV[1] ) ;

my $date_time_ = `perl $SCRIPTS_DIR/convert_datetime_tz.pl $ARGV[0] $hhmm_"00" $cc_ "UTC" ` ;
my @dt_ = split ( ' ' , $date_time_ ) ;
my $date_ = $dt_[0] ;
my $time_ = $dt_[1] ;

# utc time  
# print "$date_ $time_\n" ;

my $retval = GetUnixtimeFromUTC ( $date_ , $time_ ) ;

print $retval,"\n" ;

sub GetUnixtimeFromUTC
{
    my $yyyymmdd_ = shift ;
    my $utc_hhmm_ = shift ;

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


sub TimeZoneToCountryCodeHHMM
{
    my $tz_ = shift ;
    my $cc_ = "UTC" ;
    my $hhmm_ = $tz_ ;

    if ( index ( $tz_, "EST_" ) != -1 )
    {
	$cc_ = "NY" ;
	$hhmm_ = substr $tz_ , 4 ;
    }
    elsif ( index ( $tz_, "CST_" ) != -1 )
    {
	$cc_ = "CHI" ;
	$hhmm_ = substr $tz_ , 4 ;
    }
    elsif ( index ( $tz_, "CET_" ) != -1 )
    { 
	$cc_ = "FR" ;
	$hhmm_ = substr $tz_ , 4 ;
    }
    elsif ( index ( $tz_, "BRT_" ) != -1 )
    { 
	$cc_ = "BRZ" ;
	$hhmm_ = substr $tz_ , 4 ;	
    }
    elsif ( index ( $tz_, "HKT_" ) != -1 )
    { 
	$cc_ = "HK" ;
	$hhmm_ = substr $tz_ , 4 ;
    }
    elsif ( index ( $tz_, "IST_" ) != -1 )
    { 
	$cc_ = "IND" ;
	$hhmm_ = substr $tz_ , 4 ;
    }
    elsif ( index ( $tz_, "JST_" ) != -1 )
    {
	$cc_ = "TOK" ;
	$hhmm_ = substr $tz_, 4 ;
    }
    elsif ( index ( $tz_, "UTC_" ) != -1 )
    {
	$cc_ = "UTC" ;
	$hhmm_ = substr $tz_, 4 ;
    }
    elsif ( index ( $tz_, "MSK_" ) != -1 )
    {
	$cc_ = "MOS" ;
	$hhmm_ = substr $tz_, 4 ;
    }
    elsif ( index ( $tz_, "BST_" ) != -1 )
    {
	$cc_ = "BST" ;
	$hhmm_ = substr $tz_, 4 ;
    }
    elsif ( index ( $tz_, "LON_" ) != -1 )
    {
	$cc_ = "LON" ;
	$hhmm_ = substr $tz_, 4 ;
    }
    elsif ( index ( $tz_, "AMS_" ) != -1 )
    {
	$cc_ = "AMS" ;
	$hhmm_ = substr $tz_, 4 ;
    }
    elsif ( index ( $tz_, "PAR_" ) != -1 )
    {
	$cc_ = "PAR" ;
	$hhmm_ = substr $tz_, 4 ;
    }
  
    return $cc_, sprintf ( "%04d" , $hhmm_ ) ;
}
