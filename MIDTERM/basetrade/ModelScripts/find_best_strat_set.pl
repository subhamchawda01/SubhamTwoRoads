#!/usr/bin/perl

# \file ModelScripts/find_best_strat_set.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
  $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate

require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

require "$GENPERLLIB_DIR/global_results_methods.pl"; # GetGlobalResultsForShortcodeDate , GetStratNameFromGlobalResultsLine

require "$GENPERLLIB_DIR/array_ops.pl"; # GetMeanHighestQuartile

use constant DEBUG => 0;

sub ComputeStratComboResults;

my $USAGE="$0 SHORTCODE NUM_STRATS_TO_PICK DAYS_TO_LOOK_BEHIND [ OPTIMIZATION_MEASURE = PD ]\n\tOPTIMIZATION_MEASURE = \n\t\t P : Avg_Pnl\n\t\t V : Avg_Vol\n\t\tPV : Pnl_to_volume\n\t\tPD : Avg_Pnl_to_drawdown";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];
my $num_strats_to_pick_ = $ARGV [ 1 ];

my $num_past_days_ = $ARGV [ 2 ];

my $optimization_measure_ = "PD";
if ( $#ARGV >= 3 ) { $optimization_measure_ = $ARGV [ 3 ]; }

my $end_yyyymmdd_ = `date +%Y%m%d`; chomp ( $end_yyyymmdd_ );

$end_yyyymmdd_ = CalcPrevWorkingDateMult ( $end_yyyymmdd_ , 1 );

while ( ! ValidDate ( $end_yyyymmdd_ ) || 
       SkipWeirdDate ( $end_yyyymmdd_ ) || 
       IsDateHoliday ( $end_yyyymmdd_ ) )
{
  $end_yyyymmdd_ = CalcPrevWorkingDateMult ( $end_yyyymmdd_ , 1 );
}

# Build a map of date_to_strat_results_.
my %date_to_strat_results_ = ( );
my @all_strat_names_ = ( );

my $t_yyyymmdd_ = $end_yyyymmdd_;
for ( my $t_days_ = $num_past_days_ ; $t_days_ ; $t_days_ -- )
{
  if ( ! ValidDate ( $t_yyyymmdd_ ) ||
      SkipWeirdDate ( $t_yyyymmdd_ ) || 
      IsDateHoliday ( $t_yyyymmdd_ ) )
  {
    $t_yyyymmdd_ = CalcPrevWorkingDateMult ( $t_yyyymmdd_ , 1 );
    $t_days_ ++;
    next;
  }

  my @t_results_lines_ = ();
  GetGlobalResultsForShortcodeDate ( $shortcode_ , $t_yyyymmdd_ , \@t_results_lines_);

  for ( my $i = 0 ; $i <= $#t_results_lines_ ; $i ++ )
  {
    my $t_strat_name_ = GetStratNameFromGlobalResultVecRef ( $t_results_lines_ [ $i ] );

    $date_to_strat_results_ { $t_yyyymmdd_ } { $t_strat_name_ } = join ( " ", @ { $t_results_lines_ [ $i ] } ) ;

    if ( FindItemFromVec ( $t_strat_name_ , @all_strat_names_ ) ne $t_strat_name_ )
    {
      push ( @all_strat_names_ , $t_strat_name_ );
    }
  }

  $t_yyyymmdd_ = CalcPrevWorkingDateMult ( $t_yyyymmdd_ , 1 );
}

if ( DEBUG ) 
{
  print "All STRATS : \n";
  for ( my $i = 0 ; $i <= $#all_strat_names_ ; $i ++ )
  {
    print $all_strat_names_ [ $i ]."\n";
  }
  print "\n";
}

# Some strats are not present across all days ,
# place invalid results for those strats , so they don't
# get picked.
foreach my $t_yyyymmdd_
( keys %date_to_strat_results_ )
{
  for ( my $i = 0 ; $i <= $#all_strat_names_ ; $i ++ )
  {
    if ( ! exists ( $date_to_strat_results_ { $t_yyyymmdd_ } { $all_strat_names_ [ $i ] } ) )
    {
      $date_to_strat_results_ { $t_yyyymmdd_ } { $all_strat_names_ [ $i ] } = InvalidGlobalResult ( );
    }
  }
}

my @picked_strats_ = ( );
my %picked_strat_to_avg_pnl_ = ( );
my %picked_strat_to_avg_vol_ = ( );
my %picked_strat_to_avg_dd_ = ( );
my %picked_strat_to_avg_pnl_to_dd_ = ( );
my %picked_strat_to_avg_pnl_to_vol_ = ( );

while ( 1 )
{
  if ( $#picked_strats_ >= ( $num_strats_to_pick_ - 1 ) )
  {
    last;
  }

  my %date_to_stratcombo_results_ = ( );

  foreach my $t_yyyymmdd_
      ( keys %date_to_strat_results_ )
      {
        foreach my $t_strat_name_
            ( keys % { $date_to_strat_results_ { $t_yyyymmdd_ } } )
            {
              if ( FindItemFromVec ( $t_strat_name_ , @picked_strats_ ) eq $t_strat_name_ )
              { # This strat has already been picked. Invalidate , so it is not picked again.
                if ( DEBUG ) { print "Already picked $t_strat_name_\n"; }

                $date_to_stratcombo_results_ { $t_yyyymmdd_ } { $t_strat_name_ } = InvalidGlobalResult ( );
              }
              else
              {
                if ( DEBUG ) { print "Assessing $t_strat_name_\n"; }

                $date_to_stratcombo_results_ { $t_yyyymmdd_ } { $t_strat_name_ } = ComputeStratComboResults ( $t_yyyymmdd_ , $t_strat_name_ , @picked_strats_ , %date_to_strat_results_ );
              }
            }
      }

# Find pnl , DD , vol avg. across all dates.
  my %stratcombo_to_average_pnl_ = ( );
  my %stratcombo_to_average_dd_ = ( );
  my %stratcombo_to_average_vol_ = ( );
  my %stratcombo_to_average_pnl_to_vol_ = ( );
  my %stratcombo_to_average_pnl_to_dd_ = ( );

  my %stratcombo_to_count_ = ( );

# Go through the stratcombo results and pick the one with
# ( highest pnl / mean_quart_dd )
  foreach my $t_yyyymmdd_
      ( keys %date_to_stratcombo_results_ )
      {
        foreach my $t_strat_name_
            ( keys % { $date_to_stratcombo_results_ { $t_yyyymmdd_ } } )
            {
              if ( $date_to_stratcombo_results_ { $t_yyyymmdd_ } { $t_strat_name_ } ne InvalidGlobalResult ( ) )
              {
                $stratcombo_to_average_pnl_ { $t_strat_name_ } += GetPnlFromGlobalResultsLine ( $date_to_stratcombo_results_ { $t_yyyymmdd_ } { $t_strat_name_ } );
                $stratcombo_to_average_vol_ { $t_strat_name_ } += GetVolFromGlobalResultsLine ( $date_to_stratcombo_results_ { $t_yyyymmdd_ } { $t_strat_name_ } );

                my $t_max_dd_ = GetMaxDDFromGlobalResultsLine ( $date_to_stratcombo_results_ { $t_yyyymmdd_ } { $t_strat_name_ } );

                push ( @ { $stratcombo_to_average_dd_ { $t_strat_name_ } } , $t_max_dd_ );

                $stratcombo_to_count_ { $t_strat_name_ } ++;
              }
              else
              { # This stratcombo is already being used , assign a very low score.
                $stratcombo_to_average_pnl_ { $t_strat_name_ } = -100000;
                $stratcombo_to_average_vol_ { $t_strat_name_ } = -100000;
                push ( @ { $stratcombo_to_average_dd_ { $t_strat_name_ } } , 100000 );

                $stratcombo_to_count_ { $t_strat_name_ } = 1;
              }
            }
      }

  foreach my $t_strat_name_
      ( keys %stratcombo_to_average_pnl_ )
      {
        $stratcombo_to_average_pnl_to_vol_ { $t_strat_name_ } = $stratcombo_to_average_pnl_ { $t_strat_name_ } / abs ( $stratcombo_to_average_vol_ { $t_strat_name_ } );

        $stratcombo_to_average_pnl_ { $t_strat_name_ } /= $stratcombo_to_count_ { $t_strat_name_ };
        $stratcombo_to_average_vol_ { $t_strat_name_ } /= $stratcombo_to_count_ { $t_strat_name_ };

        my @t_dd_vec_ = $stratcombo_to_average_dd_ { $t_strat_name_ };
        $stratcombo_to_average_dd_ { $t_strat_name_ } = GetMeanHighestQuartile ( @t_dd_vec_ );

        $stratcombo_to_average_pnl_to_dd_ { $t_strat_name_ } = $stratcombo_to_average_pnl_ { $t_strat_name_ } / $stratcombo_to_average_dd_ { $t_strat_name_ };

        if ( DEBUG )
        {
          print "\nstratcombo_to_avg_pnl : $t_strat_name_ $stratcombo_to_average_pnl_{$t_strat_name_}\n";
          print "\nstratcombo_to_avg_vol : $t_strat_name_ $stratcombo_to_average_vol_{$t_strat_name_}\n";
          print "\nstratcombo_to_avg_dd : $t_strat_name_ $stratcombo_to_average_dd_{$t_strat_name_}\n";
          print "\nstratcombo_to_avg_pnl_to_dd : $t_strat_name_ $stratcombo_to_average_pnl_to_dd_{$t_strat_name_}\n";
          print "\nstratcombo_to_avg_pnl_to_vol : $t_strat_name_ $stratcombo_to_average_pnl_to_vol_{$t_strat_name_}\n";
        }
      }

# Based on optimization criteria , pick the best combo.
  my $t_result_hash_ref_ = \%stratcombo_to_average_pnl_to_dd_;

  if ( $optimization_measure_ eq "P" )
  { # Pick by avg. pnl.
    $t_result_hash_ref_ = \%stratcombo_to_average_pnl_;
  }
  elsif ( $optimization_measure_ eq "V" )
  { # Pick by avg. vol.
    $t_result_hash_ref_ = \%stratcombo_to_average_vol_;
  }
  elsif ( $optimization_measure_ eq "PV" )
  { # Pick by avg. pnl_to_vol
    $t_result_hash_ref_ = \%stratcombo_to_average_pnl_to_vol_;
  }
  elsif ( $optimization_measure_ eq "PD" )
  { # Pick by avg. pnl_to_dd
    $t_result_hash_ref_ = \%stratcombo_to_average_pnl_to_dd_;
  }

  foreach my $t_strat_name_ ( sort { $ { $t_result_hash_ref_ } { $b } <=> $ { $t_result_hash_ref_ } { $a } }
                             keys % { $t_result_hash_ref_ } )
  {
    if ( $t_strat_name_ ne InvalidGlobalResult ( ) )
    {
      if ( DEBUG ) { print "\n\t :: PICKED : $t_strat_name_\n"; }
      push ( @picked_strats_ , $t_strat_name_ );

      $picked_strat_to_avg_pnl_ { $t_strat_name_ } = $stratcombo_to_average_pnl_ { $t_strat_name_ };
      $picked_strat_to_avg_vol_ { $t_strat_name_ } = $stratcombo_to_average_vol_ { $t_strat_name_ };
      $picked_strat_to_avg_dd_ { $t_strat_name_ } = $stratcombo_to_average_dd_ { $t_strat_name_ };
      $picked_strat_to_avg_pnl_to_dd_ { $t_strat_name_ } = $stratcombo_to_average_pnl_to_dd_ { $t_strat_name_ };
      $picked_strat_to_avg_pnl_to_vol_ { $t_strat_name_ } = $stratcombo_to_average_pnl_to_vol_ { $t_strat_name_ };

      last;
    }
  }
}

printf "%-160s %10s %10s %10s %6s %5s\n\n" , "STRATEGY NAME" , "AVG. PNL" , "AVG. VOL" , "MAX DD" , "PNLDD" , "PNLVOL";
for ( my $i = 0 ; $i <= $#picked_strats_ ; $i ++ )
{
  printf "%-160s %10.3f %10.3f %10.3f %6.3f %5.3f\n" ,  $picked_strats_ [ $i ] , $picked_strat_to_avg_pnl_ { $picked_strats_ [ $i ] } , $picked_strat_to_avg_vol_ { $picked_strats_ [ $i ] } , $picked_strat_to_avg_dd_ { $picked_strats_ [ $i ] } , $picked_strat_to_avg_pnl_to_dd_ { $picked_strats_ [ $i ] } , $picked_strat_to_avg_pnl_to_vol_ { $picked_strats_ [ $i ] };
}

exit ( 0 );

sub ComputeStratComboResults
{
  my ( $t_yyyymmdd_ , $t_strat_name_ ) = @_;

  if ( DEBUG ) { print "\n => ComputeStratComboResults\n"; }

  my $stratcombo_results_ = "";

  my @t_picked_strats_results_ = ( );

  for ( my $i = 0 ; $i <= $#picked_strats_ ; $i ++ )
  {
    if ( DEBUG ) { print "Putting : ".$date_to_strat_results_ { $t_yyyymmdd_ } { $picked_strats_ [ $i ] }." in list to be assessed\n"; }

    push ( @t_picked_strats_results_ , $date_to_strat_results_ { $t_yyyymmdd_ } { $picked_strats_ [ $i ] } );
  }

# Also add results from the strat under consideration.
  push ( @t_picked_strats_results_ , $date_to_strat_results_ { $t_yyyymmdd_ } { $t_strat_name_ } );

  if ( DEBUG ) { print "Putting : ".$date_to_strat_results_ { $t_yyyymmdd_ } { $t_strat_name_ }." in list to be assessed\n"; }

  $stratcombo_results_ = CombineGlobalResultsLineFromVec ( @t_picked_strats_results_ );

  if ( DEBUG ) { print "Combined result : $stratcombo_results_\n"; }

  return $stratcombo_results_;
}
