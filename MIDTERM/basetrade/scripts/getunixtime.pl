#!/usr/bin/perl

use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $USAGE="$0 date time";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $date_ = $ARGV[0];
my $time_ = $ARGV[1];

require "$GENPERLLIB_DIR/get_unix_time_from_utc.pl"; # GetUnixtimeFromUTC
require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr

my $utc_time_ = GetUTCHHMMStr( $time_, $date_ );

my $unix_time_ = GetUnixtimeFromUTC( $date_, $utc_time_ );

print $unix_time_."\n";

