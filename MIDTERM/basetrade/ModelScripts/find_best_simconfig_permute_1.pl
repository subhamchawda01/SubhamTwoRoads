#!/usr/bin/perl
#
# \file ModelScripts/find_best_simconfig_permute_1.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
#
use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;


sub GetSimConfigFiles;
sub RunAnalyseSimReal;
sub ReportResults;
my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME = "/spare/local/".$USER."/";

my $TRADELOG_DIR = "/spare/local/logs/tradelogs/";
my $FBSCP_WORK_DIR = $SPARE_HOME."FBSCP/";

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $SIM_TRADES_LOCATION = "/spare/local/logs/tradelogs/";
my $SIM_LOG_LOCATION = "/spare/local/logs/tradelogs/";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
my $ANALYSE_SIM_EXEC = $MODELSCRIPTS_DIR."/analyse_sim_real_for_product.pl ";

if ( $USER eq "diwakar" || $USER eq "ravi" )
{
      $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}
my $SIM_STRATEGY_EXEC = $LIVE_BIN_DIR."/sim_strategy";
my $SIM_CONFIG_TO_DESCRIPTION_EXEC = $LIVE_BIN_DIR."/get_sim_config_description_from_file";



require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult

require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams

require "$GENPERLLIB_DIR/is_weird_sim_day_for_shortcode.pl"; # IsWeirdSimDayForShortcode

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst

my $USAGE= "$0 SHORTCODE START_DATE END_DATE TIMEPERIOD SIMCONFIG_PERMUTATION_FILE ";
if ( $#ARGV < 0 )
{
  print $USAGE."\n";
  exit;
}
my $shortcode_ = $ARGV[0];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 1 ] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 2 ] );
my $time_period_ = $ARGV[ 3 ];
my $simconfig_permutation_file_ = $ARGV [ 4 ];


my @sim_config_file_list_ = ();
my %simconfig_to_pnl_score_vec_ = ();
my %simconfig_to_pnl_score_ = ();
my %simconfig_to_vol_score_vec_ = ();
my %simconfig_to_vol_score_ = ();

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $FBSCP_WORK_DIR.$unique_gsm_id_;

for ( my $i = 0 ; $i < 30 ; $i ++ )
{
  if ( -d $work_dir_ )
  {
    print STDERR "Surprising but this dir exists\n";
    $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
    $work_dir_ = $FBSCP_WORK_DIR.$unique_gsm_id_;
  }
  else
  {
    last;
  }
}

my $local_simconfigs_dir_ = $work_dir_."/simconfigs_dir";
if ( ! ( -d $FBSCP_WORK_DIR ) ) { `mkdir -p $FBSCP_WORK_DIR`; }
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_simconfigs_dir_ ) ) { `mkdir -p $local_simconfigs_dir_`; }

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush ( 1 );
print "Log file = ".$main_log_file_."\n";


GetSimConfigFiles();
if ( $#sim_config_file_list_ < 0 )
{
      print $main_log_file_handle_ "Exiting due to empty sim_config_file_list_\n";
}
else
{
  RunAnalyseSimReal();
  ReportResults();
}

exit;



sub GetSimConfigFiles
{
  print $main_log_file_handle_ "# GetSimConfigFiles\n";
  @sim_config_file_list_ = PermuteParams ( $simconfig_permutation_file_ , $local_simconfigs_dir_ );
  chomp ( @sim_config_file_list_ );
  print $main_log_file_handle_ "# PermuteParams = ".$#sim_config_file_list_."\n";

}

sub RunAnalyseSimReal 
{
#run the sim_real for individual days for further analysis and store the resutls
  foreach my $sim_config_file_name_ ( @sim_config_file_list_ )
   {
     my @day_pnl_vec_ = ();
     my @day_vol_vec_ = ();
     my $this_date_ = $trading_end_yyyymmdd_;
     while ( $this_date_ >= $trading_start_yyyymmdd_ )
     {
       print " For Simconfig: " . $sim_config_file_name_." Date: ".$this_date_."\n";
       my $exec_cmd_ = "$ANALYSE_SIM_EXEC $shortcode_ $this_date_ $time_period_ $sim_config_file_name_ ";
       print $main_log_file_handle_ "$exec_cmd_ \n";
       print "=========================================\n"; 
       my @res_ = `$exec_cmd_`; chomp(@res_);
       print join('\n',@res_)."\n";
       print "=========================================\n";
       foreach my $line_ (@res_ )
       {
         if ( index ( $line_, "PNLVOLSTAT:") >= 0 )
         {
           my @line_words_ = split(' ',$line_);
           push ( @day_pnl_vec_, $line_words_[1] );
           push ( @day_vol_vec_ , $line_words_[2] );
         }
       }
       $this_date_ = CalcPrevDate( $this_date_ );
     }
     foreach my $val_ (@day_pnl_vec_ )
     {
       print $main_log_file_handle_ $val_." ";
     }
     print $main_log_file_handle_ "\n";
     $simconfig_to_pnl_score_vec_{$sim_config_file_name_} = \@day_pnl_vec_;
   }
}

sub ReportResults 
{
#send mail

  print $main_log_file_handle_ "#ReportResults\n";
  foreach my $key_ (keys %simconfig_to_pnl_score_vec_ )
  {
    my $sim_score_vec_ = $simconfig_to_pnl_score_vec_{$key_};

    foreach my $val_ ( @$sim_score_vec_ )
    {
      print $main_log_file_handle_ $val_." ";
    }
    print $main_log_file_handle_ "\n";
    my $avg_pnl_score_ = GetAverage($sim_score_vec_ );
    $simconfig_to_pnl_score_{$key_} = $avg_pnl_score_;
    print $main_log_file_handle_ "SIMCONFIG: ".$key_." -> ".$avg_pnl_score_ ."\n";
  }

  print $main_log_file_handle_ "===================RESULTS===========\n";
  print $main_log_file_handle_ " #simconfig #pnl_score_ \n";
  my @keys_ = sort { $simconfig_to_pnl_score_{$a} <=> $simconfig_to_pnl_score_{$b} } keys(%simconfig_to_pnl_score_);
  foreach my $key_ ( @keys_ )
  {
    print $main_log_file_handle_ $key_." ".$simconfig_to_pnl_score_{$key_}."\n";
  }
}
