#!/usr/bin/perl

# \file ModelScripts/parallel_params_permute.pl
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
# SHORTCODE
# TIMEPERIOD
# BASEPX
# PARAMFILE_WITH_PERMUTATIONS
# TRADING_START_YYYYMMDD
# TRADING_END_YYYYMMDD

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use Fcntl qw (:flock);
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

package ResultLine;
use Class::Struct;


# declare the struct
struct ( 'ResultLine', { pnl_ => '$', volume_ => '$', ttc_ => '$' } );

package main;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/";

my $REPO="basetrade";

my $SCRIPTS_BASE_DIR=$HOME_DIR."/".$REPO."/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR=$HOME_DIR."/basetrade_install/bin";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl";
my $USAGE="$0 temp_strategy_catted_file_ temp_strategy_list_file_ unique_sim_id_ tradingdate_ results_base_dir";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }
my $temp_strategy_catted_file_ = $ARGV [ 0 ];
my $temp_strategy_list_file_ = $ARGV [ 1 ];
my $unique_sim_id_ = $ARGV [ 2 ];
my $tradingdate_ = $ARGV [ 3 ];
my $results_base_dir = $ARGV [ 4 ];

my $exec_cmd_ = $LIVE_BIN_DIR."/sim_strategy_options SIM ".$temp_strategy_catted_file_." ".$unique_sim_id_." ".$tradingdate_." ADD_DBG_CODE -1 2>/dev/null" ;
# print "$exec_cmd_ \n";
my @sim_strategy_options_lines_ = `$exec_cmd_`;

my $this_trades_filename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".$unique_sim_id_;
my $this_log_filename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".$unique_sim_id_;
my $this_pnl_stats_exec_ = "$MODELSCRIPTS_DIR/get_pnl_stats_options.pl $this_trades_filename_";

my %prod_to_unique_id_pnlstats_map_ = (); 
if ( ExistsWithSize ( $this_trades_filename_ ) )
{
# All
	my @pnl_stats_all_ = `$MODELSCRIPTS_DIR/get_pnl_stats_options.pl $this_trades_filename_ A`;
	my @rwords_ = split ( ' ', $pnl_stats_all_[0] );
	if ( $#rwords_ >= 1 )
	{
		my $unique_sim_id_ = $rwords_[0];
		my $prod_name_ = $rwords_[1];
		splice ( @rwords_, 0, 2 );

		$prod_to_unique_id_pnlstats_map_{$prod_name_}{$unique_sim_id_} = join (' ', @rwords_);
	}

# Underlying
	my @pnl_stats_ul_ = `$MODELSCRIPTS_DIR/get_pnl_stats_options.pl $this_trades_filename_ U`;
	if ( $#rwords_ >= 1 )
	{
		my $unique_sim_id_ = $rwords_[0];
		my $prod_name_ = $rwords_[1];
		splice ( @rwords_, 0, 2 );
		$prod_to_unique_id_pnlstats_map_{$prod_name_}{$unique_sim_id_} = join(' ', @rwords_);
	}
}

# Product
my @pnl_stats_i_ = `$MODELSCRIPTS_DIR/get_pnl_stats_options.pl $this_trades_filename_ I`;
# print @pnl_stats_i_;
for ( my $t_pnlstats_output_lines_index_ = 0; $t_pnlstats_output_lines_index_ <= $#pnl_stats_i_; $t_pnlstats_output_lines_index_++ )
{
	my @rwords_ = split ( ' ', $pnl_stats_i_[$t_pnlstats_output_lines_index_] );
	if ( $#rwords_ >= 1 )
	{
		my $unique_sim_id_ = $rwords_[0];
		my $prod_name_ = $rwords_[1];
		splice ( @rwords_, 0, 2 );
		$prod_to_unique_id_pnlstats_map_{$prod_name_}{$unique_sim_id_} = join ( ' ', @rwords_ );
	}
}
#print "\n".$unique_sim_id_." ".$unique_gsm_id_."\n";
#`rm -f $this_trades_filename_; rm -f $this_log_filename_`;

# now dump the results in local_results_dir
# we will extend the strat_name by appending shortcode
# strategy names are appended with prod_ ( ALL/NSE_NIFTY_P0_O2/BANKNIFTY ) corresponding to
# complete strategy results or per option or per underlying
# we will dump them under same local_results_dir
# we should ideally have same number of pnl_stats line as sim_stratey_option output lines

# unique_gsm_id_ == unique_sim_id_

for ( my $i = 0; $i <= $#sim_strategy_options_lines_; $i++ )
{
# ALL == SIMRESULT
# ALL
	my @rwords_ = split ( ' ', $sim_strategy_options_lines_[$i] );
	my $prod_ = $rwords_[0];
	if ( $prod_ eq "SIMRESULT" )
	{
		$prod_ = "ALL";
	}
	splice ( @rwords_, 0, 1 ); # prod
		splice ( @rwords_, -1 ); # min_pnl


		my $remaining_simresult_line_ = join ( ' ' , @rwords_ );
	my $stats_ = "0 0 0 0 0 0 0 0 0 0 0 0 0" ;
	if ( exists ( $prod_to_unique_id_pnlstats_map_{$prod_}{$unique_sim_id_} ) )
	{
		$stats_ = $prod_to_unique_id_pnlstats_map_{$prod_}{$unique_sim_id_};
	}
	my $result_line_ = $remaining_simresult_line_." ".$stats_;

	my $base_strat_name_ = basename `cat $temp_strategy_list_file_`;
	chomp ( $base_strat_name_ );
	$base_strat_name_ = $base_strat_name_."_".$prod_;

	my ($tradingdateyyyy_, $tradingdatemm_, $tradingdatedd_) = BreakDateYYYYMMDD ( $tradingdate_ );
	my $resultsfilename_ = $results_base_dir."/".$tradingdateyyyy_."/".$tradingdatemm_."/".$tradingdatedd_."/results_database.txt";

	my $resultsfilepathdir_ = dirname ( $resultsfilename_ ); chomp ( $resultsfilepathdir_ );
	if ( ! ( -d $resultsfilepathdir_ ) )
	{
		`mkdir -p $resultsfilepathdir_`;
	}
# write results to the result file
	open ( RESULTSDBFILE, ">> $resultsfilename_" );
	flock ( RESULTSDBFILE, LOCK_EX ); # write lock
		print RESULTSDBFILE $base_strat_name_, " ", $tradingdate_, " ", $result_line_, "\n" ;
	close ( RESULTSDBFILE );
}
