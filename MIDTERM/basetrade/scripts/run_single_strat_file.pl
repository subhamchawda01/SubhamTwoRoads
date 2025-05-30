#!/usr/bin/perl

# \file ModelScripts/run_single_strat_file.pl
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

sub AddResultsToDatabase;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME="/spare/local/".$USER."/";
my $REPO = "basetrade";

my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib/";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
$LIVE_BIN_DIR = $BIN_DIR if $USER ne "dvctrader";

my $TRADELOG_DIR = "/spare/local/logs/tradelogs/";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/get_files_pending_sim.pl"; # GetFilesPendingSimAndPnlSamplesFromShcDateDir
require "$GENPERLLIB_DIR/get_seconds_to_prep_for_shortcode.pl"; #GetDataStartTimeForStrat
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile

# start
my $USAGE="$0 shortcode input_strat_list_filename tradingdate_ temp_strategy_list_filenames_ [WORK_DIR] [GLOBALRESULTSDBDIR|DB] [GLOBALPNLSAMPLESDIR|INVALIDDIR] [MKT_MODEL|DEF=DEF] [custom sim_strategy]";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 1 ); }

my $shortcode_ = $ARGV [ 0 ];
my $input_strat_list_filename_ = $ARGV [ 1 ];
my $tradingdate_ = $ARGV [ 2 ];
my $temp_strategy_list_filenames_ = $ARGV[3];

my $GLOBALRESULTSDBDIR = "DB";
my $GLOBALPNLSAMPLESDIR = "INVALIDDIR";
$GLOBALRESULTSDBDIR = "DEF_DIR" if $USER ne "dvctrader";
my $this_day_work_dir_;


