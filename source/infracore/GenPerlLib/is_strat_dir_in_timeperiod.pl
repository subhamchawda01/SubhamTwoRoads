# \file GenPerlLib/is_strat_dir_in_timeperiod.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
# This script takes inputs like :
# DIRNAME = UTC_715-UTC_1015
# TIME_DURATION = [ EU_MORN_DAY | EU_MORN_DAY_US_DAY | US_EARLY_MORN | US_MORN | US_DAY | US_MORN_DAY  ]
# and returns if it false in that period

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="infracore";

my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

sub IsStratDirInTimePeriod
{
    my $dirname_ = shift;
    my $timeperiod_ = shift;

    my $retval = 0;

    if ( ( $timeperiod_ eq "EU_MORN_DAY" ) &&
	 ( ( $dirname_ eq "EST_230-EST_830" ) ||
	   ( $dirname_ eq "UTC_630-UTC_900" ) ||
	   ( $dirname_ eq "EST_610-EST_930" ) ||
	   ( $dirname_ eq "UTC_700-UTC_1200" ) ||
	   ( $dirname_ eq "UTC_715-UTC_915" ) ||
	   ( $dirname_ eq "UTC_715-UTC_1015" ) ||
	   ( $dirname_ eq "UTC_715-UTC_1200" ) ) )
    { $retval = 1; }

    if ( ( $timeperiod_ eq "US_MORN_DAY" ) &&
	 ( ( $dirname_ eq "EST_700-EST_1430" ) ||
	   ( $dirname_ eq "EST_700-EST_1450" ) ||
	   ( $dirname_ eq "EST_800-EST_1200" ) ||
	   ( $dirname_ eq "EST_800-EST_1200-DT" ) ||
	   ( $dirname_ eq "EST_830-EST_930" ) ||
	   ( $dirname_ eq "EST_830-EST_1145" ) ||
	   ( $dirname_ eq "EST_830-EST_1245" ) ||
	   ( $dirname_ eq "EST_830-EST_1330" ) ||
	   ( $dirname_ eq "EST_830-EST_1430" ) ||
	   ( $dirname_ eq "EST_830-EST_1545" ) ||
	   ( $dirname_ eq "EST_830-EST_1600" ) ||
	   ( $dirname_ eq "EST_915-EST_1200" ) ||
	   ( $dirname_ eq "EST_915-EST_1545" ) ||
	   ( $dirname_ eq "EST_915-EST_1600" ) ||
	   ( $dirname_ eq "EST_930-EST_1200" ) ||
	   ( $dirname_ eq "EST_935-EST_1200" ) ||
	   ( $dirname_ eq "EST_930-EST_1245" ) ||
	   ( $dirname_ eq "EST_935-EST_1245" ) ||
	   ( $dirname_ eq "EST_930-EST_1400" ) ||
	   ( $dirname_ eq "EST_930-EST_1545" ) ||
	   ( $dirname_ eq "EST_935-EST_1545" ) ||
	   ( $dirname_ eq "EST_935-EST_1600" ) 
	 ) )
    { $retval = 1; }

    if ( $retval == 0 )
    { # not handled above 
	my $start_str_ = "CET_800";
	my $end_str_ = "EST_1600";
	given ( $timeperiod_ )
	{
	    when ( "EU_MORN_DAY" ) 
	    { 
		$start_str_ = "CET_800";
		$end_str_ = "EST_830";
	    }
	    when ( "EU_MORN_DAY_US_DAY" )
	    { 
		$start_str_ = "CET_800";
		$end_str_ = "EST_1600";
	    }
	    when ( "US_EARLY_MORN" )
	    { 
		$start_str_ = "EST_600";
		$end_str_ = "EST_830";
	    }
	    when ( "US_MORN" )
	    { 
		$start_str_ = "EST_700";
		$end_str_ = "EST_945";
	    }
	    when ( "US_DAY" )
	    { 
		$start_str_ = "EST_915";
		$end_str_ = "EST_1600";
	    }
	    when ( "US_MORN_DAY" )
	    { 
		$start_str_ = "EST_600";
		$end_str_ = "EST_1610";
	    }
	    default
	    {
	    }
	}

	if ( index ( $timeperiod_ , "EST" ) >= 0 ||
	     index ( $timeperiod_ , "UTC" ) >= 0 ||
	     index ( $timeperiod_ , "CET" ) >= 0 )
	{ # Very complex time period requirments have necessitated this support.
	    my @time_period_words_ = split ( '-' , $timeperiod_ );
	    if ( $#time_period_words_ == 1 )
	    {
		$start_str_ = $time_period_words_ [ 0 ]; chomp ( $start_str_ );
		$end_str_ = $time_period_words_ [ 1 ]; chomp ( $end_str_ );
	    }
	}

	my @words_ = split ( '-', $dirname_ ) ;
	if ( $#words_ == 1 )
	{
	    my $gststr_ = $words_[0];
	    my $getstr_ = $words_[1];
	    
	    my $st_ = `$BIN_DIR/get_utc_hhmm_str $start_str_`;
	    my $et_ = `$BIN_DIR/get_utc_hhmm_str $end_str_`;
	    
	    my $gst_ = `$BIN_DIR/get_utc_hhmm_str $gststr_`;
	    my $get_ = `$BIN_DIR/get_utc_hhmm_str $getstr_`;
	    
	    if ( ( $gst_ >= $st_ ) && 
		 ( $get_ <= $et_ ) )
	    { $retval = 1; }
	}
    }
    $retval;
}

1
