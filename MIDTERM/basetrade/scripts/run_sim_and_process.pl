#!/usr/bin/perl

# \file ModelScripts/run_simulations_and_write_results.pl
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
use FileHandle;
use POSIX;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME="/spare/local/".$USER."/";
my $REPO = "basetrade";

my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib/";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/get_files_pending_sim.pl"; # GetFilesPendingSimAndPnlSamplesFromShcDateDir
require "$GENPERLLIB_DIR/get_seconds_to_prep_for_shortcode.pl"; #GetDataStartTimeForStrat

my $TRADELOG_DIR = "/spare/local/logs/tradelogs/";

# start
my $USAGE="$0 shortcode input_strat_list_filename temp_strat_run_id tradingdate_ [market_model_index_]";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];
my $input_strat_list_filename_ = $ARGV [ 1 ];
my $temp_sim_run_id_ = $ARGV [ 2 ];
my $tradingdate_ = $ARGV [ 3 ];
my $MKT_MODEL = "DEF";
if ( $#ARGV >= 4 ) { $MKT_MODEL = $ARGV[4]; }

my $debug_ = 1;
my $market_model_index_ = 0;
if ( $MKT_MODEL eq "DEF" )
{
  $MKT_MODEL = GetMarketModelForShortcode ( $shortcode_ );
}
if ( $MKT_MODEL >= 0 )
{
 	$market_model_index_ = $MKT_MODEL ;
}

if ( IsDateHoliday ( $tradingdate_ ) || ( IsProductHoliday ( $tradingdate_, $shortcode_ ) ) ) {
	print "$tradingdate_ is Holiday!";
	exit 0;
}                

my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($temp_sim_run_id_);
my $pnl_stats_command = "$MODELSCRIPTS_DIR/get_pnl_stats_2.pl $this_tradesfilename_";

my $temp_output_log = $TRADELOG_DIR."/output_log.".$tradingdate_.".".int($temp_sim_run_id_);
my @strategy_filevec_ = ($input_strat_list_filename_);
my @this_day_strategy_filevec_ = ();

GetFilesPendingSimAndPnlSamplesFromShcDateDir ( $shortcode_, $tradingdate_, \@strategy_filevec_ , \@this_day_strategy_filevec_, "DB" , "INVALIDDIR");
my $sim_with_no_db_results = @this_day_strategy_filevec_;
if ( $sim_with_no_db_results  == 0 ) {
	print "DB result exists! \n";
	exit ( 0 );
}

my $data_start_time_ = 0.0;
my $USE_DEFAULT_START_TIME = 0;
if ( $USE_DEFAULT_START_TIME == 0) {
	$data_start_time_ = GetDataStartTimeForStrat( $input_strat_list_filename_, $tradingdate_ );
}
my $sim_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $input_strat_list_filename_ $temp_sim_run_id_ $tradingdate_ $market_model_index_ 0 $data_start_time_ 0 ADD_DBG_CODE -1 2>&1";
# This line was printed by sim_strategy (stdout/stderr)
print "$sim_cmd_ \n";
my @exec_output_lines_=`$sim_cmd_`;
print "@exec_output_lines_ \n";
my @pnlstats_output_lines_=`$pnl_stats_command 2>&1`; chomp ( @pnlstats_output_lines_ );
my @pnlsamples_output_lines_=`$MODELSCRIPTS_DIR/get_pnl_samples.pl $this_tradesfilename_ 2>&1`;
# print "$sim_cmd_ \n $pnl_stats_command \n @pnlstats_output_lines_ \n @pnlsamples_output_lines_ \n";
my  $unique_id_to_pnlstats_map_ = "";
my $this_day_work_dir_ = $SPARE_HOME."RS/".$shortcode_."/".$tradingdate_."/".$temp_sim_run_id_;

if ( -d $this_day_work_dir_ ) { `rm -rf $this_day_work_dir_`; }
	`mkdir -p $this_day_work_dir_`;

my $main_log_file_ = $this_day_work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

for ( my $t_pnlstats_output_lines_index_ = 0 ; $t_pnlstats_output_lines_index_ <= $#pnlstats_output_lines_; $t_pnlstats_output_lines_index_ ++ )
{
  my @rwords_ = split ( ' ', $pnlstats_output_lines_[$t_pnlstats_output_lines_index_] );
  if( $#rwords_ >= 2 )
  {
    my $unique_sim_id_ = $rwords_[1];
    splice ( @rwords_, 0, 1 ); # remove the first two words since it is unique_sim_id_
    $unique_id_to_pnlstats_map_ = join ( ' ', @rwords_ );

    if ( $debug_ == 1 )
    {
      print $main_log_file_handle_ "For $unique_sim_id_ pnlstatsmappedto: ".$unique_id_to_pnlstats_map_."\n";
    }
  }
}

my $temp_pnlsamples_list_file_ = $this_day_work_dir_."/temp_pnl_samples_list_file_".$tradingdate_.".txt";
#print "$temp_pnlsamples_list_file_ \n";
open PNLSAMPLES, "> $temp_pnlsamples_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_pnlsamples_list_file_ for writing\n" );
for ( my $t_pnlsamples_output_lines_index_ = 0 ; $t_pnlsamples_output_lines_index_ <= $#pnlsamples_output_lines_; $t_pnlsamples_output_lines_index_ ++ )
{                       
  my @rwords_ = split ( ' ', $pnlsamples_output_lines_[$t_pnlsamples_output_lines_index_] );
  if ( $#rwords_ >= 2 )
  {
    my $unique_sim_id_ = $rwords_[1];
    splice ( @rwords_, 0, 1 ); # remove the first word since it is unique_sim_id_
      print PNLSAMPLES join ( ' ', @rwords_ );
    print PNLSAMPLES "\n";
  }
}
close PNLSAMPLES;

my $temp_results_list_file_ = $this_day_work_dir_."/temp_results_list_file_".$tradingdate_.".txt" ;
#print "$temp_results_list_file_  \n";
my @list_of_unique_ids_in_TRLF = ( );
my $unique_strat_id_  = $temp_sim_run_id_ ;
open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );
for ( my $t_exec_output_lines_index_ = 0; $t_exec_output_lines_index_ <= $#exec_output_lines_; $t_exec_output_lines_index_ ++ )
{
  if ( $exec_output_lines_ [ $t_exec_output_lines_index_ ] =~ /SIMRESULT/ )
  { # SIMRESULT pnl volume                                                                                                                                                                                               
    my @rwords_ = split ( ' ', $exec_output_lines_[$t_exec_output_lines_index_] );
    if ( $#rwords_ >= 2 ) 
    {
      splice ( @rwords_, 0, 1 ); # remove the first word since it is "SIMRESULT", typically results files just have pnl, volume, etc                                                                                     
      my $remaining_simresult_line_ = join ( ' ', @rwords_ );
      if ( $unique_id_to_pnlstats_map_ eq '' )
      {
        $unique_id_to_pnlstats_map_ = "0 0 0 0 0 0 0 0 0 0 0 0 0";
      }
      printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$remaining_simresult_line_, $unique_id_to_pnlstats_map_, $unique_strat_id_  ;
      printf TRLF "%s %s\n",$remaining_simresult_line_,$unique_id_to_pnlstats_map_;
      push ( @list_of_unique_ids_in_TRLF , $unique_strat_id_ );

      if ( $debug_ == 1 )
      {
        printf $main_log_file_handle_ "Adding to TRLF %s %s\n",$remaining_simresult_line_,$unique_id_to_pnlstats_map_;
      }
    }
    else
    {
      PrintStacktraceAndDie ( "SIMRESULT line has less than 3 words\n" ); 
    }
  }
}
close TRLF;

my $temp_strategy_list_filenames_ = $this_day_work_dir_."/temp_strategy_list_file_".$tradingdate_.".txt" ;
open TSLF, "> $temp_strategy_list_filenames_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_filenames_ for writing\n" );
printf TSLF "%s \n", $input_strat_list_filename_;
close TSLF;

#Consider result only if the run was fine
if ( ( ExistsWithSize ( $temp_results_list_file_ ) ) &&
    ( ExistsWithSize ( $temp_strategy_list_filenames_ ) ) &&
    ( ExistsWithSize ( $temp_pnlsamples_list_file_ ) ) )
{
  my $results_database_dir_ =  "INVALIDDIR";
  my $pnlsamples_database_dir_ =  "INVALIDDIR" ;
  my $is_db_ = 1;
  my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_global_database.pl $temp_strategy_list_filenames_ $temp_results_list_file_ $tradingdate_ $results_database_dir_ $is_db_ $temp_pnlsamples_list_file_ $pnlsamples_database_dir_"; 
 
  my $output_results_ = `$exec_cmd`;

  if ( $debug_ == 1 ) 
  { 
    print $main_log_file_handle_ "$exec_cmd\n"; 
    print $main_log_file_handle_ "Output: $output_results_\n"; 
  }

  if ( $output_results_ =~ /FAILED/ )
  {
# do not rm these files
    print STDERR "AR2GDb.pl FAILED on $tradingdate_. output :\n $output_results_\n";
    if ( $debug_ == 1 )
    {
      print $main_log_file_handle_ "AR2GDb.pl FAILED on $tradingdate_. output :\n $output_results_\n";
    }

    open TEMP_STRATEGY_LIST_FILEHANDLE, "< $temp_strategy_list_filenames_ " or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_filenames_\n" );
    my @temp_strat_list_lines_ = <TEMP_STRATEGY_LIST_FILEHANDLE>;
    chomp ( @temp_strat_list_lines_ );
    close TEMP_STRATEGY_LIST_FILEHANDLE ;

    for ( my $tslf_idx_ = 0 ; $tslf_idx_ <= $#temp_strat_list_lines_ ; $tslf_idx_ ++ )
    {
      printf STDERR "AR2GDb.pl failed list file item : %s\n", $temp_strat_list_lines_[$tslf_idx_] ;
      if ( $debug_ == 1 )
      {
        printf $main_log_file_handle_ "AR2GDb.pl failed list file item : %s\n", $temp_strat_list_lines_[$tslf_idx_] ;
      }
    }
    exit ( 0 );
  }
  else
  {
# probably not needed after this
#`rm -f $temp_results_list_file_`;
#`rm -f $temp_pnlsamples_list_file_`;
#`rm -f $temp_strategy_list_filenames_`;
  }
}
else
{
  if ( ! ExistsWithSize ( $temp_strategy_list_filenames_ ) )
  {
    print STDERR "StrategyListFile $temp_strategy_list_filenames_ missing\n" ;
    if ( $debug_ == 1 )
    { 
      print $main_log_file_handle_ "StrategyListFile $temp_strategy_list_filenames_ missing\n" ;
    }
  }
  else
  {
    if ( ! ExistsWithSize ( $temp_results_list_file_ ) )
    {
      print STDERR "Date: $tradingdate_ ResultsFile $temp_results_list_file_ missing for: $input_strat_list_filename_ listfile: $temp_strategy_list_filenames_ \n" ;
      if ( $debug_ == 1 )
      {
        print $main_log_file_handle_ "Date: $tradingdate_ ResultsFile $temp_results_list_file_ missing for: $input_strat_list_filename_  listfile: $temp_strategy_list_filenames_ \n" ;
      }
    }
    elsif ( ! ExistsWithSize ( $temp_pnlsamples_list_file_)  )
    {
      print STDERR "Date: $tradingdate_ PnlSamplesFile $temp_pnlsamples_list_file_ missing for: $input_strat_list_filename_  listfile: $temp_strategy_list_filenames_ \n" ;
      if ( $debug_ == 1 )
      {
        print $main_log_file_handle_ "Date: $tradingdate_ PnlSamplesFile $temp_pnlsamples_list_file_ missing for: $input_strat_list_filename_  listfile: $temp_strategy_list_filenames_ \n" ;
      }
    }

    open TEMP_STRATEGY_LIST_FILEHANDLE, "< $temp_strategy_list_filenames_ " or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_filenames_\n" );
    my @temp_strat_list_lines_ = <TEMP_STRATEGY_LIST_FILEHANDLE>;
    chomp ( @temp_strat_list_lines_ );
    close TEMP_STRATEGY_LIST_FILEHANDLE ;

    for ( my $tslf_idx_ = 0 ; $tslf_idx_ <= $#temp_strat_list_lines_ ; $tslf_idx_ ++ )
    {
      printf STDERR "failed list file item : %s\n", $temp_strat_list_lines_[$tslf_idx_] ;
      if ( $debug_ == 1 )
      {
        printf $main_log_file_handle_ "failed list file item : %s\n", $temp_strat_list_lines_[$tslf_idx_] ;
      }
    }
  }
}

