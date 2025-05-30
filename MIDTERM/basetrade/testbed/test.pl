#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";


require "$GENPERLLIB_DIR/get_avg_event_count_per_sec_for_shortcode.pl"; # GetAvgEventCountPerSecForShortcode

my $prod_ = $ARGV[0];
my $start_time_ = $ARGV[1];
my $end_time__ = $ARGV[2];
my $end_date_ = $ARGV[3];


my $l1events_print_ = GetAvgEventCountPerSecForShortcode ( $prod_, $start_time_, $end_time_, $end_date_ );

print $l1events_print_."\n";
