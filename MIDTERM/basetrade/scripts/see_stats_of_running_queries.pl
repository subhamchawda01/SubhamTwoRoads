#!/usr/bin/perl
use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
require "$GENPERLLIB_DIR/get_trading_location_for_shortcode.pl"; # GetTradingLocationForShortcode

my $usage_ = "$0  SHORTCODE  NUMDAYS";

if ($#ARGV < 1) {
    print $usage_."\n";
    exit (0);
}

my $dep_shortcode_ = $ARGV[0];
my $num_days_ = $ARGV[1];

my $today_yyyymmdd_ = `date +%Y%m%d`; chomp ( $today_yyyymmdd_ );
my $trading_location_ = GetTradingLocationForShortcode ( $dep_shortcode_ , $today_yyyymmdd_ );

system ( "$HOME_DIR/basetrade/scripts/see_stats_of_running_queries.sh $trading_location_ $dep_shortcode_ $num_days_" );
