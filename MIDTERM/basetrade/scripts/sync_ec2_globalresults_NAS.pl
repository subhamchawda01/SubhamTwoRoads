#!/usr/bin/perl
#
## \file ModelScripts/find_best_model_for_strategy_var_pert.pl
##
## \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
##  Address:
##       Suite No 162, Evoma, #14, Bhattarhalli,
##       Old Madras Road, Near Garden City College,
##       KR Puram, Bangalore 560049, India
##       +91 80 4190 3551
##
#


use strict;
use warnings;
use POSIX;
use feature "switch";
use Fcntl qw (:flock);
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/;
use FileHandle;
use Digest::MD5 qw(md5 md5_hex md5_base64);
use sigtrap qw(handler signal_handler normal-signals error-signals);

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $MODELING_STIR_STRATS_DIR=$MODELING_BASE_DIR."/stir_strats";

require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate

if($#ARGV<1)
{
  print "USAGE : <script> source_dir_ target_dir_ [<shc_list_file_> / ALL] [start_date_ / INVALID] [end_date_ / INVALID] \n";
  exit(0);
}

sub SyncResultsShc;
sub IsValidStrat;

my $uid_ = `date +%N`; chomp($uid_);
my $optshc_ = "ALL";
my $home_results_dir_ = shift;
my $nas_results_dir_ = shift;
my $start_date_ = "INVALID";
my $end_date_ = "INVALID";

if ($#ARGV >= 0) { $optshc_ = shift; }
if ($#ARGV >= 0) { $start_date_ = shift; }
if ($#ARGV >= 0) { $end_date_ = shift; } 

my $lockfile_ = "$HOME_DIR/sync_ec2_lock_".md5_hex($nas_results_dir_);
my $is_lock_created_ = 0;
if ( -f $lockfile_  )
{
  print "lock($lockfile_) exists for $nas_results_dir_, a sync might be already running for this target dir. exiting\n";
  exit(1);
}
else
{
  `echo $nas_results_dir_ > $lockfile_`;
  $is_lock_created_ = 1;
  print "created lock $lockfile_ . Make sure it is deleted after the run.\n";
}

my $work_dir_ = $SPARE_HOME."/ec2_merge/".$uid_; 
`mkdir -p $work_dir_`;
my $tmp_result_file_ = $work_dir_."/tmpfile";      


my @all_shc_;
if ( $optshc_ eq "ALL" )
{
  opendir DIR, $home_results_dir_ or PrintStacktraceAndDie ( "Could not open home results directory $home_results_dir_ for reading.");
  @all_shc_ = readdir ( DIR );
  close ( DIR );
}
else 
{
  open FILE, "< $optshc_" or PrintStacktraceAndDie ( "Could not open shc_list_file $optshc_ for reading.");
  @all_shc_ = <FILE>; chomp(@all_shc_);
  close(FILE);
}

print join(", ", @all_shc_ )."\n"; 
foreach my $shc_ ( @all_shc_ )
{
  if ( $shc_ eq ".." or $shc_ eq "." or $shc_ eq "" ) { next; }
  SyncResultsShc ( $shc_ );
}
`rm -f $work_dir_`;
exit(1);
#------------------------------------------

sub SyncResultsShc
{
  my $shc_ = $_[0];
  my $nas_shc_results_dir_ = "$nas_results_dir_/$shc_";
  my $home_shc_results_dir_ = "$home_results_dir_/$shc_";
  
  if ( ! ( -d $home_shc_results_dir_ ) )
  {
    print "The shortcode results directory $home_shc_results_dir_ is not present, omitting any merge for $shc_\n";
    return;
  }

  print "Syncing for Shortcode : $shc_ \n";

  my @home_shc_result_files_ = MakeStratVecFromDir($home_shc_results_dir_);

  #creating map of valid strats, doing this once to save time to do it for every sourccefile
  my %valid_strats_map_;
  my @valid_strats_ = MakeStratVecFromDir("$MODELING_STRATS_DIR/$shc_");
  foreach my $valid_strat_ (@valid_strats_)
  {
    my $valid_strat_base_=`basename $valid_strat_`; chomp($valid_strat_base_);
    $valid_strats_map_{$valid_strat_base_} = 1;
  }
  @valid_strats_ = MakeStratVecFromDir("$MODELING_STIR_STRATS_DIR/$shc_");
  foreach my $valid_strat_ (@valid_strats_)
  {
    my $valid_strat_base_=`basename $valid_strat_`; chomp($valid_strat_base_);
    $valid_strats_map_{$valid_strat_base_} = 1;
  }
  
  foreach my $src_result_file_ ( @home_shc_result_files_ )
  {
    my $dir_dd_ = `dirname $src_result_file_`; chomp($dir_dd_);
    my $dir_mm_ = `dirname $dir_dd_`; chomp($dir_mm_);
    my $dir_yyyy_ = `dirname $dir_mm_`; chomp($dir_yyyy_);
  
    my $file_base_ = `basename $src_result_file_`; chomp($file_base_);
    my $dd_ = `basename $dir_dd_`; chomp($dd_);
    my $mm_ = `basename $dir_mm_`; chomp($mm_);
    my $yyyy_ = `basename $dir_yyyy_`; chomp($yyyy_);
  
    my $this_date_ = $yyyy_.$mm_.$dd_;
  
    if ( ! ValidDate ($this_date_) ) { next; }
  
    if ( ValidDate ($start_date_) && $this_date_ < $start_date_ ) { next; }
  
    if ( ValidDate ($end_date_) && $this_date_ > $end_date_ ) { next; }
  
    my $tgt_dir_dd_ = "$nas_shc_results_dir_/$yyyy_/$mm_/$dd_";
 
    if ( ! ( -d $tgt_dir_dd_ ) ) { `mkdir -p $tgt_dir_dd_`; }
  
    my $tgt_result_file_ = $tgt_dir_dd_."/".$file_base_;
  
    if ( ! ( -f $tgt_result_file_ ) )
    {
      my $exec_cmd_ = "cp $src_result_file_ $tgt_result_file_";
      `$exec_cmd_`;
    }
    else
    {

      #taking care of results in srcfile
      open SRCSTRATLINES, "< $src_result_file_" or PrintStacktraceAndDie("ERROR: can't open the file : $src_result_file_ for reading.\n");
      my @src_stratlines_ = <SRCSTRATLINES>; chomp(@src_stratlines_);
      close ( SRCSTRATLINES );

      open DONESTRATLINES, "> $tmp_result_file_" or PrintStacktraceAndDie("ERROR: can't open the file : $tmp_result_file_ for writing.\n");
      my %done_strats_;
      foreach my $stratline_ ( @src_stratlines_ )
      {
        my @words_ = split(' ', $stratline_);
        my $strat_ = $words_[0]; chomp($strat_);

        if ( (! exists ($done_strats_{$strat_})) && exists ($valid_strats_map_{$strat_}) )
        {
          $done_strats_{$strat_} = 1;
          print DONESTRATLINES "$stratline_\n";
        } 
      }
      close ( DONESTRATLINES );


      #taking care of results only in tgtfile and not in srcfile
      open TGTSTRATLINES, "< $tgt_result_file_" or PrintStacktraceAndDie("ERROR: can't open the file : $tgt_result_file_ for reading.\n");
      my @tgt_stratlines_ = <TGTSTRATLINES>; chomp(@tgt_stratlines_);
      close ( TGTSTRATLINES );

      open RESULTLINES, ">> $tmp_result_file_" or PrintStacktraceAndDie("ERROR: can't open the tempfile : $tmp_result_file_ for writing.\n");
      foreach my $line ( @tgt_stratlines_ )
      {
        my @words_ = split ( " ", $line );
        my $strat_base_ = $words_[0]; chomp($strat_base_);

        if ( (! exists ($done_strats_{$strat_base_})) && exists($valid_strats_map_{$strat_base_}) )
        {
          $done_strats_{$strat_base_} = 1;
          print RESULTLINES "$line\n";
        }
      }
      close ( RESULTLINES );

      #We are done, moving tmpfile to tgfile
      `mv $tmp_result_file_ $tgt_result_file_`;
    }
  }
}

sub IsValidStrat
{
  my $strat_base_ = shift;
  my $shc_ = shift;
  my $stratpath_ = `ls $MODELING_STRATS_DIR/$shc_/\*/$strat_base_ 2>/dev/null`; chomp ($stratpath_);        
  my $stir_stratpath1_ = `ls $MODELING_STIR_STRATS_DIR/$shc_/\*/$strat_base_ 2>/dev/null`; chomp($stir_stratpath1_);                
  my $stir_stratpath2_ = `ls $MODELING_STIR_STRATS_DIR/$shc_/$strat_base_ 2>/dev/null`; chomp($stir_stratpath2_);
  if ( $stratpath_ ne "" || $stir_stratpath1_ ne "" || $stir_stratpath2_ ne "" )
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

sub signal_handler
{
    die "Caught a signal $!\n";
}

sub END
{
  if( $is_lock_created_ )  
  {      
    `rm -f $lockfile_`;
    print "removed lock  $lockfile_\n";                  
  }
}

