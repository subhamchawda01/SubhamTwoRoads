#!/usr/bin/perl
#
##file is_valid.pl 
##
## \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
##  Address:
##       Suite No 162, Evoma, #14, Bhattarhalli,
##       Old Madras Road, Near Garden City College,
##       KR Puram, Bangalore 560049, India
##       +91 80 4190 3551
##
## This script takes :
##
## TRADING DATE
## SHORTCODE
use Time::Local;
my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/";
my $FBP_WORK_DIR=$SPARE_HOME."FBP/";

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/check_ilist_data.pl";
#
#
#
if(@ARGV<2) 
{
    print "USAGE : Script Trading_date (YYYYMMDD) Ilist_File\n";
}

else
{
	$tradingdate_ = $ARGV[0];
        $ilist_ = $ARGV[1];
	$tradingdate_ =~ s/\s+$//;
	$tradingdate_ =~ s/^\s*//;
	my ($year, $month, $day) = unpack "A4 A2 A2", $tradingdate_;
	eval
	{ 
    		timelocal(0,0,0,$day, $month-1, $year); # dies in case of bad date
        	if ( SkipWeirdDate ( $tradingdate_ ) || IsDateHoliday ( $tradingdate_ ) || NoDataDate ( $tradingdate_ ) || CheckIndicatorData( $tradingdate_ , $ilist_ ))
    		{
        	   print "0\n";
        	} 
    		else 
        	{
		print "1\n";
        	}
   	} or print "0\n";

}
