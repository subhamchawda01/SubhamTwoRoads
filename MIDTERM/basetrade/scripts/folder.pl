#!/usr/bin/perl
use strict;
use warnings;
use POSIX;
use feature "switch";
use Fcntl qw (:flock);
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/;
use FileHandle;
use sigtrap qw(handler signal_handler normal-signals error-signals);

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade"; 
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib"; 
require "$GENPERLLIB_DIR/date_utils.pl"; 

my $time_period_ = $ARGV [ 0 ];
my $hh_ = ` date +%H%M`; chomp ( $hh_ );

if ( $#ARGV > 0 ) {
	$hh_ = $ARGV [ 1 ];
	$hh_=`echo $hh_ | cut -d'_' -f2`;
	chomp($hh_);
}

print int($hh_ > GetUTCTime( GetEndTimeFromTP($time_period_)) );