if ( $#ARGV >= 4 ) { $this_day_work_dir_ = $ARGV[4]; }
if ( $#ARGV >= 5 ) { $GLOBALRESULTSDBDIR = $ARGV[5]; } 
if ( $#ARGV >= 6 ) { $GLOBALPNLSAMPLESDIR = $ARGV[6]; }

my $MKT_MODEL = "DEF";
my $SIM_STRATEGY_EXEC = $LIVE_BIN_DIR."/sim_strategy";
if ( $#ARGV >= 7 ) { $MKT_MODEL = $ARGV[7]; }
if ( $#ARGV >= 8 ) { $SIM_STRATEGY_EXEC = $ARGV[8]; }

my $is_db_ = int($GLOBALRESULTSDBDIR eq "DB") ;
my $dump_pnl_samples_into_dir_ = int($GLOBALPNLSAMPLESDIR ne "INVALIDDIR") ;
my $check_pnl_samples_ = int($is_db_ || $dump_pnl_samples_into_dir_); #check for pnl_samples only if they will be dumped into db or dir

my $debug_ = 1;
my $market_model_index_ = 0;
if ( $MKT_MODEL eq "DEF" ) {
  $MKT_MODEL = GetMarketModelForShortcode ( $shortcode_ );
}
if ( $MKT_MODEL >= 0 ) {
  $market_model_index_ = $MKT_MODEL ;
}

if ( IsDateHoliday ( $tradingdate_ ) || ( IsProductHoliday ( $tradingdate_, $shortcode_ ) ) ) {
  print "$tradingdate_ is Holiday!";
  exit 0;
}                

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_gsm_id_);
my $pnl_stats_command = "$MODELSCRIPTS_DIR/get_pnl_stats_2.pl $this_tradesfilename_ X";

if ( ! defined $this_day_work_dir_ ) {
  $this_day_work_dir_ = $SPARE_HOME."RS/".$shortcode_."/".$tradingdate_;
}
$this_day_work_dir_ .= "/".$unique_gsm_id_;
if ( -d $this_day_work_dir_ ) { `rm -rf $this_day_work_dir_`; }
`mkdir -p $this_day_work_dir_`;

my $main_log_file_ = $this_day_work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);
my $this_simoutfilename_ = $this_day_work_dir_."/simout.".$tradingdate_.".".int($unique_gsm_id_);
my @strategy_filevec_ = ($input_strat_list_filename_);

my @strat_vec_ = `cat $input_strat_list_filename_ 2>/dev/null`;
chomp ( @strat_vec_ );
@strat_vec_ = grep { $_ ne "" } @strat_vec_;

my @strat_namevec_ = `cat $temp_strategy_list_filenames_ 2>/dev/null`;
chomp ( @strat_namevec_ );
@strat_namevec_ = grep { $_ ne "" } @strat_namevec_;

if ($#strat_vec_ != $#strat_namevec_)
{
  print STDERR "Please provide same number of strats and strat names\n";
  exit(1);
}

my $data_start_time_ = 0.0;
my $USE_DEFAULT_START_TIME = 0;
if ( $USE_DEFAULT_START_TIME == 0) {
  $data_start_time_ = GetDataStartTimeForStrat( $input_strat_list_filename_, $tradingdate_ );
}

my $sim_cmd_ = "$SIM_STRATEGY_EXEC SIM $input_strat_list_filename_ $unique_gsm_id_ $tradingdate_ $market_model_index_ 0 $data_start_time_ 0 ADD_DBG_CODE -1 > $this_simoutfilename_ 2>&1";
#print "$sim_cmd_ \n";
my @exec_output_lines_=`$sim_cmd_`;
print "SIM output File : $this_simoutfilename_\n";

my $temp_pnlsamples_list_file_ = $this_day_work_dir_."/temp_pnl_samples_list_file_".$tradingdate_.".txt";
my $temp_results_list_file_ = $this_day_work_dir_."/temp_results_list_file_".$tradingdate_.".txt" ;

# Check if non-empty trades file exists 
if (! ( -e $this_tradesfilename_ && -s $this_tradesfilename_) )
{
  PrintErrAndLog ( "Trades file of non-zero size not found for following command: \n$sim_cmd_\n\n" );
  exit(1);
}

my @pnlstats_output_lines_=`$pnl_stats_command 2>&1`; chomp ( @pnlstats_output_lines_ );
my @pnlsamples_output_lines_=`$MODELSCRIPTS_DIR/get_pnl_samples.pl $this_tradesfilename_ 2>&1`; 
chomp( @pnlsamples_output_lines_ );

`rm -f $this_tradesfilename_`;
my $this_logfilename_ = `echo $this_tradesfilename_ | sed 's/trades./log./g'`;
`rm -f $this_logfilename_`;
my  %unique_id_to_pnlstats_map_ = ( );


open PNLSAMPLES, "> $temp_pnlsamples_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_pnlsamples_list_file_ for writing\n" );
foreach my $pnlsample_line_ ( @pnlsamples_output_lines_ ) {
  my @rwords_ = split ( ' ', $pnlsample_line_ );
  if ( $#rwords_ >= 2 ) {
    my $unique_sim_id_ = $rwords_[0];
    splice ( @rwords_, 0, 1 ); # remove the first word since it is unique_sim_id_
    print PNLSAMPLES join ( ' ', @rwords_ );
    print PNLSAMPLES "\n";
  }
}
close PNLSAMPLES;

for ( my $t_pnlstats_output_lines_index_ = 0 ; $t_pnlstats_output_lines_index_ <= $#pnlstats_output_lines_; $t_pnlstats_output_lines_index_ ++ )
{
  my @rwords_ = split ( ' ', $pnlstats_output_lines_[$t_pnlstats_output_lines_index_] );
  if( $#rwords_ >= 2 ) {
    my $unique_sim_id_ = $rwords_[0];
    splice ( @rwords_, 0, 1 ); # remove the first word since it is unique_sim_id_
    $unique_id_to_pnlstats_map_ { $unique_sim_id_ } = join ( ' ', @rwords_ );

    PrintDBGLog ( "For $unique_sim_id_ pnlstatsmappedto: ".$unique_id_to_pnlstats_map_ { $unique_sim_id_ }."\n" );
  }
}

print "#strats : ".($#strat_vec_+1)." , #pnl_stats : ".(keys %unique_id_to_pnlstats_map_)." and #pnl_samples : ".($#pnlsamples_output_lines_ + 1)."\n";

my @list_of_unique_ids_in_TRLF = ( );
open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );

my $psindex_ = 0;
for ( my $psindex_ = 0 ; $psindex_ < keys %unique_id_to_pnlstats_map_ ; $psindex_ ++ )
{
  my $unique_sim_id_ = GetUniqueSimIdFromCatFile( $input_strat_list_filename_, $psindex_ );
  printf TRLF "%s\n",$unique_id_to_pnlstats_map_ { $unique_sim_id_ };
  push ( @list_of_unique_ids_in_TRLF , $unique_sim_id_ );
}
close TRLF;

print "Adding results to :".$GLOBALRESULTSDBDIR." and samples to :".$GLOBALPNLSAMPLESDIR."\n";
AddResultsToDatabase();

sub AddResultsToDatabase 
{
  if ( ( ExistsWithSize ( $temp_results_list_file_ ) ) &&
      ( ExistsWithSize ( $temp_strategy_list_filenames_ ) ) &&
      ( ExistsWithSize ( $temp_pnlsamples_list_file_ ) ) )
  {
    my $results_database_dir_ = $is_db_ ? "INVALIDDIR" : $GLOBALRESULTSDBDIR."/".$shortcode_;
    my $pnlsamples_database_dir_ = !$dump_pnl_samples_into_dir_ ? "INVALIDDIR" : $GLOBALPNLSAMPLESDIR."/".$shortcode_;

    my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_global_database.pl $temp_strategy_list_filenames_ $temp_results_list_file_ $tradingdate_ $results_database_dir_ $is_db_ $temp_pnlsamples_list_file_ $pnlsamples_database_dir_"; 

    my $output_results_ = `$exec_cmd 2>/dev/null`;

    PrintDBGLog ( "$exec_cmd\n" ); 

    if ( $output_results_ =~ /FAILED/ )
    {
# do not rm these files
      PrintErrAndLog ( "Adding results to database FAILED on $tradingdate_\n" );

      open TEMP_STRATEGY_LIST_FILEHANDLE, "< $temp_strategy_list_filenames_ " or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_filenames_\n" );
      my @temp_strat_list_lines_ = <TEMP_STRATEGY_LIST_FILEHANDLE>;
      chomp ( @temp_strat_list_lines_ );
      close TEMP_STRATEGY_LIST_FILEHANDLE ;

      PrintErrAndLog ( "\nStratlist for which add_results_to_global_database failed : \n" );
      PrintErrAndLog ( $_."\n" )  foreach @temp_strat_list_lines_;
      exit ( 1 );
    }
  }
  else
  {
    if ( ! ExistsWithSize ( $temp_strategy_list_filenames_ ) )
    {
      PrintErrAndLog ( "StrategyListFile $temp_strategy_list_filenames_ missing\n" );
    }
    else
    {
      if ( ! ExistsWithSize ( $temp_results_list_file_ ) )
      {
        PrintErrAndLog ( "Date: $tradingdate_ ResultsFile $temp_results_list_file_ missing for: $input_strat_list_filename_ listfile: $temp_strategy_list_filenames_ \n" );
      }
      elsif ( ! ExistsWithSize ( $temp_pnlsamples_list_file_)  )
      {
        PrintErrAndLog ( "Date: $tradingdate_ PnlSamplesFile $temp_pnlsamples_list_file_ missing for: $input_strat_list_filename_  listfile: $temp_strategy_list_filenames_ \n" );
      }

      open TEMP_STRATEGY_LIST_FILEHANDLE, "< $temp_strategy_list_filenames_ " or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_filenames_\n" );
      my @temp_strat_list_lines_ = <TEMP_STRATEGY_LIST_FILEHANDLE>;
      chomp ( @temp_strat_list_lines_ );
      close TEMP_STRATEGY_LIST_FILEHANDLE ;

      PrintErrAndLog ( "\nStratlist for which result_generation failed : \n" );
      PrintErrAndLog ( $_."\n" )  foreach @temp_strat_list_lines_;  
    }
    exit ( 1 );
  }
}

sub PrintErrAndLog
{
  my $str_ = shift;

  print STDERR $str_;
  if ( $debug_ ) {
    print $main_log_file_handle_ $str_;
  }
}

sub PrintDBGLog
{
  my $str_ = shift;

  if ( $debug_ ) {
    print $main_log_file_handle_ $str_;
  }
}

