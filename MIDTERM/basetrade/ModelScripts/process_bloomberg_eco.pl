#!/usr/bin/perl

# \file ModelScripts/process_bloomberg_eco.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes output of BBG-eco command, and 
# outputs a file that is easy for CommonTradeUtils/EconomicEventsManager
# to load

use strict;
use warnings;
use feature "switch"; # for given, when

sub ConvertESTEpoch;
sub ShortenEcoZone;

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";

# require "$GENPERLLIB_DIR/array_ops.pl";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0 bbg_eco_output.csv";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $bbg_eco_filename_ = $ARGV[0];

open BBG_ECO_FILEHANDLE, "< $bbg_eco_filename_ " or PrintStacktraceAndDie ( "Could not open $bbg_eco_filename_\n" );

while ( my $thisline_ = <BBG_ECO_FILEHANDLE> )
{
    my @this_words_ = split ( ',', $thisline_ );
    
    if ( ( $#this_words_ >= 9 ) &&
	 ( length ( $this_words_[0] ) >= 16 ) &&
	 ( ! ( $this_words_[0] =~ /Download/ ) ) )
    {
	my ( $epoch_time_, $utc_time_string_ ) = ConvertESTEpoch ( $this_words_[0] );
	my $eco_zone_ = ShortenEcoZone ( $this_words_[1] );
	my $concat_event_name_ = ConcatEventName ( $this_words_[4] );
	my $default_severity_ = 2;
	printf ( "%d %s %s %d %s\n", $epoch_time_, $eco_zone_, $concat_event_name_, $default_severity_, $utc_time_string_ );
    }
}

close BBG_ECO_FILEHANDLE;

sub ConvertESTEpoch
{
    my $est_time_str_ = shift;
    
    my $est_time_year_ = substr ( $est_time_str_, 6, 4 );
    my $est_time_month_= substr ( $est_time_str_, 0, 2 );
    my $est_time_day_  = substr ( $est_time_str_, 3, 2 );
    my $est_time_hour_ = substr ( $est_time_str_, 11, 2 );
    my $est_time_minute_ = substr ( $est_time_str_, 14, 2 );
    
    use DateTime;
    my $dt = DateTime->new ( year   => $est_time_year_, month  => $est_time_month_, day    => $est_time_day_, 
			     hour   => $est_time_hour_, minute => $est_time_minute_, 
			     second => 0, nanosecond => 0, time_zone => 'America/New_York' );
    my $epoch_time_ = $dt->epoch;

    $dt->set_time_zone ( 'UTC' );

    my $utc_time_string_ = sprintf ( "%s%02d%02d_%02d:%02d:%02d_UTC", $dt->year, $dt->month, $dt->day, $dt->hour, $dt->minute, $dt->second );

    $epoch_time_,$utc_time_string_ ;
}

sub ConcatEventName
{
    my $bbg_event_name_ = shift;
    my @bbg_event_words_ = split ( ' ', $bbg_event_name_ );
    my $concat_event_name_ = join ( '_', @bbg_event_words_ );
    $concat_event_name_;
}

sub ShortenEcoZone
{
    my $long_eco_zone_name_ = shift;
    my $short_eco_zone_name_ = "USD";

    given ( $long_eco_zone_name_ ) 
    {
	when ("Australia") 
	{
	    $short_eco_zone_name_ = "AUS";
	}
	when ("Canada")
	{
	    $short_eco_zone_name_ = "CAD";
	}
	when ("China")
	{
	    $short_eco_zone_name_ = "CNY";
	}
	when ("European Monetary Union")
	{
	    $short_eco_zone_name_ = "EMU";
	}
	when ("Germany")
	{
	    $short_eco_zone_name_ = "GER";
	}
	when ("Japan")
	{
	    $short_eco_zone_name_ = "JPY";
	}
	when ("New Zealand")
	{
	    $short_eco_zone_name_ = "NZD";
	}
	when ("Switzerland")
	{
	    $short_eco_zone_name_ = "CHF";
	}
	when ("United Kingdom")
	{
	    $short_eco_zone_name_ = "GBP";
	}
	when ("United States")
	{
	    $short_eco_zone_name_ = "USD";
	}
	when ("US")
	{
	    $short_eco_zone_name_ = "USD";
	}
	when ("BZ")
	{
	    $short_eco_zone_name_ = "BRL";
	}
	default
	{
	    $short_eco_zone_name_ = "USD";
	}
    }
    $short_eco_zone_name_;
}
