#!/usr/bin/perl

# \file scripts/analyse_installed_queries.pl
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
use FileHandle;
use List::Util qw/max min/; # for max
use Math::Complex ; # sqrt
my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

my $SEE_STATS_OF_RUNNING_QUERIES = $SCRIPTS_DIR."/see_stats_of_running_queries.sh";
my $SS_NOC = $SCRIPTS_DIR."/ss_noc.sh";
my $SS = $SCRIPTS_DIR."/ss.sh";
my $SORT_MULTCOL_SQRT = $SCRIPTS_DIR."/sort_multcol_sqrt.pl";
my $SORT_MULTCOL = $SCRIPTS_DIR."/sort_multcol.pl";
my $SORT_PNL_VOLOPT = $SCRIPTS_DIR."/sort_pnl_volopt.pl";

my $USE_COLORS_ = 1;

require "$GENPERLLIB_DIR/get_trading_location_for_shortcode.pl"; # GetTradingLocationForShortcode
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/get_iso_date_from_str.pl";
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl";

sub GetStratNameFromResultLine;

my $USAGE="$0 SHORTCODE FOLDER [S/N for staged or normal] [SORT_ALGO=PNL_SHARPE_AVG]";
if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];

my $time_period_ = $ARGV [ 1 ];

my $sort_algo_ = "kCNAPnlSharpeAverage";

my $staged_tag_ = "";

if ( $#ARGV > 1 )
{
    if ( $ARGV[2] eq "S" )
    {
      $staged_tag_ = "staged_";      
    }
}

if ( $#ARGV > 2 )
{
    $sort_algo_ = $ARGV [ 3 ];
}

my $strat_dir_ = "/home/dvctrader/modelling/".$staged_tag_."strats/".$shortcode_."/".$time_period_;

my @comparison_durations_ = ( 0 , 2 , 4, 20, 120, 240 );

my $num_queries_to_show_=39;

my %duration_to_queries_ = ( );
my %strat_name_to_occurence_ = ( );
 
foreach my $duration_ ( @comparison_durations_ )
{
    if ( $duration_ > 1 )
    {
      print "=======> $duration_ days\n";
    }
    else
    {
      print "=======> $duration_ day\n";
    }

    print "POSSIBLE QUERIES\n";    
    print "SSNR  SSNPNL  SSNVOL SSNP2DD  SSNGPR  SSNPML     TTC    STRAT_NAME\n";

    # Get possibly better alternatives.
    my $end_date_ = GetIsoDateFromStr ( "TODAY-1");
    my $start_date_ = CalcPrevWorkingDateMult ($end_date_, $duration_ );  
    my $exec_cmd_ = "~/basetrade_install/bin/summarize_strategy_results $shortcode_ $strat_dir_ DB $start_date_ $end_date_ INVALIDFILE $sort_algo_ 0 INVALIDFILE 0 | grep STRAT | awk '{print \$2}'";     
    my @ss_strats_ = `$exec_cmd_`;
    $exec_cmd_ = "~/basetrade_install/bin/summarize_strategy_results $shortcode_ $strat_dir_ DB $start_date_ $end_date_ INVALIDFILE $sort_algo_ 0 INVALIDFILE 0 | grep STAT";   
    my @ss_stats_ = `$exec_cmd_`; 

    for ( my $j=0; $j <= min($num_queries_to_show_,$#ss_strats_); $j++ )
    {      
      my $result_line_ = $ss_stats_[$j];
      my $t_strat_ = $ss_strats_[$j];
      chomp($t_strat_);
      chomp($result_line_);
      my @result_words_ = split(' ', $result_line_);
      my $t_ss_noc_rank_ = $j+1;
      my $t_ss_noc_pnl_ = $result_words_[1];
      my $t_ss_noc_vol_ = $result_words_[3];
      my $t_ss_noc_ttc_ = $result_words_[8];
      my $t_ss_noc_sharpe_ = $result_words_[4];
      my $t_ss_noc_ppc_ = $result_words_[9];
      my $t_ss_noc_avg_dd_ = $result_words_[14];
      my $t_ss_noc_gain_to_pain_ratio_ = $result_words_[20];
      my $t_ss_noc_pnl_by_maxloss_ = $result_words_[21];
      my $t_ss_noc_pnl_by_dd_ = $t_ss_noc_pnl_ / $t_ss_noc_avg_dd_;

      my $t_print_line_ = sprintf "%4d %7d %7d %7.2f %7.2f %7.2f %7d   $t_strat_\n",
         $t_ss_noc_rank_,
         $t_ss_noc_pnl_,
         $t_ss_noc_vol_,
         $t_ss_noc_pnl_by_dd_,
         $t_ss_noc_gain_to_pain_ratio_,
         $t_ss_noc_pnl_by_maxloss_,
         $t_ss_noc_ttc_;
      print $t_print_line_;

      if ( FindItemFromVec ( @ { $duration_to_queries_ { $duration_ } } , $t_print_line_ ) ne $t_print_line_ )
      {
        push ( @ { $duration_to_queries_ { $duration_ } } , $t_print_line_ );
      }
    }
}

my %is_common_query_ = ( );
my %is_top_query_ = ( );

foreach my $duration_ ( sort { $a <=> $b }
        keys %duration_to_queries_ )
{
  foreach my $result_line_ ( @ { $duration_to_queries_ { $duration_ } } )
  {
    my $t_strat_name_ = GetStratNameFromResultLine2($result_line_);
    chomp($t_strat_name_);                    
    $strat_name_to_occurence_ { $t_strat_name_ } ++;
  }
}

print "COMMON QUERIES\n";
print "DAY RANK     PNL     VOL     P2DD     GPR     PML     TTC   STRAT_NAME\n";

foreach my $strat_name_ ( sort { $strat_name_to_occurence_ { $b } <=> $strat_name_to_occurence_ { $a } }
                          keys %strat_name_to_occurence_ )
{
  if ( $strat_name_to_occurence_{$strat_name_} > 1 )
  {
    foreach my $duration_ ( sort { $a <=> $b }
                            keys %duration_to_queries_ )
    {
      foreach my $result_line_ ( @ { $duration_to_queries_ { $duration_ } } )
      {
        my $t_strat_name_ = GetStratNameFromResultLine2($result_line_);
        chomp($t_strat_name_);

        if ( $t_strat_name_ eq $strat_name_ )
        {
          printf "%4d $result_line_", $duration_;            
        }
      }
    }
    print "\n\n";
  }
}

sub GetStratNameFromResultLine2
{
  my $result_line_ = shift;
  my @result_words_ = split ( ' ' , $result_line_ );
  return ($result_words_[$#result_words_]);
}
