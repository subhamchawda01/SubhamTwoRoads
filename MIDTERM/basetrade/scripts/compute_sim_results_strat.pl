#!/usr/bin/perl

# \file scripts/score_indicators.pl
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
my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'}; 

my $REPO = "basetrade";

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

use File::Basename;
use Data::Dumper;

my $work_dir_ = "/spare/local/ankit/";
#sub declarations
my @initial_population_to_score_ = ();
my @initial_population_to_pnl2_score_ = ();
my @initial_population_to_med_ttc_score_ = ( );
my @initial_population_to_mean_ttc_score_ = ( );
my @initial_population_to_volume_score_ = ();
my @initial_population_to_best_lvl_score_ = ();
my @initial_population_to_supporting_score_ = ();
my @initial_population_to_aggress_score_ = ();
sub GetAverageSimPnlForPopulation;

my $USAGE="$0 SHC NUM_STRATS STRAT_FILE START_DATE NUM_PREV_DAYS ";
if ( $#ARGV < 4){ print $USAGE."\n"; exit ( 0 ); }

my $base_shortcode_ = $ARGV[0];
my $strat_file_ = $ARGV [ 2 ];
my $tmp_strat_file_ = "/spare/local/$USER/tmp_strat";
`cp $strat_file_ $tmp_strat_file_`;
#`sed -i 's/ BRT_1700/ BRT_1100/g' $tmp_strat_file_`;
my $start_date_ = $ARGV [ 3 ];
my $num_days_ = $ARGV [ 4 ];
my $exec_cmd_ = "wc -l $strat_file_";
my $population_size_ = $ARGV[1];
print "population size : ".$population_size_."\n";
$work_dir_ = $work_dir_."/check_sim/";
`mkdir -p $work_dir_ `; #make_work_dir
GetAverageSimPnlForPopulation($tmp_strat_file_);

my $sum_ = 0;

for(my $i=0;$i<$population_size_;$i++)
{
	$sum_ += $initial_population_to_score_[$i];
	my $std_dev_ = int(sqrt($initial_population_to_pnl2_score_[$i] - $initial_population_to_score_[$i]*$initial_population_to_score_[$i]));
	print $strat_file_."\nSTATISTICS   Pnl: ".int($initial_population_to_score_[$i])."\tStd_Dev: ".$std_dev_."\t Med_TTC: ".int($initial_population_to_med_ttc_score_[$i])."\tMean_TTC: ".int($initial_population_to_mean_ttc_score_[$i])."\tVol: ".int($initial_population_to_volume_score_[$i])."\tSupporting% : ".int($initial_population_to_supporting_score_[$i])."\tBest Level% : ".int($initial_population_to_best_lvl_score_[$i])."\tAggressive% : ".int($initial_population_to_aggress_score_[$i])."\n";
}

#print $sum_/$population_size_;print "\n";

exit ( 0 );

sub GetAverageSimPnlForPopulation( )
{
	
	my ( $t_strat_filename_ ) = @_ ;
	my $num_strats_ = $population_size_;
	my $tradingdate_ = $start_date_;
	my @t_sim_strategy_output_lines_ = ( );
	my @tmpsampledates = ( );
	my $rem_days_ = $num_days_;
	while( $rem_days_ > 0)
	{
		my @sample_dates =( );
		my $num_strats_running = 0;
		for ( my $t_day_index_ = 0; $t_day_index_ < (10 < $rem_days_ ? 10 : $rem_days_); $t_day_index_ ++ )
		{
			if ( SkipWeirdDate ( $tradingdate_ ) ||
		        ( NoDataDateForShortcode ( $tradingdate_ , $base_shortcode_ ) ) || 
		    	( IsDateHoliday ( $tradingdate_ ) || ( ( $base_shortcode_ ) && ( IsProductHoliday ( $tradingdate_, $base_shortcode_ ) ) ) ) )
			{
				$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
				$t_day_index_ --;
				next;
			}
		
			my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $t_strat_filename_ 97869 $tradingdate_ ADD_DBG_CODE 1 -1 | grep SIMRESULT";
			my $log_=$work_dir_."sim_res_".$tradingdate_;
			push (@sample_dates, $tradingdate_);
			push (@tmpsampledates , $tradingdate_ );
			#print "$exec_cmd_ > $log_ & \n";
			`$exec_cmd_ > $log_ & `; #run in background to parallelize
			$num_strats_running+=1;
			$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
		}
		
		my $done_cnt_ = 0;
		while ( $done_cnt_ != $num_strats_running )
		{
			$done_cnt_ = 0;
			foreach $tradingdate_ ( @sample_dates )
			{
	        	        my $log_=$work_dir_."sim_res_".$tradingdate_;
				$done_cnt_ += `tail -n 1 $log_ 2>/dev/null | grep SIMRESULT -c `;
			}	
			select(undef, undef, undef, 1.0); #sleep for 1 secs
		}
		$rem_days_ -= 10;
	}

	my @sorted_sim_results_ = ( );
	
	for (my $j=0;$j<$num_strats_;$j++)
	{
		push(@initial_population_to_score_, 0);
		push(@initial_population_to_pnl2_score_, 0);
		push(@initial_population_to_med_ttc_score_, 0 );
		push(@initial_population_to_mean_ttc_score_, 0);
		push(@initial_population_to_volume_score_, 0);
		push(@initial_population_to_aggress_score_, 0);
		push(@initial_population_to_best_lvl_score_, 0 );
		push(@initial_population_to_supporting_score_,0);
	}
	foreach $tradingdate_ ( @tmpsampledates )
	{
		open(FILE, $work_dir_."sim_res_".$tradingdate_) or die("Unable to open file");
		# read file into an array
                @t_sim_strategy_output_lines_ = <FILE>;
		my @pnl_stats_lines_ = `$MODELSCRIPTS_DIR/get_pnl_stats_2.pl /spare/local/logs/tradelogs/trades.$tradingdate_.97869`;
		close(FILE);
		my $t_pnl_text_ = "";
		for (my $j = 0; $j <$num_strats_ ; $j ++ )
		{
			my @t_sim_rwords_ = split ( ' ', $t_sim_strategy_output_lines_[$j] );
			my @t_pnl_stats_words_ = split( ' ', $pnl_stats_lines_[$j]);
			$initial_population_to_score_[$j] += $t_sim_rwords_[1]/$num_days_;
			$initial_population_to_pnl2_score_[$j] += $t_sim_rwords_[1]*$t_sim_rwords_[1]/$num_days_;
			$initial_population_to_med_ttc_score_[$j] += $t_pnl_stats_words_[2]/$num_days_;
			$initial_population_to_mean_ttc_score_[$j] += $t_pnl_stats_words_[3]/$num_days_;
			$initial_population_to_volume_score_[$j] += $t_sim_rwords_[2]/$num_days_;
			$initial_population_to_aggress_score_[$j] += $t_sim_rwords_[5]/$num_days_;
			$initial_population_to_best_lvl_score_[$j] += $t_sim_rwords_[4]/$num_days_;
			$initial_population_to_supporting_score_[$j] += $t_sim_rwords_[3]/$num_days_;
		}
	}
	return;
}

