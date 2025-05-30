#!/usr/bin/perl
use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";

my $in_file_ = $ARGV [ 0 ];
my $out_file_ = $ARGV [ 1 ];
my $shortcode_ = $ARGV [ 2 ];
my $datagen_output_ = $ARGV [ 3 ];
my $start_time_ = $ARGV [ 4 ];
my $end_time_ = $ARGV [ 5 ];

open CONFIG_FILE, "< $in_file_";
open OUT_FILE, "> $out_file_";
while ( my $config_line_ = <CONFIG_FILE> )
{
    chomp($config_line_);
    my @config_words_ = split ' ', $config_line_;

    my $print_pred_counters_for_this_pred_algo_script = $MODELSCRIPTS_DIR."/print_pred_counters_for_this_pred_algo.pl" ; # replace this SearchScript in future
    my $cmd = "$print_pred_counters_for_this_pred_algo_script ".$shortcode_." ".$config_words_[ 0 ]." ".$config_words_[ 1 ]." ".$datagen_output_." $start_time_ $end_time_";
    print $cmd;
    
    my $pred_counter = `$cmd`;
    chomp ( $pred_counter );
    my $t_string = $pred_counter." ".$config_words_[ 1 ]."\n";
    print OUT_FILE $t_string;
    

}
close OUT_FILE;
close CONFIG_FILE;
