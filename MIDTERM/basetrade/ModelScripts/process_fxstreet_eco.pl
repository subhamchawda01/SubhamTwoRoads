#!/usr/bin/perl

# \file ModelScripts/process_fxstreet_eco.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# Source: http://www.fxstreet.com/fundamental/economic-calendar/
# This script takes output of FXStreet export to CSV of economic calendar, and 
# outputs a file that is easy for CommonTradeUtils/EconomicEventsManager 
# to load

use strict;
use warnings;
use feature "switch"; # for given, when
use DateTime;

sub ShortenEcoZone;
sub FXSConvertUTCEpoch;
sub CleanQuotesAndAccidentalComma;

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";

# require "$GENPERLLIB_DIR/array_ops.pl";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0 fxs_eco_output.csv";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $fxs_eco_filename_ = $ARGV[0];

open FXS_ECO_FILEHANDLE, "< $fxs_eco_filename_ " or PrintStacktraceAndDie ( "Could not open $fxs_eco_filename_\n" ) ;

while ( my $thisline_ = <FXS_ECO_FILEHANDLE> )
{
    $thisline_ = CleanQuotesAndAccidentalComma ( $thisline_ );
    my @this_words_ = split ( ',', $thisline_ );

#    print "$#this_words_\n";
#    print "$#this_words_ $thisline_\n";

    if ( ( $#this_words_ == 6 )
	 && ( length ( $this_words_[1] ) >= 17 ) 
	 && ( ! ( $this_words_[0] eq "country" ) ) 
	 && ( ! ( $this_words_[0] eq "Country" ) ) 
	)
    {
    	my $eco_zone_ = ShortenEcoZone ( $this_words_[0] );
	my ( $epoch_time_, $utc_time_string_ ) = FXSConvertUTCEpoch ( $this_words_[1] );
    	my $concat_event_name_ = ConcatEventName ( $this_words_[2] );
	my $event_severity_ = $this_words_[6];
    	printf ( "%d %s %s %d %s\n", $epoch_time_, $eco_zone_, $concat_event_name_, $event_severity_, $utc_time_string_ );
    }
}

close FXS_ECO_FILEHANDLE;

sub FXSConvertUTCEpoch
{
    my $est_time_str_ = shift;
    
    my $est_time_year_ = substr ( $est_time_str_, 0, 4 );
    my $est_time_month_= substr ( $est_time_str_, 4, 2 );
    my $est_time_day_  = substr ( $est_time_str_, 6, 2 );
    my $est_time_hour_ = substr ( $est_time_str_, 9, 2 );
    my $est_time_minute_ = substr ( $est_time_str_, 12, 2 );
    
    my $dt = DateTime->new ( year   => $est_time_year_, month  => $est_time_month_, day    => $est_time_day_, 
			     hour   => $est_time_hour_, minute => $est_time_minute_, 
			     second => 0, nanosecond => 0, time_zone => 'UTC' );
    my $epoch_time_ = $dt->epoch;

    $dt->set_time_zone ( 'UTC' );

    my $utc_time_string_ = sprintf ( "%s%02d%02d_%02d:%02d:%02d_UTC", $dt->year, $dt->month, $dt->day, $dt->hour, $dt->minute, $dt->second );

    $epoch_time_,$utc_time_string_ ;
}

sub ConcatEventName
{
    my $fxs_event_name_ = shift;
    my @fxs_event_words_ = split ( ' ', $fxs_event_name_ );
    my $concat_event_name_ = join ( '_', @fxs_event_words_ );
    $concat_event_name_;
}

sub CleanQuotesAndAccidentalComma
{
    my $line_with_comma_ = shift;
    $line_with_comma_ =~ s/Food, Energy/Food Energy/g;
    $line_with_comma_ =~ s/"//g;
    # my @words_ = split ( '"', $line_with_comma_ );
    # for ( my $i = 0; $i <= $#words_; $i ++ )
    # {
    # 	printf ( "%d ^%s^\n", $i,$words_[$i] );
    # }
    # PrintStacktraceAndDie ( join ( '', @words_ ) );
    # PrintStacktraceAndDie ( $line_with_comma_ );
    $line_with_comma_;
}

sub ShortenEcoZone
{
    my $long_eco_zone_name_ = shift;
    my $short_eco_zone_name_ = "USD";

    given ( $long_eco_zone_name_ ) 
    {
	when ("Argentina")
	{
	    $short_eco_zone_name_ = "ARG";
	}
	when ("Australia") 
	{
	    $short_eco_zone_name_ = "AUD";
	}
	when ("Austria")
	{
	    $short_eco_zone_name_ = "AUS";
	}
	when ("Belgium")
	{
	    $short_eco_zone_name_ = "BLG";
	}
	when ("Brazil")
	{
	    $short_eco_zone_name_ = "BRL";
	}
	when ("Canada")
	{
	    $short_eco_zone_name_ = "CAD";
	}
	when ("Chile")
	{
	    $short_eco_zone_name_ = "CLP";
	}
	when ("China")
	{
	    $short_eco_zone_name_ = "CNY";
	}
	when ("Colombia")
	{
	    $short_eco_zone_name_ = "COP";
	}
	when ("Czech Republic")
	{
	    $short_eco_zone_name_ = "CZK";
	}
	when ("Denmark")
	{
	    $short_eco_zone_name_ = "DMK";
	}
	when ("European Monetary Union")
	{
	    $short_eco_zone_name_ = "EUR";
	}
	when ("Finland")
	{
	    $short_eco_zone_name_ = "FIN";
	}
	when ("France")
	{
	    $short_eco_zone_name_ = "FRA";
	}
	when ("Germany")
	{
	    $short_eco_zone_name_ = "GER";
	}
	when ("Greece")
	{
	    $short_eco_zone_name_ = "GRC";
	}
	when ("Hong Kong SAR")
	{
	    $short_eco_zone_name_ = "HKD";
	}
	when ("Hungary")
	{
	    $short_eco_zone_name_ = "HUF";
	}
	when ("Iceland")
	{
	    $short_eco_zone_name_ = "ISK";
	}
	when ("India")
	{
	    $short_eco_zone_name_ = "INR";
	}
	when ("Indonesia")
	{
	    $short_eco_zone_name_ = "IDR";
	}
	when ("Ireland")
	{
	    $short_eco_zone_name_ = "IRE";
	}
	when ("Italy")
	{
	    $short_eco_zone_name_ = "ITA";
	}
	when ("Japan")
	{
	    $short_eco_zone_name_ = "JPY";
	}
	when ("Korea")
	{
	    $short_eco_zone_name_ = "KRW";
	}
	when ("Korea Republic of")
	{
	    $short_eco_zone_name_ = "KRW";
	}
	when ("Mexico")
	{
	    $short_eco_zone_name_ = "MXN";
	}
	when ("Netherlands")
	{
	    $short_eco_zone_name_ = "DUT";
	}
	when ("New Zealand")
	{
	    $short_eco_zone_name_ = "NZD";
	}
	when ("Norway")
	{
	    $short_eco_zone_name_ = "NOK";
	}
	when ("Peru")
	{
	    $short_eco_zone_name_ = "PEN";
	}
	when ("Poland")
	{
	    $short_eco_zone_name_ = "PLN";
	}
	when ("Portugal")
	{
	    $short_eco_zone_name_ = "POR";
	}
	when ("Romania")
	{
	    $short_eco_zone_name_ = "ROL";
	}
	when ("Russia")
	{
	    $short_eco_zone_name_ = "RUB";
	}
	when ("Singapore")
	{
	    $short_eco_zone_name_ = "SGD";
	}
	when ("Slovakia")
	{
	    $short_eco_zone_name_ = "SLV";
	}
	when ("South Africa")
	{
	    $short_eco_zone_name_ = "ZAR";
	}
	when ("Spain")
	{
	    $short_eco_zone_name_ = "SPA";
	}
	when ("Sweden")
	{
	    $short_eco_zone_name_ = "SEK";
	}
	when ("Switzerland")
	{
	    $short_eco_zone_name_ = "CHF";
	}
	when ("Turkey")
	{
	    $short_eco_zone_name_ = "TRY";
	}
	when ("United Kingdom")
	{
	    $short_eco_zone_name_ = "GBP";
	}
	when ("United States")
	{
	    $short_eco_zone_name_ = "USD";
	}
	when ("All") #G20 meeting etc, use USD here and adjust( probably reduce ) severity of such events
	{
	    $short_eco_zone_name_ = "USD";
	}
	default
	{
	    $short_eco_zone_name_ = "---";
	    print STDERR "$long_eco_zone_name_ not expected\n";
	}
    }
    $short_eco_zone_name_;
}
