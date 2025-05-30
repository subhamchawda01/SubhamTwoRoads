#!/usr/bin/perl

# \file scripts/remove_indicators_from_list.pl
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
use FileHandle;

my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER ne "dvctrader" )
{
    $LIVE_BIN_DIR = $BIN_DIR;
}

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

# start
my $USAGE="$0 ILIST_FILE BAD_ILIST_FILE [REMOVE/KEEP]";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $ilist_file_ = $ARGV [ 0 ];
my $bad_ilist_file_ = $ARGV [ 1 ];
my $to_remove_ = 1;

if ( ( $#ARGV > 1 ) && ( index ( $ARGV [ 2 ] , "K" ) == 0 ) )
{
    $to_remove_ = 0;
}

my $random_id_ = `date +%N`; $random_id_ = $random_id_ + 0;
my $edited_ilist_file_ = $ilist_file_.$random_id_;

open ( BAD_ILIST_FILE , "<" , $bad_ilist_file_ ) or PrintStacktraceAndDie ( "Could not open config file $bad_ilist_file_" );
my @bad_indicators_ = <BAD_ILIST_FILE>; chomp ( @bad_indicators_ );
close ( BAD_ILIST_FILE );

foreach my $bad_indicator_line_ ( @bad_indicators_ )
{
    # OnlineComputedCutoffPair HHI_0 NKM_0 0.25 MktSizeWPrice # TSTAT 0.937115 CORR 0.0215105
    # to
    # OnlineComputedCutoffPair HHI_0 NKM_0 0.25 MktSizeWPrice
    my @t_indicator_words_ = split ( ' ' , $bad_indicator_line_ ); chomp ( @t_indicator_words_ );

    my $t_bad_indicator_line_ = $t_indicator_words_ [ 0 ];
    for ( my $i = 1 ; $i <= $#t_indicator_words_ && $t_indicator_words_ [ $i ] ne "#" ; $i ++ )
    {
	$t_bad_indicator_line_ = $t_bad_indicator_line_." ".$t_indicator_words_ [ $i ];
    }

    my $exec_cmd_ = "";

    if ( $to_remove_ )
    {
	$exec_cmd_ = "grep -v \"".$t_bad_indicator_line_."\" ".$ilist_file_." > $edited_ilist_file_";
    }
    else
    {
	$exec_cmd_ = "grep \"".$t_bad_indicator_line_."\" ".$ilist_file_." > $edited_ilist_file_";
    }

    `$exec_cmd_`;

    `mv $edited_ilist_file_ $ilist_file_`;
}

exit ( 0 );
