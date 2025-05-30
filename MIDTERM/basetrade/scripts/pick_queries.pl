#!/usr/bin/perl

# \file scripts/pick_queries.pl
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

require "$GENPERLLIB_DIR/get_trading_location_for_shortcode.pl"; # GetTradingLocationForShortcode
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

sub GetStratNameFromResultLine;

my $USAGE="$0 SHORTCODE TIMEPERIOD MINVOLUME COMPARISONDURATION_1 COMPARISONDURATION_2 ...";
if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];
my $time_period_ = $ARGV [ 1 ];

my $min_volume_ = $ARGV [ 2 ];

my $today_yyyymmdd_ = `date +%Y%m%d`; chomp ( $today_yyyymmdd_ );
my $trading_location_ = GetTradingLocationForShortcode ( $shortcode_ , $today_yyyymmdd_ );

my @comparison_durations_ = ( );

for ( my $i = 3 ; $i <= $#ARGV ; $i ++ )
{
    push ( @comparison_durations_ , $ARGV [ $i ] );
}

# Maintain a list of all possible queries.
# Later cross-ref among them / find top 2 from each / etc.
my %duration_to_queries_ = ( );
my %strat_name_to_occurence_ = ( );

# Get a list of running queries.
# num_past_days not important , just need a list of running strats
my @running_queries_ = `$SEE_STATS_OF_RUNNING_QUERIES $trading_location_ $shortcode_ 10 $time_period_`;

for ( my $t_rq_ = 0 ; $t_rq_ <= $#running_queries_ ; $t_rq_ ++ )
{
    $running_queries_ [ $t_rq_ ] = GetStratNameFromResultLine ( $running_queries_ [ $t_rq_ ] );
}

my $num_queries_to_pick_ = max ( 24, ( $#running_queries_ * 2 ) );

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $num_queries_to_pick_ = max ( 24 , ( $#running_queries_ * 3 ) );
}

foreach my $duration_ ( @comparison_durations_ )
{
    # Get possibly better alternatives.
    my @ss_stats_ = `$SS $shortcode_ $time_period_ $duration_ | awk '{ if ( \$2 >= $min_volume_ ) { print \$0; } }' | $SORT_MULTCOL_SQRT 1 2`;
    my @ss_noc_stats_ = `$SS_NOC $shortcode_ $time_period_ $duration_ | awk '{ if ( \$2 >= $min_volume_ ) { print \$0; } }' | $SORT_MULTCOL_SQRT 1 2`;

    my @best_ss_queries_ = ( );

    for ( my $i = 0 ; $i <= $num_queries_to_pick_ ; $i ++ )
    {
	if ( $i <= $#ss_noc_stats_ )
	{
	    push ( @best_ss_queries_ , $ss_noc_stats_ [ $i ] );
	}
    }

    for ( my $ss_query_ = 0 ; $ss_query_ <= min ( 15, $#best_ss_queries_ ) ; $ss_query_ ++ )
    {
	my $t_ss_query_ = GetStratNameFromResultLine ( $best_ss_queries_ [ $ss_query_ ] );

	my $t_ss_rank_ = ( $ss_query_ + 1 ); 

	my $t_ss_pnl_ = GetPnlFromResultLine ( $best_ss_queries_ [ $ss_query_ ] );
	my $t_ss_vol_ = GetVolFromResultLine ( $best_ss_queries_ [ $ss_query_ ] );
	my $t_ss_pnl_to_drawdown_ = GetPnlToDrawdownFromResultLine ( $best_ss_queries_ [ $ss_query_ ] );

	my $t_ss_noc_rank_ = -1; my $t_ss_noc_pnl_ = 0; my $t_ss_noc_vol_ = 0; my $t_ss_noc_pnl_to_drawdown_ = 0;

	my $t_print_line_ = 
	    sprintf "%3d %7d %7d %6.2f %s\n" , 
	    $t_ss_rank_ ,
	    $t_ss_pnl_ ,
	    $t_ss_vol_ ,
	    $t_ss_pnl_to_drawdown_ ,
	    $t_ss_query_;

	if ( FindItemFromVec ( @ { $duration_to_queries_ { $duration_ } } , $t_print_line_ ) ne $t_print_line_ ) 
	{
	    push ( @ { $duration_to_queries_ { $duration_ } } , $t_print_line_ );
	}
    }
}

my %is_common_query_ = ( );
my %is_top_query_ = ( );

# Summarize possible queries.
{
    my $print_common_queries_ = 1;
    # Find common good queries if any.
    foreach my $duration_ ( sort { $a <=> $b }
			    keys %duration_to_queries_ )
    {
	foreach my $result_line_ ( @ { $duration_to_queries_ { $duration_ } } )
	{
	    my $strat_name_ = GetStratNameFromResultLine ( $result_line_ );
	    $strat_name_to_occurence_ { $strat_name_ } ++;
	}
    }

    if ( $print_common_queries_ )
    {
	print " DAY RANK     PNL     VOL   P2DD STRAT_NAME\n";
    }

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
		    my $t_strat_name_ = GetStratNameFromResultLine ( $result_line_ );

		    if ( $strat_name_ eq $t_strat_name_ )
		    {
			if ( $print_common_queries_ )
			{
			    printf "%4d  $result_line_", $duration_;
			}
			$is_common_query_ { $strat_name_ } = 1;
		    }
		}
	    }
	    if ( $print_common_queries_ )
	    {
		print "\n";
	    }
	}
    }
}

exit ( 0 );

sub GetStratNameFromResultLine
{
    # 22029 16505 2.65 w_strategy_ilist_BR_DOL_0_US_Mkt_Mkt_J0.noeu_8_na_e3_20111206_20120123_EST_800_EST_1400_500_0_0_fsr.5_3_FSHLR_0.01_0_0_0.7.tt_EST_700_EST_1400.pfi_3

    my $result_line_ = shift;

    my @result_words_ = split ( ' ' , $result_line_ );

    if ( $#result_words_ >= 3 )
    {
	return $result_words_ [ $#result_words_ ];
    }
    else
    {
	return "";
    }
}

sub GetPnlFromResultLine
{
    # 22029 16505 2.65 w_strategy_ilist_BR_DOL_0_US_Mkt_Mkt_J0.noeu_8_na_e3_20111206_20120123_EST_800_EST_1400_500_0_0_fsr.5_3_FSHLR_0.01_0_0_0.7.tt_EST_700_EST_1400.pfi_3

    my $result_line_ = shift;

    my @result_words_ = split ( ' ' , $result_line_ );

    if ( $#result_words_ >= 0 )
    {
	return $result_words_ [ 0 ];
    }
    else
    {
	return 0;
    }
}

sub GetVolFromResultLine
{
    # 22029 16505 2.65 w_strategy_ilist_BR_DOL_0_US_Mkt_Mkt_J0.noeu_8_na_e3_20111206_20120123_EST_800_EST_1400_500_0_0_fsr.5_3_FSHLR_0.01_0_0_0.7.tt_EST_700_EST_1400.pfi_3

    my $result_line_ = shift;

    my @result_words_ = split ( ' ' , $result_line_ );

    if ( $#result_words_ >= 1 )
    {
	return $result_words_ [ 1 ];
    }
    else
    {
	return 0;
    }
}

sub GetPnlToDrawdownFromResultLine
{
    # 22029 16505 2.65 w_strategy_ilist_BR_DOL_0_US_Mkt_Mkt_J0.noeu_8_na_e3_20111206_20120123_EST_800_EST_1400_500_0_0_fsr.5_3_FSHLR_0.01_0_0_0.7.tt_EST_700_EST_1400.pfi_3

    my $result_line_ = shift;

    my @result_words_ = split ( ' ' , $result_line_ );

    if ( $#result_words_ >= 2 )
    {
	return $result_words_ [ 2 ];
    }
    else
    {
	return 0;
    }
}
