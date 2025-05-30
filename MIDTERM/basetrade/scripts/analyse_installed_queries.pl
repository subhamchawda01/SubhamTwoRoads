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

my $GET_INSTALLED_QUERIES = $SCRIPTS_DIR."/get_installed_queries.sh";
my $SS_NOC = $SCRIPTS_DIR."/ss_noc.sh";
my $SS = $SCRIPTS_DIR."/ss.sh";
my $SORT_MULTCOL_SQRT = $SCRIPTS_DIR."/sort_multcol_sqrt.pl";
my $SORT_MULTCOL = $SCRIPTS_DIR."/sort_multcol.pl";
my $SORT_PNL_VOLOPT = $SCRIPTS_DIR."/sort_pnl_volopt.pl";

my $USE_COLORS_ = 1;

require "$GENPERLLIB_DIR/get_trading_location_for_shortcode.pl"; # GetTradingLocationForShortcode
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

sub GetStratNameFromResultLine;

my $USAGE="$0 SHORTCODE TIMEPERIOD [USER] [MIN_VOLUME=0] [MAX_TTC=inf] [STRAT_FILE_NAME=w_strategy_ilist_BR_DOL_0_US_Mkt_Mkt_J0.noeu_8_na_e3_20111206_20120123_EST_800_EST_1400_500_0_0_fsr.5_3_FSHLR_0.01_0_0_0.7.tt_EST_700_EST_1400.pfi_3]";
if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];

my $time_period_ = $ARGV [ 1 ];
if ( $time_period_ eq "US" )
{ $time_period_ = "US_MORN_DAY"; }
if ( $time_period_ eq "EU" )
{ $time_period_ = "EU_MORN_DAY"; }
if ( $time_period_ eq "EUS" )
{ $time_period_ = "EU_MORN_DAY_US_DAY"; }

my $user_ = "dvctrader";
my $min_volume_ = 0;
my $max_ttc_ = 9999999999;
my $strat_file_name_ = " ";

