#!/usr/bin/perl
#
# \file ModelScripts/find_best_model_for_strategy_var_pert.pl
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
my $MODELLING_DIR = "$HOME/kishenp/RegimeModelling";
my $MODELS_DIR = "$MODELLING_DIR/models";
my $STRATS_DIR = "$MODELLING_DIR/strats";
my $PARAMS_DIR = "$MODELLING_DIR/params";
my $DATABASE_DIR = "$MODELLING_DIR/database";
my $USAGE="$0 shc sd ed start_time end_time regime_ind_list set_of_strats_";

require "$HOME/basetrade_install/GenPerlLib/make_strat_vec_from_dir_and_tt.pl"; # MakeStratVecFromDirAndTT
require "$HOME/basetrade_install/GenPerlLib/get_basepx_strat_first_model.pl"; # MakeStratVecFromDirAndTT
require "$HOME/basetrade_install/GenPerlLib/strat_utils.pl"; # MakeStratVecFromDirAndTT
if ( $#ARGV < 5 ){ print "USAGE : ".$USAGE."\n"; exit(0);}

my $shortcode_ = $ARGV[ 0 ];
my $start_date_ = $ARGV[ 1 ];
my $end_date_ = $ARGV[ 2 ];
my $start_time_ = $ARGV[ 3 ];
my $end_time_ = $ARGV[ 4 ];
my $regime_ind_list_ = $ARGV[ 5 ];
my $set_ = $ARGV[ 6 ];
my $px_type_ = "ALL";

$MODELS_DIR = $MODELS_DIR."/".$shortcode_."/".$start_time_."-".$end_time_;
$STRATS_DIR = $STRATS_DIR."/".$shortcode_."/".$start_time_."-".$end_time_;
$PARAMS_DIR = $PARAMS_DIR."/".$shortcode_."/".$start_time_."-".$end_time_;
$DATABASE_DIR = $DATABASE_DIR."/".$shortcode_."/".$start_time_."-".$end_time_;
if ( !( -e $MODELS_DIR ) )
{
`mkdir -p $MODELS_DIR`;
}
if ( !( -e $STRATS_DIR ) )
{
`mkdir -p $STRATS_DIR`;
}
if ( !( -e $PARAMS_DIR ) )
{
`mkdir -p $PARAMS_DIR`;
}
if ( !( -e $DATABASE_DIR ) )
{
`mkdir -p $DATABASE_DIR`;
}

my $DATABASE_FILE = $DATABASE_DIR."/mapping.txt";
if ( -e $DATABASE_FILE )
{
`rm $DATABASE_FILE`;
}
my @index_to_strat_ = ();

open (SET, "<", $set_ ) or PrintStacktraceAndDie ( "Could not open $set_ for reading" );
my @set_lines_ = <SET>; chomp ( @set_lines_ );
close(SET);

if ( $#ARGV >= 6 ) { $px_type_ = $ARGV [ 6 ]; }
my $timeperiod_ = "$start_time_-$end_time_";
my @temp_all_strats_in_dir_ = MakeStratVecFromDirAndTT("/home/dvctrader/modelling/strats/".$shortcode_, $timeperiod_);
my @all_strats_in_dir_ = ( );
for ( my $i=0; $i<=$#temp_all_strats_in_dir_; $i++ )
{
my $base_p = `basename $temp_all_strats_in_dir_[ $i ]`;
chomp($base_p);
for ( my $j=0; $j<=$#set_lines_; $j++ )
{
if ( $base_p eq $set_lines_[ $j ] )
{
push (@all_strats_in_dir_, $temp_all_strats_in_dir_[ $i ]);
}
}
}
for ( my $i=0; $i<=$#all_strats_in_dir_; $i++ )
{
#print "$all_strats_in_dir_[ $i ]\n";
my $cmd = "cp $all_strats_in_dir_[ $i ] $STRATS_DIR/strat_$i";
`$cmd`;
}
for ( my $i=0; $i<=$#all_strats_in_dir_-1; $i++ )
{
for ( my $j=$i+1; $j<=$#all_strats_in_dir_; $j++ )
{
if ( CanMergeForRegime($all_strats_in_dir_[ $i ], $all_strats_in_dir_[ $j ] )  )
{
#print "Can merge $all_strats_in_dir_[ $i ] $all_strats_in_dir_[ $j ]\n";
MergeStratsWithRegime( $shortcode_, $all_strats_in_dir_[ $i ], $all_strats_in_dir_[ $j ], $i, $j, $regime_ind_list_, $start_time_, $end_time_ );
}
else
{
#print "Can't merge $all_strats_in_dir_[ $i ] $all_strats_in_dir_[ $j ]\n";
}
}
}


sub CanMergeForRegime
{
my ($strat_1, $strat_2) = @_;
my $type_1 = GetStrattype($strat_1);
my $type_2 = GetStrattype($strat_2);
if ($type_1 ne $type_2) { return 0; }
my $regress_1 = GetRegression($strat_1);
my $regress_2 = GetRegression($strat_2);
if ( $regress_1 ne "LINEAR" || $regress_1 ne $regress_2 ) { return 0; }
my $basepx_1 = GetBasepxStratFirstModel($strat_1);
my $basepx_2 = GetBasepxStratFirstModel($strat_2);
if ( $basepx_1 ne $basepx_2 ) { return 0; }
if ( IsRollStrat($strat_1) || IsRollStrat($strat_2) ) { return 0; }
if ( IsRegimeStrat($strat_1) || IsRegimeStrat($strat_2) ) { return 0; }
return 1;
}

sub MergeStratsWithRegime
{
my ($shc_, $strat_1_, $strat_2_, $idx_1_, $idx_2_, $regime_ind_file_, $start_time_, $end_time_ ) = @_;
open ( REGIMES , "<" , $regime_ind_file_ ) or PrintStacktraceAndDie ( "Could not open $regime_ind_file_ for reading" );
my @regimes_ = <REGIMES>; chomp ( @regimes_ );
close (REGIMES);
open ( FILE, ">>", $DATABASE_FILE ) or PrintStacktraceAndDie ( "Could not open $DATABASE_FILE for writing" );

my $model_1_ = GetModel ( $strat_1_ );
my $model_2_ = GetModel ( $strat_2_ );
my $param_1_ = GetParam ( $strat_1_ );
my $param_2_ = GetParam ( $strat_2_ );
open (M1, "<", $model_1_ ) or PrintStacktraceAndDie ( "Could not open $model_1_ for reading" );
my @model_1_lines_ = <M1>; chomp ( @model_1_lines_ );
close(M1);
open (M2, "<", $model_2_ ) or PrintStacktraceAndDie ( "Could not open $model_2_ for reading" );
my @model_2_lines_ = <M2>; chomp ( @model_2_lines_ );
close(M2);
my $type_ = GetStrattype( $strat_1_ );
for ( my $i=0; $i<=$#regimes_; $i++ )
{
my $strat_1_2_ = $STRATS_DIR."/strat_".$idx_1_."_".$idx_2_."_".$i;
my $strat_2_1_ = $STRATS_DIR."/strat_".$idx_2_."_".$idx_1_."_".$i;
my $model_1_2_ = $MODELS_DIR."/model_".$idx_1_."_".$idx_2_."_".$i;
my $model_2_1_ = $MODELS_DIR."/model_".$idx_2_."_".$idx_1_."_".$i;
my $param_1_2_ = $PARAMS_DIR."/param_".$idx_1_."_".$idx_2_."_".$i;
my $param_2_1_ = $PARAMS_DIR."/param_".$idx_2_."_".$idx_1_."_".$i;
print FILE basename($strat_1_2_)." ".basename($strat_1_)." ".basename($strat_2_)."\n";
print FILE basename($strat_2_1_)." ".basename($strat_2_)." ".basename($strat_1_)."\n";
open ( STRAT, ">" , $strat_1_2_ ) or PrintStacktraceAndDie ( "Could not open $strat_1_2_ for writing" );
print STRAT "STRATEGYLINE $shc_ $type_ $model_1_2_ $param_1_2_ $start_time_ $end_time_ 1111\n";
close(STRAT);
open ( STRAT, ">" , $strat_2_1_ ) or PrintStacktraceAndDie ( "Could not open $strat_2_1_ for writing" );
print STRAT "STRATEGYLINE $shc_ $type_ $model_2_1_ $param_2_1_ $start_time_ $end_time_ 1111\n";
close(STRAT);

open ( MODEL, ">" , $model_1_2_ ) or PrintStacktraceAndDie ( "Could not open $model_1_2_ for writing" );
for ( my $line = 0; $line <=$#model_1_lines_-1; $line++ )
{
if ( $line == 2 )
{
print MODEL "REGIMEINDICATOR 1.00 $regimes_[$i]\n";
}
if ( $line == 1 )
{
print MODEL "MODELMATH SELECTIVENEW CHANGE\n";
next;
}
print MODEL "$model_1_lines_[ $line ]\n";
}
print MODEL "INDICATORINTERMEDIATE\n";
for ( my $line = 3; $line <=$#model_2_lines_-1; $line++ )
{
print MODEL "$model_2_lines_[ $line ]\n";
}
print MODEL "INDICATOREND\n";
close(MODEL);


open ( MODEL, ">" , $model_2_1_ ) or PrintStacktraceAndDie ( "Could not open $model_2_1_ for writing" );
for ( my $line = 0; $line <=$#model_2_lines_-1; $line++ )
{
if ( $line == 2 )
{
print MODEL "REGIMEINDICATOR 1.00 $regimes_[$i]\n";
}
if ( $line == 1 )
{
print MODEL "MODELMATH SELECTIVENEW CHANGE\n";
next;
}
print MODEL "$model_2_lines_[ $line ]\n";
}
print MODEL "INDICATORINTERMEDIATE\n";
for ( my $line = 3; $line <=$#model_1_lines_-1; $line++ )
{
print MODEL "$model_1_lines_[ $line ]\n";
}
print MODEL "INDICATOREND\n";
close(MODEL);


open ( PARAM, ">" , $param_1_2_ ) or PrintStacktraceAndDie ( "Could not open $param_1_2_ for writing" );
print PARAM "PARAMFILELIST $param_1_\n";
print PARAM "PARAMFILELIST $param_2_\n";
print PARAM "INDICATOR 1.00 $regimes_[$i]\n";
close(PARAM);

open ( PARAM, ">" , $param_2_1_ ) or PrintStacktraceAndDie ( "Could not open $param_2_1_ for writing" );
print PARAM "PARAMFILELIST $param_2_\n";
print PARAM "PARAMFILELIST $param_1_\n";
print PARAM "INDICATOR 1.00 $regimes_[$i]\n";
close(PARAM);
}
close(FILE);
}
