#!/usr/bin/perl
#
# \file scripts/analyse_pick.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
#

use strict;
use warnings;
use feature "switch";
use FileHandle;
use POSIX;
use List::Util qw/max min/; # for max
use File::Basename;
use Term::ANSIColor; 
my $HOME = $ENV{'HOME'};
my $USAGE="$0 shc sd ed strat_dir global_results_dir";
require "$HOME/basetrade_install/GenPerlLib/make_strat_vec_from_dir_and_tt.pl"; # MakeStratVecFromDirAndTT
#require "$HOME/make_strat_vec_from_dir_and_tt.pl"; # MakeStratVecFromDirAndTT
if ( $#ARGV < 4 ){ print "USAGE : ".$USAGE."\n"; exit(0);}

my $shc= $ARGV [ 0 ]; 
my $sd= $ARGV [ 1 ]; 
my $ed= $ARGV [ 2 ]; 
my $strat_dir = $ARGV [ 3 ];
my $glob_results = $ARGV [ 4 ];
my %strat_to_pnl_map_ = ( );
my @picked_regime_strats_ = ( );
my %strat_to_first_regime_map_ = ( );
my %strat_to_second_regime_map_ = ( );
my %strat_to_percentage_improv_ = ( );
#print "shc=$shc given_sd=$given_sd ed=$ed num_strats=$num_strats dir=$dir sort_algo=$sort_algo sfreq=$sfreq hfreq=$hfreq mvol=$mvol\n";

my $uid=`date +%N`;
chomp ($uid);
my $cmd="$HOME/basetrade_install/bin/summarize_strategy_results $shc $strat_dir $glob_results $sd $ed";
my @sum_res_ = `$cmd`;
chomp (@sum_res_);
for ( my $i=0; $i<=$#sum_res_; $i++ )
{
my @res_words_ = split ' ',$sum_res_[$i];
$strat_to_pnl_map_{ $res_words_[1] } = $res_words_[2];
}
for ( my $i=0; $i<=$#sum_res_; $i++ )
{
my @res_words_ = split ' ',$sum_res_[$i];
my @strat_words_ = split '_',$res_words_[1];
if ( $#strat_words_ > 1 )
{
my $strat_1 = "strat_".$strat_words_[1];
my $strat_2 = "strat_".$strat_words_[2];
if (( $strat_to_pnl_map_ { $res_words_[1] } >= $strat_to_pnl_map_ { $strat_1 } ) && ( $strat_to_pnl_map_ { $res_words_[1] } >= $strat_to_pnl_map_ { $strat_2 } ) )
{
my $max_ = $strat_to_pnl_map_ { $strat_1 };
if ( $strat_to_pnl_map_ { $strat_1 } < $strat_to_pnl_map_ { $strat_2 } )
{
$max_ = $strat_to_pnl_map_ { $strat_2 };
}
$strat_to_percentage_improv_ { $res_words_[1] } = ( $strat_to_pnl_map_ { $res_words_[1] } - $max_ ) * 100 / abs($max_);
if ( $strat_to_percentage_improv_ { $res_words_[1] } > 10 )
{
push (@picked_regime_strats_, $res_words_[1]);
if ( ! exists $strat_to_first_regime_map_{$strat_1} )
{
$strat_to_first_regime_map_{$strat_1} = 1;
}
else
{
$strat_to_first_regime_map_{$strat_1} += 1;
}
if ( ! exists $strat_to_second_regime_map_{$strat_2} )
{
$strat_to_second_regime_map_{$strat_2} = 1;
}
else
{
$strat_to_second_regime_map_{$strat_2} += 1;
}
}
}
}
}
print "PICKED REGIME STRATS:\n";
for ( my $i=0; $i<=$#picked_regime_strats_; $i++ )
{
print "$picked_regime_strats_[$i] $strat_to_percentage_improv_{$picked_regime_strats_[$i]}\n";
}

print "REGIME STATS:\n";
print "FIRST REGIME:\n";
foreach my $strat_ (sort { $strat_to_first_regime_map_{$b} <=> $strat_to_first_regime_map_{$a} } keys %strat_to_first_regime_map_)
{
print "$strat_ $strat_to_first_regime_map_{$strat_}\n";
}

print "SECOND REGIME:\n";
foreach my $strat_ (sort { $strat_to_second_regime_map_{$b} <=> $strat_to_second_regime_map_{$a} } keys %strat_to_second_regime_map_)
{
print "$strat_ $strat_to_second_regime_map_{$strat_}\n";
}
