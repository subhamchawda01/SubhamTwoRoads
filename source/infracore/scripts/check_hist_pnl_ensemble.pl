#!/usr/bin/perl

# \file scripts/check_hist_pnl_ensemble.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
#
# shortcode
# TIME_DURATION = [ EU_MORN_DAY | US_MORN | US_DAY | US_MORN_DAY | EU_MORN_DAY_US_DAY ]
# num_days

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="infracore";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $GLOBALRESULTSDBDIR=$HOME_DIR."/globalresults"; # Changed for DVCTrader ... so that it does not clash with others

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage GetStdev
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1 # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD

# start 
my $USAGE="$0 timeperiod num_days";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }
my $timeperiod_ = $ARGV[0];
my $num_days_ = min ( 90, int ( $ARGV[1] ) );

my %shortcode2numqueries_ ;
my %shortcode2numdayspast_ ;

$shortcode2numqueries_{"FGBS_0"}=6; $shortcode2numdayspast_{"FGBS_0"}=3; # best was 1 but very close to 3
$shortcode2numqueries_{"FGBM_0"}=3; $shortcode2numdayspast_{"FGBM_0"}=1; # second best is 5 days
$shortcode2numqueries_{"FGBL_0"}=1; $shortcode2numdayspast_{"FGBL_0"}=7; # second best is 1
$shortcode2numqueries_{"FESX_0"}=6; $shortcode2numdayspast_{"FESX_0"}=1; # second best is 14
#$shortcode2numqueries_{"FDAX_0"}=0; $shortcode2numdayspast_{"FDAX_0"}=14;

$shortcode2numqueries_{"ZF_0"}=6; $shortcode2numdayspast_{"ZF_0"}=5; # second best was 2 days
$shortcode2numqueries_{"ZN_0"}=1; $shortcode2numdayspast_{"ZN_0"}=2; # second best was 3 days
$shortcode2numqueries_{"ZB_0"}=1; $shortcode2numdayspast_{"ZB_0"}=2;
#$shortcode2numqueries_{"UB_0"}=1; $shortcode2numdayspast_{"UB_0"}=1; # second best was 3

$shortcode2numqueries_{"CGB_0"}=1; $shortcode2numdayspast_{"CGB_0"}=1; # second best is 2-3 ... 10 is also good
#$shortcode2numqueries_{"SXF_0"}=0; $shortcode2numdayspast_{"SXF_0"}=14;
$shortcode2numqueries_{"BAX_0"}=0; $shortcode2numdayspast_{"BAX_0"}=5; # second best is 2-3 ... 10 is also good
$shortcode2numqueries_{"BAX_1"}=1; $shortcode2numdayspast_{"BAX_1"}=5; # second best is 2-3 ... 10 is also good
$shortcode2numqueries_{"BAX_2"}=1; $shortcode2numdayspast_{"BAX_2"}=5; # second best is 2-3 ... 10 is also good
$shortcode2numqueries_{"BAX_3"}=1; $shortcode2numdayspast_{"BAX_3"}=5; # second best is 2-3 ... 10 is also good
$shortcode2numqueries_{"BAX_4"}=1; $shortcode2numdayspast_{"BAX_4"}=5; # second best is 2-3 ... 10 is also good
$shortcode2numqueries_{"BAX_5"}=0; $shortcode2numdayspast_{"BAX_5"}=5; # second best is 2-3 ... 10 is also good
$shortcode2numqueries_{"BAX_6"}=0; $shortcode2numdayspast_{"BAX_6"}=5; # second best is 2-3 ... 10 is also good


my %shortcode2pnlvec_ ;
my @totalpnlvec_ = ();
my @tradingdates_ = ();

