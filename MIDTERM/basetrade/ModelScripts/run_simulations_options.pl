#!/usr/bin/perl

# \file ModelScripts/run_simulations_for_options.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes as input :
# directory name ( runs simulations for all files below directory )
# or strategy_list_filename ( a file with lines with just 1 word corresponding to the fullpath of a strategyfile )
# trading_start_yyyymmdd
# trading_end_yyyymmdd
# [ localresults_base_dir ] ( if not specified then use Globalresultsdbdir )
#
# for each valid_date in the period :
#   make a temporary_this_day_strategy_list_file of only the entries which do not have data in the results_base_dir
#   breaks the temporary_this_day_strategy_list_file into blocks of 15 entries
#   for each block of <= 15 entries : create temp_strategy_block_file ( with just the names of files in that block )
#     runs a base_trade_init on this tradingdate and this block and collects the results in a temp_results_block_file
#     then uses add_results_to_local_database ( temp_strategy_block_file, temp_results_block_file, results_base_dir )
#
# At the end looking at the original strategy_list_filename and results_base_dir
# build a summary_results.txt of the following format :
# STRATEGYFILENAME strategy_file_name
# date_1 pnl_1 volume_1
# date_2 pnl_2 volume_2
# .
# .
# date_n pnl_n volume_n
# STATISTICS pnl_average_ pnl_stdev_ volume_average_ pnl_sharpe_ pnl_conservative_average_ pnl_median_average_

use strict;
use warnings;
use POSIX;
use feature "switch";
use List::Util qw/max min/;

sub SanityCheckInputArguments ;
sub LivingEachDay;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/s3_utils.pl"; # S3PutFilePreservePath

if ( $USER ne "dvctrader" )
{
  $LIVE_BIN_DIR = $BIN_DIR;
}

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 

my $GLOBALRESULTSTRADESDIR = "/NAS1/ec2_globalresults_trades"; # since this will be synced to S3 anyways


my $hostname_ = `hostname`;
if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
{ # AWS
  $GLOBALRESULTSTRADESDIR = "/NAS1/ec2_globalresults_trades"; # since this will be synced to S3 anyways
}

# flag to control if trades file named as : trades.YYYYMMDD.w_pbt_strat_ilist_OMix_OMix_lmtest4_200_10.pfi_4
# are generated and synced to S3.
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult # IsDateHoliday
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1 # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_files_pending_sim.pl"; # GetFilesPendingSim
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/exists_and_same.pl"; # ExistsAndSame
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_matchbase_stir.pl"; # MakeStratVecFromDirMatchBase
require "$GENPERLLIB_DIR/check_ilist_data.pl"; # CheckIndicatorData


my $USAGE="$0 shortcode strat_directory/stratlist_file trading_start_yyyymmdd trading_end_yyyymmdd [GLOBALRESULTSDBDIR|DEF_DIR|DB=DB] [--SPECIFICSHC|-s SHCNAME/NONE] [--GLOBALPNLSAMPLESDIR|-p DIRNAME/INVALIDDIR] [--USE_DISTRIBUTED|-d 0/1] [--DEBUG|-dbg 0/1]";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $base_dir = $ARGV[1];
my $trading_start_yyyymmdd_ = max ( 20110901, $ARGV[2] );
my $trading_end_yyyymmdd_ = $ARGV[3];
my $MAX_STRAT_FILES_IN_ONE_SIM = 1; # since functionality for multiple strat in one file is not there for options
my $USE_DISTRIBUTED = 1;

my $GLOBALRESULTSDBDIR = "DB";
if ( $USER ne "dvctrader" )
{
	$GLOBALRESULTSDBDIR = "DEF_DIR";
}
my $MKT_MODEL = "DEF";

