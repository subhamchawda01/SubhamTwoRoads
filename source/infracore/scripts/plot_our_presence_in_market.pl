#!/usr/bin/perl

# \file scripts/plot_our_presence_in_market.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "infracore";

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

# start
my $USAGE="$0 SHORTCODE TIMEPERIOD DATE SUMMARY/PLOT COMP-OPTIONS";

if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];
my $yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 2 ] );
my $summary_ = $ARGV [ 3 ];
my $comp_options_ = $ARGV [ 4 ];

my @comp_durations_ = ( );

given ( $shortcode_ )
{
    when ( "ZF_0" )
    {
	push ( @comp_durations_ , 1800 );
	push ( @comp_durations_ , 3000 );
    }
    when ( "ZN_0" )
    {
	push ( @comp_durations_ , 1800 );
	push ( @comp_durations_ , 3000 );
    }
    when ( "ZB_0" )
    {
	push ( @comp_durations_ , 1800 );
	push ( @comp_durations_ , 3000 );
    }
    when ( "UB_0" )
    {
	push ( @comp_durations_ , 1800 );
	push ( @comp_durations_ , 3000 );
    }

    when ( "FESX_0" )
    {
	push ( @comp_durations_ , 1800 );
	push ( @comp_durations_ , 3000 );
    }
    when ( "FGBS_0" )
    {
	push ( @comp_durations_ , 3000 );
	push ( @comp_durations_ , 4000 );
    }
    when ( "FGBM_0" )
    {
	push ( @comp_durations_ , 1800 );
	push ( @comp_durations_ , 3000 );
    }
    when ( "FGBL_0" )
    {
	push ( @comp_durations_ , 1500 );
	push ( @comp_durations_ , 2200 );
    }
    when ( "FOAT_0" )
    {
	push ( @comp_durations_ , 3000 );
	push ( @comp_durations_ , 4000 );
    }

    when ( "CGB_0" )
    {
	push ( @comp_durations_ , 1800 );
	push ( @comp_durations_ , 3000 );
    }
    when ( "BAX_1" )
    {
	push ( @comp_durations_ , 3000 );
	push ( @comp_durations_ , 5000 );
    }
    when ( "BAX_2" )
    {
	push ( @comp_durations_ , 3000 );
	push ( @comp_durations_ , 5000 );
    }
    when ( "BAX_3" )
    {
	push ( @comp_durations_ , 3000 );
	push ( @comp_durations_ , 5000 );
    }
    when ( "BAX_4" )
    {
	push ( @comp_durations_ , 3000 );
	push ( @comp_durations_ , 5000 );
    }
    when ( "BAX_5" )
    {
	push ( @comp_durations_ , 3000 );
	push ( @comp_durations_ , 5000 );
    }
}

my $PLOT_OUR_PRESENCE_IN_MARKET = $SCRIPTS_DIR."/plot_our_presence_in_market.sh";
my $UNIX_TO_GMT = $SCRIPTS_DIR."/unixtime2gmtstr.pl";

foreach my $comp_duration_ ( @comp_durations_ )
{
    my $exec_cmd_ = $PLOT_OUR_PRESENCE_IN_MARKET." $shortcode_ $yyyymmdd_ $comp_duration_ $summary_ $comp_options_ 2>/dev/null";
    print $exec_cmd_."\n";

    my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );
    for ( my $exec_line_index_ = 0 ; $exec_line_index_ <= $#exec_output_ ; $exec_line_index_ ++ )
    {
	my $exec_line_ = $exec_output_ [ $exec_line_index_ ];

	if ( index ( $exec_line_ , "START" ) >= 0 )
	{
	    my @exec_line_words_ = split ( ' ' , $exec_line_ );

	    $exec_line_ = sprintf ( "%12s %12s %12s %12s" , $exec_line_words_ [ 0 ] , $exec_line_words_ [ 1 ] , $exec_line_words_ [ 4 ] , $exec_line_words_ [ 11 ] );

	    $exec_output_ [ $exec_line_index_ ] = $exec_line_;
	}
	else
	{
	    my @exec_line_words_ = split ( ' ' , $exec_line_ );

	    my $exec_0_cmd_ = $UNIX_TO_GMT." ".$exec_line_words_ [ 0 ];
	    my $exec_1_cmd_ = $UNIX_TO_GMT." ".$exec_line_words_ [ 1 ];

	    my @exec_0_output_ = `$exec_0_cmd_`; chomp ( @exec_0_output_ );
	    my @exec_1_output_ = `$exec_1_cmd_`; chomp ( @exec_1_output_ );

	    my @concise_exec_0_output_ = split ( ' ' , $exec_0_output_ [ 0 ] );
	    my @concise_exec_1_output_ = split ( ' ' , $exec_1_output_ [ 0 ] );

	    $exec_line_words_ [ 0 ] = $concise_exec_0_output_ [ $#concise_exec_0_output_ - 1];
	    $exec_line_words_ [ 1 ] = $concise_exec_1_output_ [ $#concise_exec_1_output_ - 1];

	    if ( $exec_line_words_ [ 4 ] > 0.0 && $exec_line_words_ [ 11 ] > 0.0 )
	    {
		$exec_line_ = sprintf ( "%12s %12s %12.3f %12.3f" , $exec_line_words_ [ 0 ] , $exec_line_words_ [ 1 ] , $exec_line_words_ [ 4 ] , $exec_line_words_ [ 11 ] );
	    }
	    else
	    {
		$exec_line_ = "";
	    }

	    $exec_output_ [ $exec_line_index_ ] = $exec_line_;
	}
    }

    foreach my $exec_output_line_ ( @exec_output_ )
    {
	if ( $exec_output_line_ )
	{
	    print $exec_output_line_."\n";
	}
    }
}


exit ( 0 );
