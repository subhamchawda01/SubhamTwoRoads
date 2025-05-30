#!/usr/bin/perl

# \file ModelScripts/run_datagen_and_generate_reg_data.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use Math::Complex; # sqrt
use FileHandle;
use POSIX;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib/";

require "$GENPERLLIB_DIR/search_exec.pl"; # SearchExec
require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllOutputFilesPopulated , AllPIDSTerminated

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";

my $DATALOG_DIR = "/spare/local/logs/datalogs/";

my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( );

# start
my $USAGE="$0 shortcode ilist_filename_ tradingdate_list_filename_ start_time_ end_time_ datagen_msecs_ datagen_l1events_ datagen_trades_ to_print_on_eco_ timed_to_reg_pred_duration_ timed_to_reg_pred_algo_ regdata_filename_ work_dir_ [use_fake_faster_data_(0/1) = 1] [sampling_shortcode_str_]";

if ( $#ARGV < 12 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];
my $ilist_filename_ = $ARGV [ 1 ];
my $tradingdate_list_filename_ = $ARGV [ 2 ];
my @tradingdate_list_ = `cat $tradingdate_list_filename_`; chomp ( @tradingdate_list_ );
my $start_time_ = $ARGV [ 3 ];
my $end_time_ = $ARGV [ 4 ];
my $datagen_msecs_ = $ARGV [ 5 ];
my $datagen_l1events_ = $ARGV [ 6 ];
my $datagen_trades_ = $ARGV [ 7 ];
my $to_print_on_eco_ = $ARGV [ 8 ];
my $timed_to_reg_pred_duration_ = $ARGV [ 9 ];
my $timed_to_reg_pred_algo_ = $ARGV [ 10 ];
my $regdata_filename_ = $ARGV [ 11 ];
my $work_dir_ = $ARGV [ 12 ];

my $use_fake_faster_data_ = 1;
if( $#ARGV >= 13)
{    
  $use_fake_faster_data_ = $ARGV[13];
}

my $sampling_shortcode_str_ = "";
if ( $#ARGV >= 14 )
{
    $sampling_shortcode_str_ = $ARGV [ 14 ];
}



my @unique_sim_id_list_ = ( );
my @independent_parallel_commands_ = ( );
my @output_timed_file_list_ = ( );

my @intermediate_files_ = ( );

for ( my $t_index_ = 0; $t_index_ <= $#tradingdate_list_ ; $t_index_ ++ )
{
    my $tradingdate_ = $tradingdate_list_ [ $t_index_ ];
    $tradingdate_ =~ s/^\s+|\s+$//g;

    my $unique_sim_id_ = GetGlobalUniqueId ( );
    push ( @unique_sim_id_list_ , $unique_sim_id_ );

    my $output_timed_file_ = $work_dir_."timed_output_".$tradingdate_."_".basename ( $ilist_filename_ );
    `> $output_timed_file_`;
    push ( @output_timed_file_list_ , $output_timed_file_ );

    my $datagen_exec = SearchExec ("datagen");
    my $exec_cmd_ = "$datagen_exec $ilist_filename_ $tradingdate_ $start_time_ $end_time_ $unique_sim_id_ $output_timed_file_ $datagen_msecs_ $datagen_l1events_ $datagen_trades_ $to_print_on_eco_ USE_FAKE_FASTER_DATA $use_fake_faster_data_ $sampling_shortcode_str_ ADD_DBG_CODE -1 >/dev/null 2>&1";
    push ( @independent_parallel_commands_ , $exec_cmd_ );

    my $log_filename_ = $DATALOG_DIR."log.".$tradingdate_.".".$unique_sim_id_;

    push ( @intermediate_files_ , $log_filename_ );
    push ( @intermediate_files_ , $output_timed_file_ );
}  

for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; )
{
    my @pids_to_poll_this_run_ = ( );

    my $THIS_MAX_CORES_TO_USE_IN_PARALLEL = TemperCoreUsageOnLoad ( $MAX_CORES_TO_USE_IN_PARALLEL );
    for ( my $num_parallel_ = 1 ; $num_parallel_ <= $THIS_MAX_CORES_TO_USE_IN_PARALLEL && $command_index_ <= $#independent_parallel_commands_ ; $num_parallel_ ++ )
    {
	my $exec_cmd_ = $independent_parallel_commands_ [ $command_index_ ];

	$exec_cmd_ = $exec_cmd_." & echo \$!";
	my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

	if ( $#exec_cmd_output_ >= 0 )
	{
	    my @exec_cmd_output_words_ = split ( ' ' , $exec_cmd_output_ [ 0 ] );
	    if ( $#exec_cmd_output_words_ >= 0 )
	    {
		my $t_pid_ = $exec_cmd_output_words_ [ 0 ];
		$t_pid_ =~ s/^\s+|\s+$//g;

		push ( @pids_to_poll_this_run_ , $t_pid_ );
	    }
	}

	$command_index_ ++;

	sleep ( 1 );
    }	

    while ( ! AllPIDSTerminated ( @pids_to_poll_this_run_ ) )
    { # there are still some datagen which haven't completed
	sleep ( 1 );
    }
}

@unique_sim_id_list_ = ( );
@independent_parallel_commands_ = ( );
my @output_reg_file_list_ = ( );

for ( my $t_index_ = 0; $t_index_ <= $#tradingdate_list_ ; $t_index_ ++ )
{
    my $tradingdate_ = $tradingdate_list_ [ $t_index_ ];
    $tradingdate_ =~ s/^\s+|\s+$//g;

    my $input_timed_file_ = $work_dir_."timed_output_".$tradingdate_."_".basename ( $ilist_filename_ );
    my $output_reg_file_ = $work_dir_."reg_output_".$tradingdate_."_".basename ( $ilist_filename_ );

    push ( @output_reg_file_list_ , $output_reg_file_ );

    my $timed_to_reg_pred_counter_ = 0;
    my $print_pred_counters_for_this_pred_algo_script = $MODELSCRIPTS_DIR."/print_pred_counters_for_this_pred_algo.pl" ; # replace this SearchScript in future
    my $exec_cmd_ = "$print_pred_counters_for_this_pred_algo_script $shortcode_ $timed_to_reg_pred_duration_ $timed_to_reg_pred_algo_ $input_timed_file_ $start_time_ $end_time_";
#    print $exec_cmd_."\n";
    my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );
    if ( $#exec_cmd_output_ >= 0 )
    {
	$timed_to_reg_pred_counter_ = $exec_cmd_output_ [ 0 ];
    }

    my $timed_data_to_reg_data_exec = SearchExec ( "timed_data_to_reg_data" );
    $exec_cmd_ = "$timed_data_to_reg_data_exec $ilist_filename_ $input_timed_file_ $timed_to_reg_pred_counter_ $timed_to_reg_pred_algo_ $output_reg_file_ >/dev/null 2>&1";
    push ( @independent_parallel_commands_ , $exec_cmd_ );

    push ( @intermediate_files_ , $input_timed_file_ );
    push ( @intermediate_files_ , $output_reg_file_ );
}  

for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; )
{
    my @pids_to_poll_this_run_ = ( );

    my $THIS_MAX_CORES_TO_USE_IN_PARALLEL = TemperCoreUsageOnLoad ( $MAX_CORES_TO_USE_IN_PARALLEL );
    for ( my $num_parallel_ = 1 ; $num_parallel_ <= $THIS_MAX_CORES_TO_USE_IN_PARALLEL && $command_index_ <= $#independent_parallel_commands_ ; $num_parallel_ ++ )
    {
	my $exec_cmd_ = $independent_parallel_commands_ [ $command_index_ ];

	$exec_cmd_ = $exec_cmd_." & echo \$!";
	my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

#	print $exec_cmd_."\n";

	if ( $#exec_cmd_output_ >= 0 )
	{
	    my @exec_cmd_output_words_ = split ( ' ' , $exec_cmd_output_ [ 0 ] );
	    if ( $#exec_cmd_output_words_ >= 0 )
	    {
		my $t_pid_ = $exec_cmd_output_words_ [ 0 ];
		$t_pid_ =~ s/^\s+|\s+$//g;

		push ( @pids_to_poll_this_run_ , $t_pid_ );
	    }
	}

	$command_index_ ++;

	sleep ( 1 );
    }	

    while ( ! AllPIDSTerminated ( @pids_to_poll_this_run_ ) )
    { # there are still some datagen which haven't completed
	sleep ( 1 );
    }
}

`> $regdata_filename_`;
for ( my $reg_file_index_ = 0 ; $reg_file_index_ <= $#output_reg_file_list_ ; $reg_file_index_ ++ )
{
    my $this_regdata_filename_ = $output_reg_file_list_ [ $reg_file_index_ ];
    `cat $this_regdata_filename_ >> $regdata_filename_`;
}

foreach my $t_file_ ( @intermediate_files_ )
{
    `rm -rf $t_file_`;
}
