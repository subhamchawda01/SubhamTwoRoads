#!/usr/bin/perl

use strict;
use warnings;
use File::Basename; # for basename and dirname
use sigtrap qw(handler signal_handler normal-signals error-signals);
use List::Util qw[min max sum]; # max , min , sum

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
if ( ! ( -d $LIVE_BIN_DIR ) && ! ( -e $LIVE_BIN_DIR ) )
{ $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin"; }

#my $BASETRADEINFODIR="/home/dvctrader/ashwin/tradeinfo/";
my $BASETRADEINFODIR="/spare/local/tradeinfo/";
my $PROD_LRDBFILE_BASE_DIR=$BASETRADEINFODIR."LRDBBaseDir";
my $TEST_LRDBFILE_BASE_DIR=$HOME_DIR."/LRDBTest";
my $PROD_RETLRDBFILE_BASE_DIR=$BASETRADEINFODIR."RetLRDBBaseDir";
my $SPARE_HOME="/spare/local/".$USER."/";
my $SPARE_LRDB_DIR=$SPARE_HOME."lrdbdata";
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";
my $EXCHANGE_SESSIONS_FILE=$TEST_LRDBFILE_BASE_DIR."/exchange_hours_file.txt";
my $PORTFOLIO_INPUTS="/spare/local/tradeinfo/PCAInfo/portfolio_inputs";

require "$GENPERLLIB_DIR/get_port_constituents.pl"; # IsValidPort
require "$GENPERLLIB_DIR/get_exch_from_shortcode.pl"; # IsValidShc GetExchFromSHC
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/exists_with_size.pl";
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/lock_utils.pl"; # for TakeLock, RemoveLock

my $end_yyyymmdd_ = `date +%Y%m%d`; chomp ( $end_yyyymmdd_ );
my $lookback_ = 5;
my $LRDBFILE_BASE_DIR = $PROD_LRDBFILE_BASE_DIR;
$LRDBFILE_BASE_DIR = $TEST_LRDBFILE_BASE_DIR;
my $lrdb_pairs_file_prefix_ = $LRDBFILE_BASE_DIR."/lrdb_pairs_timed"; # /home/hmiyabajaj/LRDBTest/lrdb_pairs_timed
my $l1_events_file_ = $LRDBFILE_BASE_DIR."/avg_l1_events_timeperiod.txt";

my $start_yyyymmdd_ = CalcPrevWorkingDateMult ( $end_yyyymmdd_, $lookback_ );
my %exchange_sessions_ = ();
my %shortcode_start_end_to_l1events_ = ();

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0; 
my $work_dir_ = $SPARE_LRDB_DIR."/".$unique_gsm_id_ ;
while ( ExistsWithSize($work_dir_) )
{
    $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0; 
      $work_dir_ = $SPARE_LRDB_DIR."/".$unique_gsm_id_ ;
}
`rm -rf $work_dir_`; `mkdir -p $work_dir_`;

my $log_file_ = $work_dir_."/main_log_file.txt"; 
print $log_file_."\n";
open LOGFH , ">" , $log_file_ or PrintStacktraceAndDie ( "cannot open the $log_file_\n" );

print LOGFH "start_date = ".$start_yyyymmdd_."\n";
print LOGFH "end_date_ = ".$end_yyyymmdd_."\n";
print LOGFH "lookback = ".$lookback_."\n";
print LOGFH "lrdbdir = ".$LRDBFILE_BASE_DIR."\n";

if (-e $EXCHANGE_SESSIONS_FILE)
{
  open SESSIONS_FILE , "< $EXCHANGE_SESSIONS_FILE " or PrintStacktraceAndDie ( "could not open $EXCHANGE_SESSIONS_FILE for reading\n" );
  my @lines_ = <SESSIONS_FILE>;
  chomp( @lines_ );
  foreach my $line_ ( @lines_ )
  {
    my @words_ = split(",", $line_ );chomp(@words_);
    if ( $words_[0] ne "Exchange")
    {
      my $exch_ = shift @words_;
      $exchange_sessions_{$exch_} = \@words_;
    }
  }
}
if ( -e $l1_events_file_ )
{ `rm -rf $l1_events_file_` };
print LOGFH "l1 events path ".$l1_events_file_."\n";
open L1_EVENTS_FILE , "> $l1_events_file_ " or PrintStacktraceAndDie ( "could not open $l1_events_file_ for writing\n" );

my @exchanges_ = keys %exchange_sessions_;
#print "Exchages @exchanges_\n";
foreach my $this_exchange_ ( @exchanges_ )
{
  foreach my $this_session_ (@{ $exchange_sessions_{$this_exchange_} })
  {
    my $lrdb_pairs_file_path_ = $lrdb_pairs_file_prefix_."_".$this_exchange_."_".$this_session_.".txt";
    if ( ! ExistsWithSize ( $lrdb_pairs_file_path_ ) ) {
      print LOGFH "lrdb pairs file $lrdb_pairs_file_path_ is missing, skipping ....\n";
#      close LOGFH;
#      exit ( 0 );
    }
    open LRDB_PAIRS_FILE, "< $lrdb_pairs_file_path_ " or next;
    print LOGFH "reading $lrdb_pairs_file_path_...\n";
    while ( my $t_line_ = <LRDB_PAIRS_FILE> )
    {
      chomp ( $t_line_ );
      my @lrdb_pairs_line_words_ = split ( ' ' , $t_line_ );
      if ( $#lrdb_pairs_line_words_ < 3 || substr($lrdb_pairs_line_words_[0], 0, 1) eq "#" )
      {
        print LOGFH "skipping $t_line_ [ incorrect format ]\n";
        next;
      }
      my $dep_shortcode_ = $lrdb_pairs_line_words_ [ 0 ];
      my $indep_shortcode_ = $lrdb_pairs_line_words_ [ 1 ];
      my $start_time_ = $lrdb_pairs_line_words_ [ 2 ];
      my $end_time_ = $lrdb_pairs_line_words_ [ 3 ];

      if (!exists($shortcode_start_end_to_l1events_{$dep_shortcode_."_".$start_time_."-".$end_time_}))
      {
        my $l1events_ = GetL1Events ( $dep_shortcode_, $start_time_, $end_time_, $start_yyyymmdd_, $end_yyyymmdd_ );
        $shortcode_start_end_to_l1events_{$dep_shortcode_."_".$start_time_."-".$end_time_} = $l1events_;
        printf L1_EVENTS_FILE $dep_shortcode_."_".$start_time_."-".$end_time_." ".$l1events_."\n";
      }

      if (!exists($shortcode_start_end_to_l1events_{$indep_shortcode_."_".$start_time_."-".$end_time_}))
      {
        my $l1events_ = GetL1Events ( $indep_shortcode_, $start_time_, $end_time_, $start_yyyymmdd_, $end_yyyymmdd_ );
        $shortcode_start_end_to_l1events_{$indep_shortcode_."_".$start_time_."-".$end_time_} = $l1events_;
        printf L1_EVENTS_FILE $indep_shortcode_."_".$start_time_."-".$end_time_." ".$l1events_."\n";
      }
    }
    close LRDB_PAIRS_FILE;

  }
}
close L1_EVENTS_FILE;


sub GetL1Events
{
  my $shc_ = shift;
  my $start_time_ = shift;
  my $end_time_  = shift;
  my $start_date_ = shift;
  my $end_date_ = shift;
  my $exec_cmd_;
  if (IsValidPort($shc_))
  {
    $exec_cmd_ = "grep -w $shc_ $PORTFOLIO_INPUTS";
    my $portfolio_line_ = `$exec_cmd_`;chomp($portfolio_line_);
    my @portfolio_shcs_ = split(" ",$portfolio_line_);chomp(@portfolio_shcs_);
    @portfolio_shcs_ = @portfolio_shcs_[3..$#portfolio_shcs_];
    my @portfolio_l1events_ = ( );
    my $l1events_;
    foreach my $port_shc_ (@portfolio_shcs_)
    {
      $l1events_ = GetL1Events ( $port_shc_, $start_time_, $end_time_, $start_date_, $end_date_ );
      push (@portfolio_l1events_, $l1events_);
    }
    my @sorted_portfolio_events_  = sort @portfolio_l1events_;
    my $port_l1events_ = sum(@portfolio_l1events_)/@portfolio_l1events_;
    return $port_l1events_;
  } else
  {
    $exec_cmd_ = $SCRIPTS_DIR."/get_avg_l1_events_in_timeperiod.pl $shc_ $start_date_ $end_date_ $start_time_ $end_time_ | tail -2 | head -1";
    my $line_ =  `$exec_cmd_`;my @words_ = split(" ",$line_); chomp ( @words_ );
    printf LOGFH "L1Events for $shc_ $start_time_ $end_time_ $words_[-1]\n";
    return $words_[-1];
  }
}

sub END
{
  close LRDB_PAIRS_FILE;
  close LOGFH;
}
