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


my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";

require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/results_db_access_manager.pl"; #ExecuteWriteQueryOnResultsDB 
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; #BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; #CalcPrevWorkingDateMult

if($#ARGV<1)
{
  print "USAGE : <script> shc_/ALL sdate [edate=TODAY] [strat_list_/strat_dir_/ALL=ALL] [results_dir_/DB=DB] [hard_removal(0|1)=0] [is_option_strat(0|1)=0]\n";
  exit(0);
}

my $uid_ = `date +%N`; chomp($uid_);
my $shc_ = $ARGV[0];
my $all_shc = $shc_ eq "ALL";

my $start_date_ = GetIsoDateFromStrMin1($ARGV[1]);
my $end_date_ = GetIsoDateFromStrMin1("TODAY");
if ( $#ARGV >= 2 ) { $end_date_ = GetIsoDateFromStrMin1($ARGV[2]); }

my $strat_list_ = "ALL";
if ( $#ARGV >= 3 ) { $strat_list_ = $ARGV[3]; }
my $all_strats_ = $strat_list_ eq "ALL";

my $results_dir_ = "DB";
if ( $#ARGV >= 4 ) { $results_dir_ = $ARGV[4]; }
my $is_db = $results_dir_ eq "DB";

my $hard_remove_ = 0;
if ( $#ARGV >= 5 ) { $hard_remove_ = int($ARGV[5] > 0) + 0; }

my $is_option_strat_ = 0;
if ( $#ARGV >= 6 ) { $is_option_strat_ = int($ARGV[6] > 0) + 0; }

if ( $hard_remove_ > 0 ) {
  print "WARNING! This will delete the results from Database. Please confirm one of the following options\n1) Continue with delete\n2) Just set regeneration flag instead\n3) Abort\nInput Option: ";
  my $inp_ = <STDIN>; chomp($inp_); $inp_ = int($inp_); 
  if ( $inp_ != 1 ) {
    if ( $inp_ == 2 ) {
      print "Just Setting the Flag. You made the right choice!\n";
      $hard_remove_ = 0;
    }
    else {
      print "Thank God you aborted! Saved so many lives!\n"; 
      exit(0);
    }
  }
  else {
    print "Deleting Results! I hope you know what you just did!\n";
  }
}


my @strats_ = ();
if ( !$all_strats_ ) {
  if ( -f $strat_list_ ) {
    open STRATLIST, "< $strat_list_" or PrintStacktraceAndDie("ERROR: can't open file : $strat_list_ for reading.\n");
    my @t_strats_ = <STRATLIST>;chomp(@t_strats_);

    foreach my $t_strat_ (@t_strats_) {
      my $t_base_ = `basename $t_strat_`; chomp($t_base_);
      push(@strats_, $t_base_);
    }
    close STRATLIST;
  }
  elsif ( -d $strat_list_ ) {
    my @t_strats_ = MakeStratVecFromDir($strat_list_);
    foreach my $t_strat_ (@t_strats_)
    {
      my $t_base_ = `basename $t_strat_`; chomp($t_base_);
      push(@strats_, $t_base_);
    }
  }
}

my $strats_removed_ = 0;

if ( $is_db ) {
  my $query_ = "UPDATE wf_results JOIN wf_configs ON wf_results.configid = wf_configs.configid SET regenerate = ? WHERE date >= ? AND date <= ?";
  my @args_ = ('Y', $start_date_, $end_date_);

  if ( $hard_remove_ > 0 ) {
    #delete the results instead of just setting up a flag
    $query_ = "DELETE wf_results FROM wf_results JOIN wf_configs ON wf_results.configid = wf_configs.configid WHERE date >= ? AND date <= ?";
    @args_ = ($start_date_, $end_date_);
  }

  if (( !$all_shc ) && !($is_option_strat_)) { 
    $query_ = $query_." AND shortcode = ?";
    push ( @args_, $shc_ );
  }

  if ( $all_strats_ ) {
    $strats_removed_ += ExecuteWriteQueryOnResultsDB ( $query_, @args_ ); 
  }
  else {
    if($is_option_strat_) {
      $query_ = $query_." AND cname like ?";
      foreach my $t_strat_ (@strats_){
        $strats_removed_ += ExecuteWriteQueryOnResultsDB ( $query_, @args_ , $t_strat_."%" ); 
      }
    }
    else  {
      $query_ = $query_." AND cname = ?";

      foreach my $t_strat_ (@strats_) {
        $strats_removed_ += ExecuteWriteQueryOnResultsDB ( $query_, @args_ , $t_strat_ ); 
      }
    }
  }
}
else {
  my $t_dt_ = $end_date_;
  my $t_result_file_ = "tmp_$uid_";

  while ( $t_dt_ >= $start_date_ ) {
    my ( $yyyy, $mm, $dd ) = BreakDateYYYYMMDD ( $t_dt_ );
    my @result_files_ = ("$results_dir_/$shc_/$yyyy/$mm/$dd/results_database.txt");

    if ( $all_shc ) {
      @result_files_ = `ls $results_dir_/*/$yyyy/$mm/$dd/results_database.txt`;
      chomp(@result_files_);
    }

    foreach my $result_file_ (@result_files_) {
      next if ( ! -s $result_file_ );

      if ( $all_strats_ ) {
        my $t_num_strats_ = `wc -l $result_file_ | awk '{print \$1}'`; chomp($t_num_strats_);
        $strats_removed_ += $t_num_strats_;
        `rm $result_file_`; 
      }
      else {
        `rm -f $t_result_file_`;
        open TEMPRESULTFILE, "> $t_result_file_" or PrintStacktraceAndDie("ERROR: can't open file : $t_result_file_ for writing.\n");
        open RESULTFILE, "< $result_file_" or PrintStacktraceAndDie("ERROR: can't open file : $result_file_ for reading.\n");

        while (my $line_ = <RESULTFILE>) {
          chomp($line_);
          my @words_ = split(' ', $line_);
          
          if ($#words_ >= 0) {
            if ( grep {$_ eq $words_[0]} @strats_ )
            {
              $strats_removed_++;
              next;
            }
          }
          print TEMPRESULTFILE "$line_\n";
        }
        close TEMPRESULTFILE;
        close RESULTFILE;
        `mv $t_result_file_ $result_file_`;
      }
    }

    $t_dt_ = CalcPrevWorkingDateMult($t_dt_,1);
  }
}

print "Total Results Removed : $strats_removed_\n";