if ( $#ARGV > 1 )
{
    $user_ = $ARGV [ 2 ];
}
if ( $#ARGV > 2 )
{
    $min_volume_ = $ARGV [ 3 ];
}
if ( $#ARGV > 3 )
{
    $max_ttc_ = $ARGV [ 4 ];
}
if ( $#ARGV > 4 )
{
    $strat_file_name_ = $ARGV [ 5 ];
}

my $today_yyyymmdd_ = `date +%Y%m%d`; chomp ( $today_yyyymmdd_ );
my $trading_location_ = GetTradingLocationForShortcode ( $shortcode_ , $today_yyyymmdd_ );

#my @comparison_durations_ = ( 4 , 24 , 120, 210 );
my @comparison_durations_ = ( 0, 4, 20, 120, 240 );

if ( $user_ eq "kputta" )
{
    @comparison_durations_ = ( 0, 5, 10, 20, 60, 90, 180, 250 );
}

# Maintain a list of all possible queries.
# Later cross-ref among them / find top 2 from each / etc.
my %duration_to_queries_ = ( );
my %strat_name_to_occurence_ = ( );

use Term::ANSIColor; 

# Get a list of running queries.
# num_past_days not important , just need a list of running strats
my @running_queries_ = split(" ", `$GET_INSTALLED_QUERIES $today_yyyymmdd_ $shortcode_`);

for ( my $i = 1 ; $i < 10 ; $i++) 
{
    $today_yyyymmdd_ = `date -d '$i day ago' +%Y%m%d`; chomp ( $today_yyyymmdd_ );
    @running_queries_ = split(" ", `$GET_INSTALLED_QUERIES $today_yyyymmdd_ $shortcode_`);
    if ( scalar(@running_queries_) > 1 ) {
	last ;
    }
}

#for ( my $t_rq_ = 0 ; $t_rq_ <= $#running_queries_ ; $t_rq_ ++ )
#{
#    $running_queries_ [ $t_rq_ ] = GetStratNameFromResultLine ( $running_queries_ [ $t_rq_ ] );
#}

my $DEFAULT_MAX_QUERIES_TO_SHOW=24;
my $num_queries_to_pick_ = max ( $DEFAULT_MAX_QUERIES_TO_SHOW, ( $#running_queries_ * 2 ) );

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $num_queries_to_pick_ = max ( $DEFAULT_MAX_QUERIES_TO_SHOW, ( $#running_queries_ * 3 ) );
}

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

    print "INSTALLED QUERIES\n";    
    print "SSNR  SSNPNL  SSNVOL SSNP2DD TTC STRAT_NAME\n";

    # Get possibly better alternatives.
    my @ss_stats_ = `$SS $shortcode_ $time_period_ $duration_ TODAY-1 1 | awk '{ if ( \$2 >= $min_volume_ && \$7 <= $max_ttc_ ) { print \$0; } }' | $SORT_MULTCOL_SQRT 1 2`;
    my @ss_noc_stats_ = `$SS_NOC $shortcode_ $time_period_ $duration_ TODAY-1 1 | awk '{ if ( \$2 >= $min_volume_ && \$7 <= $max_ttc_ ) { print \$0; } }' | $SORT_MULTCOL_SQRT 1 2`;

    # (i) See if the currently running queries are part of historically "good" queries.
    for ( my $i = 0 ; $i <= $#running_queries_ ; $i ++ )
    {
	my $t_running_strat_name_ = $running_queries_ [ $i ];

	# Default values indicating queries not found at all.
	my $t_ss_noc_rank_ = -1; my $t_ss_noc_pnl_ = 0; my $t_ss_noc_vol_ = 0; my $t_ss_noc_pnl_to_drawdown_ = 0; my $t_ss_noc_ttc_ = 0;
	my $should_print_ = 0;

	for ( my $j = 0 ; $j <= $#ss_noc_stats_ ; $j ++ )
	{
	    my $t_ss_noc_strat_name_ = GetStratNameFromResultLine ( $ss_noc_stats_ [ $j ] );
	    if ( $t_ss_noc_strat_name_ eq $t_running_strat_name_ )
	    {
		$should_print_ = 1;
	    }
	    if ( $t_ss_noc_strat_name_ eq $t_running_strat_name_ )
	    {
		$t_ss_noc_rank_ = ( $j + 1 );
		$t_ss_noc_pnl_ = GetPnlFromResultLine ( $ss_noc_stats_ [ $j ] );
		$t_ss_noc_vol_ = GetVolFromResultLine ( $ss_noc_stats_ [ $j ] );
		$t_ss_noc_ttc_ = GetTTCFromResultLine ( $ss_noc_stats_ [ $j ] );
		$t_ss_noc_pnl_to_drawdown_ = GetPnlToDrawdownFromResultLine ( $ss_noc_stats_ [ $j ] );
		last;
	    }
	}

	if ( $should_print_ == 1 )
	{
	    my $t_print_line_ =
		sprintf "%4d %7d %7d %7.2f %8.2f $t_running_strat_name_\n" , 
		$t_ss_noc_rank_ , 
		$t_ss_noc_pnl_ ,
		$t_ss_noc_vol_ , 
		$t_ss_noc_pnl_to_drawdown_ ,
		$t_ss_noc_ttc_;

	    print $t_print_line_;
	}
    }

    print "POSSIBLE QUERIES\n";    
    print "SSR   SSPNL   SSVOL SSP2DD    TTC      STRAT_NAME\n";

    my @best_ss_queries_ = ( );

    for ( my $i = 0 ; $i <= $num_queries_to_pick_ ; $i ++ )
    {
#	if ( $i <= $#ss_stats_ )
#	{
#	    push ( @best_ss_queries_ , $ss_stats_ [ $i ] );
#	}
	if ( $i <= $#ss_noc_stats_ ) # changed to ss_noc -- gchak
	{
	    push ( @best_ss_queries_ , $ss_noc_stats_ [ $i ] );
	}
    }

    # Print out picks from ss_.
    for ( my $ss_query_ = 0 ; $ss_query_ <= min ( 15, $#best_ss_queries_ ) ; $ss_query_ ++ ) # added min 15 -- gchak
    {
	my $t_ss_query_ = GetStratNameFromResultLine ( $best_ss_queries_ [ $ss_query_ ] );

	my $t_ss_rank_ = ( $ss_query_ + 1 ); 

	my $t_ss_pnl_ = GetPnlFromResultLine ( $best_ss_queries_ [ $ss_query_ ] );
	my $t_ss_vol_ = GetVolFromResultLine ( $best_ss_queries_ [ $ss_query_ ] );
	my $t_ss_ttc_ = GetTTCFromResultLine ( $best_ss_queries_ [ $ss_query_ ] );
	my $t_ss_pnl_to_drawdown_ = GetPnlToDrawdownFromResultLine ( $best_ss_queries_ [ $ss_query_ ] );

	my $t_ss_noc_rank_ = -1; my $t_ss_noc_pnl_ = 0; my $t_ss_noc_vol_ = 0; my $t_ss_noc_pnl_to_drawdown_ = 0; my $t_ss_noc_ttc_ = 0;

	if ( FindItemFromVec ( $t_ss_query_ , @running_queries_ ) eq $t_ss_query_ )
	{
	    if ( $USE_COLORS_ == 1 )
	    {
		print color ( "cyan" );
	    }
	}

	my $t_print_line_ = 
	    sprintf "%3d %7d %7d %6.2f %8.2f %s\n" , 
	    $t_ss_rank_ ,
	    $t_ss_pnl_ ,
	    $t_ss_vol_ ,
	    $t_ss_pnl_to_drawdown_ ,
	    $t_ss_ttc_ ,
	    $t_ss_query_;

	if ( FindItemFromVec ( @ { $duration_to_queries_ { $duration_ } } , $t_print_line_ ) ne $t_print_line_ ) 
	{
	    print $t_print_line_;
	    push ( @ { $duration_to_queries_ { $duration_ } } , $t_print_line_ );
	}

	if ( $USE_COLORS_ == 1 )
	{
	    print color ( "reset" );
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
	    my $strat_name_ = GetStratNameFromResultLine2 ( $result_line_ );
	    $strat_name_to_occurence_ { $strat_name_ } ++;
	}
    }

    if ( $print_common_queries_ )
    {
     print "COMMON QUERIES\n";
     print "DAY RANK     PNL     VOL   P2DD    TTC      STRAT_NAME\n";
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
		    my $t_strat_name_ = GetStratNameFromResultLine2 ( $result_line_ );

		    if ( $strat_name_ eq $t_strat_name_ )
		    {
			if ( $print_common_queries_ )
			{
			    if ( FindItemFromVec ( $t_strat_name_ , @running_queries_ ) eq $t_strat_name_ ) 
			    {
				if ( $USE_COLORS_ == 1 )
				{
				    print color ( "cyan" );
				}
			    }
			    
			    printf "%4d  $result_line_", $duration_;
			}
			$is_common_query_ { $strat_name_ } = 1;
			
			if ( $print_common_queries_ )
			{
			    if ( $USE_COLORS_ == 1 )
			    {
				print color ( "reset" );
			    }
			}
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

exit ( 0 ); # added --gchak

# Find top queries across each run.
{
    print "TOP QUERIES\n";
    print "DAYS OCCR RANK     PNL     VOL   P2DD    TTC      STRAT_NAME\n";

    my $num_queries_per_days_ = $num_queries_to_pick_ / $#comparison_durations_;

    foreach my $duration_ ( sort { $a <=> $b } 
			    keys %duration_to_queries_ )
    {
	my $t_lines_ = 0;
	
	foreach my $result_line_ ( @ { $duration_to_queries_ { $duration_ } } )
	{
	    my $t_strat_name_ = GetStratNameFromResultLine2 ( $result_line_ );
	    
	    if ( exists $is_top_query_ { $t_strat_name_ } )
	    {
		next;
	    }

	    $is_top_query_ { $t_strat_name_ } = 1;

	    if ( FindItemFromVec ( $t_strat_name_ , @running_queries_ ) eq $t_strat_name_ ) 
	    {
		if ( exists $is_common_query_ { $t_strat_name_ } )
		{
		    if ( $USE_COLORS_ == 1 )
		    {
			print color ( "yellow" );
		    }
		}
		else
		{
		    if ( $USE_COLORS_ == 1 )
		    {
			print color ( "cyan" );
		    }
		}
	    }
	    else
	    {
		if ( exists $is_common_query_ { $t_strat_name_ } )
		{
		    if ( $USE_COLORS_ == 1 )
		    {
			print color ( "red" );
		    }
		}
	    }

	    printf "%4d %4d  $result_line_" , $duration_, $strat_name_to_occurence_ { $t_strat_name_ };

	    if ( $USE_COLORS_ == 1 )
	    {
		print color ( "reset" );
	    }

	    $t_lines_ ++;

	    if ( $t_lines_ >= $num_queries_per_days_ )
	    {
		last;
	    }
	}
    }
}

{ # Find queries that should potentially be replaced.
    print "BAD QUERIES\n";
    print "DAYS RANK     PNL     VOL   P2DD    TTC      STRAT_NAME\n";

    my @bad_queries_ = ( );

    # First ones to go are (i) not ranked higher & (ii) not common.
    for ( my $i = 0 ; $i <= $#running_queries_ ; $i ++ )
    {
	my $t_strat_name_ = $running_queries_ [ $i ];

	if ( exists $is_common_query_ { $t_strat_name_ } || exists $is_top_query_ { $t_strat_name_ } )
	{
	    next;
	}

	push ( @bad_queries_ , $t_strat_name_ );
    }

    # Second, the ones which are not top performing, but might still be common.
    for ( my $i = 0 ; $i <= $#running_queries_ ; $i ++ )
    {
	my $t_strat_name_ = $running_queries_ [ $i ];

	if ( exists $is_top_query_ { $t_strat_name_ } )
	{
	    next;
	}

	if ( exists $is_common_query_ { $t_strat_name_ } )
	{
	    push ( @bad_queries_ , $t_strat_name_ );
	}
    }

    for ( my $i = 0 ; $i <= $#bad_queries_ ; $i ++ )
    {
	my $is_print_line_ = 0;

	foreach my $duration_ ( sort { $a <=> $b } 
				keys %duration_to_queries_ )
	{
	    foreach my $result_line_ ( @ { $duration_to_queries_ { $duration_ } } )
	    {
		my $t_strat_name_ = GetStratNameFromResultLine2 ( $result_line_ );

		if ( $t_strat_name_ eq $bad_queries_ [ $i ] )
		{
		    if ( exists $is_common_query_ { $t_strat_name_ } )
		    {
			if ( $USE_COLORS_ == 1 )
			{
			    print color ( "yellow" );
			}
		    }
		    else
		    {
			if ( $USE_COLORS_ == 1 )
			{
			    print color ( "cyan" );
			}
		    }

		    printf "%4d  $result_line_", $duration_;

		    if ( $USE_COLORS_ == 1 )
		    {
			print color ( "reset" );
		    }

		    $is_print_line_ = 1;
		}
	    }
	}

	if ( $is_print_line_ == 1 ) { print "\n"; }
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
	return $result_words_ [ 3 ];
    }
    else
    {
	return "";
    }
}

sub GetStratNameFromResultLine2
{
    # 22029 16505 2.65 w_strategy_ilist_BR_DOL_0_US_Mkt_Mkt_J0.noeu_8_na_e3_20111206_20120123_EST_800_EST_1400_500_0_0_fsr.5_3_FSHLR_0.01_0_0_0.7.tt_EST_700_EST_1400.pfi_3

    my $result_line_ = shift;

    my @result_words_ = split ( ' ' , $result_line_ );

    if ( $#result_words_ >= 5 )
    {
	return $result_words_ [ 5 ];
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

sub GetTTCFromResultLine
{
    # 22029 16505 2.65 w_strategy_ilist_BR_DOL_0_US_Mkt_Mkt_J0.noeu_8_na_e3_20111206_20120123_EST_800_EST_1400_500_0_0_fsr.5_3_FSHLR_0.01_0_0_0.7.tt_EST_700_EST_1400.pfi_3

    my $result_line_ = shift;

    my @result_words_ = split ( ' ' , $result_line_ );

    if ( $#result_words_ >= 6 )
    {
	return $result_words_ [ 6 ];
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