if ( $#ARGV >= 4 ) { $GLOBALRESULTSDBDIR = $ARGV[4]; }

my $GLOBALPNLSAMPLESDIR = "INVALIDDIR";
my $is_specific_shc_ = 0;
my $specific_shc_ = "NONE";
ReadArgs( );

if ( $is_specific_shc_ == 1 ) 
{
	#Removing total result of the strats
	my $remove_command_ = $SCRIPTS_DIR."/remove_strat_results.pl $shortcode_ $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ $base_dir $GLOBALRESULTSDBDIR 0";
        `$remove_command_`;
}

#setting default directories
if ( $GLOBALRESULTSDBDIR eq "DEF_DIR" )
{
	$GLOBALRESULTSDBDIR = $HOME_DIR."/globalresults";
	if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
	{ # AWS
		$GLOBALRESULTSDBDIR = $HOME_DIR."/ec2_globalresults";
        }
}

my $is_db_ = int($GLOBALRESULTSDBDIR eq "DB");
my $dump_pnl_samples_into_dir_ = int($GLOBALPNLSAMPLESDIR ne "INVALIDDIR") ;
my $check_pnl_samples_ = int($is_db_ || $dump_pnl_samples_into_dir_); #check for pnl_samples only if they will be dumped into db or dir

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $DEBUG = 1;

my $shift_di_symbols_ = -1;
my @independent_parallel_commands_ = ( );

#Variables for distributed (celery) version
my $DISTRIBUTED_EXECUTOR = "/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/run_my_job.py";
my $celery_shared_location = "/media/shared/ephemeral17/temp_strats/";
my $SHARED_LOCATION = "/media/shared/ephemeral17/commands";
my $CUSTOM_EXEC = "";

CheckIsDistributedSharedResultsFolder( );
SanityCheckInputArguments( );

my @strategy_filevec_ = ();

LocalMakeStratVecFromDirMatchBaseStir ( );

LivingEachDay ( );

ExecuteCommands ( );

exit( 0 );


###### subs ######


sub ReadArgs
{
  my $current_mode_ = "";
  foreach my $this_arg_ ( @ARGV[ 5..$#ARGV  ] ) {
    given ( $this_arg_ ) {
      when ( "=" ) { next; };
      when ( "--GLOBALPNLSAMPLESDIR" ) { $current_mode_ = "GLOBALPNLSAMPLESDIR"; next; }
      when ( "--USE_DISTRIBUTED" ) { $current_mode_ = "USE_DISTRIBUTED"; next; }
      when ( "--DEBUG" ) { $current_mode_ = "DEBUG"; next; }
      when ( "--SPECIFICSHC" ) { $current_mode_ = "SPECIFICSHC"; next; }
      when ( "-p" ) { $current_mode_ = "GLOBALPNLSAMPLESDIR"; next; }
      when ( "-d" ) { $current_mode_ = "USE_DISTRIBUTED"; next; }
      when ( "-dbg" ) { $current_mode_ = "DEBUG"; next; }
      when ( "-s" ) { $current_mode_ = "SPECIFICSHC"; next; }
      default {
        if ( $current_mode_ eq "GLOBALPNLSAMPLESDIR" ) {
          $GLOBALPNLSAMPLESDIR = $this_arg_;
        }
        elsif ( $current_mode_ eq "USE_DISTRIBUTED" ) {
          $USE_DISTRIBUTED = $this_arg_;
        }
        elsif ( $current_mode_ eq "DEBUG" ) {
          $DEBUG = $this_arg_;
        }
        elsif ( $current_mode_ eq "SPECIFICSHC" ) {
          $is_specific_shc_ = 1;
          $specific_shc_ = $this_arg_;          
        }
      }
    }
  }
}

sub LivingEachDay
{
  my @resultsfiles_=();
  my $tradingdate_ = $trading_end_yyyymmdd_;
  my $max_days_at_a_time_ = 2000;
  for ( my $j = 0 ; $j < $max_days_at_a_time_ ; $j ++ ) 
  {
START_OF_SIM_FOR_DAY:
    if ( SkipWeirdDate ( $tradingdate_ ) || IsDateHoliday ( $tradingdate_ ) )
    {
      $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
      next;
    }
    if ( ( ! ValidDate ( $tradingdate_ ) ) ||
        ( $tradingdate_ < $trading_start_yyyymmdd_ ) ) 
    {
      last;
    }

    {
      my @this_day_strategy_filevec_ = ();
      if ( $check_pnl_samples_ ) {
        GetFilesPendingSimAndPnlSamplesFromShcDateDir ( $shortcode_, $tradingdate_, \@strategy_filevec_ , \@this_day_strategy_filevec_, $GLOBALRESULTSDBDIR , $GLOBALPNLSAMPLESDIR); 
      }
      else {
        GetFilesPendingSimFromShcDateDir ( $shortcode_, $tradingdate_, \@strategy_filevec_ , \@this_day_strategy_filevec_, $GLOBALRESULTSDBDIR );   
      }

      if ( $#this_day_strategy_filevec_ >= 0 )
      {
        my $this_day_work_dir_ = $SPARE_HOME."RS/".$shortcode_."/".$tradingdate_."/".$unique_gsm_id_;
        if ( -d $this_day_work_dir_ ) { `rm -rf $this_day_work_dir_`; }
        `mkdir -p $this_day_work_dir_`;

        my $main_log_file_ = $this_day_work_dir_."/main_log_file.txt";
        my $main_log_file_handle_ = FileHandle->new;
        $main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" ); 
        $main_log_file_handle_->autoflush(1);
        print $main_log_file_handle_ "Logfile for $tradingdate_\n";

        my $temp_strategy_list_file_index_ = 0; # only a counter for differnet stratlistfilenames
        my $this_strat_bunch_next_strat_id_ = 10011; # start from beginnin
        while ($temp_strategy_list_file_index_ <= $#this_day_strategy_filevec_)
        { 
          my $temp_strategy_cat_file_ = $this_day_work_dir_."/temp_strategy_list_file_".$temp_strategy_list_file_index_.".txt" ; 
          my $temp_strategy_list_filenames_ = $this_day_work_dir_."/temp_strategy_list_filenames_".$temp_strategy_list_file_index_.".txt" ;
          open TSCF, "> $temp_strategy_cat_file_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_cat_file_ for writing\n" );
          open TSLF, "> $temp_strategy_list_filenames_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_filenames_ for writing\n" );
          my @strat_index_vec_ = () ; # Single strats index

          my $this_strat_filename_ = $this_day_strategy_filevec_[$temp_strategy_list_file_index_] ;
          print $main_log_file_handle_ "Processing $this_strat_filename_ assigning stratid $this_strat_bunch_next_strat_id_\n";
          open THIS_STRAT_FILEHANDLE, "< $this_strat_filename_ " or PrintStacktraceAndDie ( "Could not open $this_strat_filename_\n" );
          my @this_strat_lines_ = <THIS_STRAT_FILEHANDLE>;
          close THIS_STRAT_FILEHANDLE ;

          for ( my $tsl_index_ = 0 ; $tsl_index_ <= $#this_strat_lines_; $tsl_index_ ++ )
          {
            my @tsl_words_ = split ( ' ', $this_strat_lines_[$tsl_index_] );
            $tsl_words_[4] = $this_strat_bunch_next_strat_id_ ++;
            push ( @strat_index_vec_, $tsl_words_[2] ) ;
            print TSCF join ( ' ', @tsl_words_ )."\n";
            print $main_log_file_handle_ "Adding to TSCF ".join ( ' ', @tsl_words_ )."\n";
          }
          print TSLF "$this_strat_filename_\n";
          close TSCF;

          {
            my $temporary_strat_location = $temp_strategy_cat_file_;
            my $temporary_stratname_location = $temp_strategy_list_filenames_;

            if ( $USE_DISTRIBUTED ) {
              $temporary_strat_location = $celery_shared_location.$temp_strategy_cat_file_; #Copy the strat file to a shared location, so that it is accessible to celery workers
              $temporary_stratname_location = $celery_shared_location.$temp_strategy_list_filenames_;
              my $copy_cmd = "mkdir -p `dirname $temporary_strat_location`; chmod a+rw `dirname $temporary_strat_location`; cp $temp_strategy_cat_file_ $temporary_strat_location; chmod -R a+rw $temporary_strat_location";
              `$copy_cmd`;
              $copy_cmd = "mkdir -p `dirname $temporary_stratname_location`; chmod a+rw `dirname $temporary_stratname_location`; cp $temp_strategy_list_filenames_ $temporary_stratname_location; chmod -R a+rw $temporary_stratname_location";
            `$copy_cmd`;
            }

            my $script_path = "/home/dvctrader/basetrade_install/scripts/run_single_strat_file_options.pl";
          
            my $exec_cmd_ = "$script_path $shortcode_ $temporary_strat_location $tradingdate_ $temporary_stratname_location $GLOBALRESULTSDBDIR $GLOBALPNLSAMPLESDIR $specific_shc_";
            push ( @independent_parallel_commands_ , $exec_cmd_ );
          }

          $temp_strategy_list_file_index_ ++;
          $this_strat_bunch_next_strat_id_ = 10011; # start from beginning again
        }
          $main_log_file_handle_->close;
      } # if
    }
    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
  }    
}


sub ExecuteCommands
{
  if ( $USE_DISTRIBUTED ) {
    my $commands_file_id_  = `date +%N`;
    chomp( $commands_file_id_ );
    my $commands_file_ = $SHARED_LOCATION."/".$commands_file_id_;
    my $commands_file_handle_ = FileHandle->new;
    $commands_file_handle_->open ( "> $commands_file_ " ) or PrintStacktraceAndDie ( "Could not open $commands_file_ for writing\n" );
    print $commands_file_handle_ "$_ \n" for @independent_parallel_commands_;
    close $commands_file_handle_;
    my $dist_exec_cmd_ = "$DISTRIBUTED_EXECUTOR -n dvctrader -m 1 -f $commands_file_ -s 1";
    if ( $CUSTOM_EXEC ne "" ) {
      $dist_exec_cmd_ = "$DISTRIBUTED_EXECUTOR -n dvctrader -m 1 -f $commands_file_ -s 1 -e $CUSTOM_EXEC";
    }
    if ( defined $ENV{'QUICKRUN'} ) {
      $dist_exec_cmd_ = $dist_exec_cmd_." -q autoscalegroup";
    }
    print "Command executed: $dist_exec_cmd_ \n";
    my @output_lines_ = `$dist_exec_cmd_`;
    chomp( @output_lines_ );
    print "$_ \n" for @output_lines_;
  }
  else {
    foreach my $command ( @independent_parallel_commands_ ) {
      my @output_lines_ = `$command`;
      my $return_val_ = $?;
      if ( $return_val_ != 0 || $DEBUG == 1 ) {
        print "Output: \n".join ( "", @output_lines_ )."\n";
      }
    }
  }
}

sub LocalMakeStratVecFromDirMatchBaseStir
{
  my $top_directory_ = $base_dir;
  $top_directory_ = File::Spec->rel2abs ( $top_directory_ );
  my @ret_abs_file_paths_ = ();
  if ( -d $top_directory_ )
  {
    if (opendir my $dh, $top_directory_)
    {
      my @t_list_=();
      while ( my $t_item_ = readdir $dh)
      {
        push @t_list_, $t_item_;
      }
      closedir $dh;

      for my $dir_item_ (@t_list_)
      {
        next if $dir_item_ eq '.' || $dir_item_ eq '..';

        my $strat_file_path_ = $top_directory_."/".$dir_item_;
        if ( -f $strat_file_path_ )
        {
          push @ret_abs_file_paths_, $strat_file_path_;
        }
        if ( -d $strat_file_path_ )
        {
          push @ret_abs_file_paths_, MakeStratVecFromDirMatchBaseStir ( $strat_file_path_ ) ;
        }
      }
    }
  }
  @strategy_filevec_ = @ret_abs_file_paths_;
}

sub CheckIsDistributedSharedResultsFolder {
  if ( ! $is_db_ && $USE_DISTRIBUTED ) {
    if( index($GLOBALRESULTSDBDIR, "/media/shared/ephemeral") != 0 ) {
      my $folder_id = `date +%N`;
      chomp($folder_id);
      $GLOBALRESULTSDBDIR = "/media/shared/ephemeral16/run_sim_logs/".$folder_id."/";
      `mkdir -p $GLOBALRESULTSDBDIR; chmod a+rw $GLOBALRESULTSDBDIR`;
      print "Using ".$GLOBALRESULTSDBDIR." instead. Please use shared folder with distributed version \n";
    }
  }
}

sub SanityCheckInputArguments
{
  if ( ! ( ( -d $base_dir ) || ( -f $base_dir ) ) )
  { 
    print STDERR "$base_dir isn't a file or directory";
    exit( 0 );
  }

  if ( ! ( $trading_start_yyyymmdd_ ) )
  {
    print STDERR "TRADING_START_YYYYMMDD missing\n";
    exit ( 0 );
  }

  if ( ! ( ValidDate ( $trading_start_yyyymmdd_ ) ) )
  {
    print STDERR "TRADING_START_YYYYMMDD not Valid\n";
    exit ( 0 );
  }

  if ( ! ( $trading_end_yyyymmdd_ ) )
  {
    print STDERR "TRADING_END_YYYYMMDD missing\n";
    exit ( 0 );
  }

  if ( ! ( ValidDate ( $trading_end_yyyymmdd_ ) ) )
  {
    print STDERR "TRADING_END_YYYYMMDD not Valid\n";
    exit ( 0 );
  }

}