my $tradingdate_ = GetIsoDateFromStrMin1 ( "TODAY-1" );
for ( my $t_day_index_ = 0 ; $t_day_index_ < $num_days_ ; $t_day_index_ ++ ) 
{
    if ( ( ! ValidDate ( $tradingdate_ ) ) ||
	 SkipWeirdDate ( $tradingdate_ ) ||
	 IsDateHoliday ( $tradingdate_ ) )
    {
	$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
	next;
    }
#    print "$tradingdate_\n";
    my ($tradingdate_YYYY_, $tradingdate_MM_, $tradingdate_DD_) = BreakDateYYYYMMDD ( $tradingdate_ );

    my $prev_trading_date_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
    my $skip_counter_ = 0;
    while ( ( ! ValidDate ( $prev_trading_date_ ) ) ||
	    SkipWeirdDate ( $prev_trading_date_ ) ||
	    IsDateHoliday ( $prev_trading_date_ ) )
    {
	$prev_trading_date_ = CalcPrevWorkingDateMult ( $prev_trading_date_, 1 );
	$skip_counter_ ++;
	if ( $skip_counter_ > 30 )
	{
	    last; # error case 
	}
    }
    
    my $this_day_total_pnl_ = 0;
    my %shortcode2pnl_;
    foreach my $shortcode_ ( keys %shortcode2numqueries_ )
    {
	if ( exists $shortcode2numqueries_{$shortcode_} )
	{
	    my $numqueries_ = $shortcode2numqueries_{$shortcode_};
	    my $num_days_past_ = $shortcode2numdayspast_{$shortcode_};
	    if ( $numqueries_ > 0 )
	    {
		my $this_day_shortcode_pnl_ = 0;
		my $result_file_ = $GLOBALRESULTSDBDIR."/".$shortcode_."/".$tradingdate_YYYY_."/".$tradingdate_MM_."/".$tradingdate_DD_."/results_database.txt" ;
#		print "#$shortcode_\n";
		my $exec_cmd="$MODELSCRIPTS_DIR/summarize_strats_for_day.pl $shortcode_ $timeperiod_ $prev_trading_date_ $num_days_past_ | grep -v MAXDD | awk '{ if ( NF > 15 ) { printf \"\%d \%d \%.2f \%s\\n\", \$3, \$5, (\$3 / \$16), \$2 } }' | sort -rg -k3 | head -n$numqueries_ | awk '{print \$4}'";
#		print $exec_cmd."\n";
		my @selected_strats_ = `$exec_cmd`;
#		print @selected_strats_ ;
		chomp @selected_strats_;
		foreach my $sel_strat_ ( @selected_strats_ )
		{
#		      print "SEARCHING FOR $sel_strat_ in $result_file_\n";
		    my $pnl_ = 0;
		    if ( -e $result_file_ )
		    {
			$pnl_ = `grep $sel_strat_ $result_file_ | awk '{print \$3}' | head -n1`; chomp ( $pnl_ );
		    }
		    if ( ! $pnl_ ) { $pnl_ = 0; }
#		    print "$sel_strat_ $pnl_\n";
		    $this_day_total_pnl_ += $pnl_;
		    $this_day_shortcode_pnl_ += $pnl_;
		}
		$shortcode2pnl_{$shortcode_} = $this_day_shortcode_pnl_;
#		  print "$shortcode_ $this_day_shortcode_pnl_\n";

		if ( exists $shortcode2pnlvec_ { $shortcode_ } ) 
		{ # seen shortcode for a different date already
		    my $scalar_ref_ = $shortcode2pnlvec_ { $shortcode_ } ;
		    push ( @$scalar_ref_, $this_day_shortcode_pnl_ );
		} 
		else 
		{ # seeing this shortcode for the first time
		    my @this_pnl_vec_ = ();
		    push ( @this_pnl_vec_, $this_day_shortcode_pnl_ );
		    $shortcode2pnlvec_ { $shortcode_ } = [ @this_pnl_vec_ ] ;
		}

	    }
	}
    }
    push ( @totalpnlvec_, $this_day_total_pnl_ );
    push ( @tradingdates_, $tradingdate_ );
    $tradingdate_ = $prev_trading_date_ ;
}

for ( my $i = 0 ; $i <= $#totalpnlvec_ ; $i ++ )
{
    my $this_day_total_pnl_ = $totalpnlvec_[$i];
    $tradingdate_ = $tradingdates_ [$i];
    printf "TOTAL %d %6d   ",$tradingdate_, $this_day_total_pnl_;
    foreach my $shortcode_ ( keys %shortcode2pnlvec_ )
    {
	my $scalar_ref_pnl_vec_ = $shortcode2pnlvec_{$shortcode_};
	my $this_day_shortcode_pnl_ = $$scalar_ref_pnl_vec_[$i];
	printf " %s %6d", $shortcode_, $this_day_shortcode_pnl_;
    }
    print "\n";
}

#printf "TOTAL %6d %6d %6d", GetAverage ( \@totalpnlvec_ ), GetStdev ( \@totalpnlvec_ ), GetMeanDeviationFromTrend ( \@totalpnlvec_ );
printf "TOTAL %6d %6d %6d\n", GetAverage ( \@totalpnlvec_ ), GetStdev ( \@totalpnlvec_ ), GetL2NormLosses ( \@totalpnlvec_ );
#printf "TOTAL %6d %6d\n", GetAverage ( \@totalpnlvec_ ), GetStdev ( \@totalpnlvec_ );
foreach my $shortcode_ ( keys %shortcode2pnlvec_ )
{
    my $scalar_ref_pnl_vec_ = $shortcode2pnlvec_{$shortcode_};
#    printf " [%s %6d %6d %6d]\n", $shortcode_, GetAverage ( $scalar_ref_pnl_vec_ ), GetStdev ( $scalar_ref_pnl_vec_ ), GetMeanDeviationFromTrend ( $scalar_ref_pnl_vec_ );
    printf " [%s %6d %6d %6d]\n", $shortcode_, GetAverage ( $scalar_ref_pnl_vec_ ), GetStdev ( $scalar_ref_pnl_vec_ ), GetL2NormLosses ( $scalar_ref_pnl_vec_ );
#    printf "%s %6d %6d\n", $shortcode_, GetAverage ( $scalar_ref_pnl_vec_ ), GetStdev ( $scalar_ref_pnl_vec_ );
}
#printf "\n";
