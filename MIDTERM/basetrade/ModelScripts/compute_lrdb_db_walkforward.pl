#!/usr/bin/perl

use strict;
use warnings;
use File::Basename;    # for basename and dirname
use sigtrap qw(handler signal_handler normal-signals error-signals);
use List::Util qw[min max sum];    # max , min
#use Time::Piece;
#use Time::Seconds;

my $USER             = $ENV{'USER'};
my $HOME_DIR         = $ENV{'HOME'};
my $REPO             = "basetrade";
my $GENPERLLIB_DIR   = $HOME_DIR . "/" . $REPO . "_install/GenPerlLib";
my $SCRIPTS_DIR      = $HOME_DIR . "/" . $REPO . "_install/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR . "/" . $REPO . "_install/ModelScripts";
my $LIVE_BIN_DIR     = $HOME_DIR . "/LiveExec/bin";
if ( !( -d $LIVE_BIN_DIR ) && !( -e $LIVE_BIN_DIR ) ) { $LIVE_BIN_DIR = $HOME_DIR . "/" . $REPO . "_install/bin"; }

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl";     # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE = "$0 walkforward_start_date_ walkforward_periodicity_nmonths dep|ALL|filename indep|ALL|filename [TYPE(CHANGE|RETURNS)=CHANGE] [mode = 1] [recompute = 0] [distributed = 1]\n";
$USAGE = $USAGE . "# Mode = 0 : No files will be changed. New Values will be printed on stdout.\n";
$USAGE = $USAGE . "# Mode = 1 : Input date will be changes to 1st of specified month. Eg. 20150510 => 20150501. This is to keep only one dated file for each month.\n";
$USAGE = $USAGE . "# Mode = 2 : A new file with the specified date will be created. Should be used in special circumstances only.\n";

if ( $#ARGV < 3 ){
  print $USAGE. "\n";
  exit(0);
}

#reading args and initializing
my $DISTRIBUTED_EXECUTOR = "/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/run_my_job.py";
my $wf_start_date_ = GetIsoDateFromStrMin1(shift);
my $wd_nmonths_ = shift;
my $input_dep_   = shift;    # if all is specified we use all the *indep pairs from the lrdb_pairs_ file
my $input_indep_ = shift;    # if all is specified we use all the dep* paris from the lrdb_pairs_ file

my $lrdb_type_str_    = "CHANGE";
my $mode_             = 1;                                   # 0 => print only, 2 => force creation of a new file
my $recompute_        = 0;
my $distributed_      = 1;
if (@ARGV) { $lrdb_type_str_ = shift; }

if (!($lrdb_type_str_ eq "RETURNS" || $lrdb_type_str_ eq "CHANGE")) {
  print $USAGE. "\n";
  print "Type should be CHANGE or RETURNS \n";
  exit(0);
}


if (@ARGV) { $mode_          = int(shift) + 0; }
if (@ARGV) { $recompute_     = int(shift) + 0; }
if (@ARGV) { $distributed_   = int(shift) + 0; }

my $wf_end_date_ = GetIsoDateFromStrMin1("TODAY-1");

my $curr_date_ = $wf_start_date_;
my $itr_ = 0;

my @instructions_vec_ = ( );

while ( $curr_date_ <= $wf_end_date_ && $itr_ < 50) {
  print "Computing and Adding LRDB for date: $curr_date_; ($input_dep_ $input_indep_ $lrdb_type_str_, mode: $mode_, recompute: $recompute_)\n";

  my $exec_cmd_ = "$MODELSCRIPTS_DIR/compute_lrdb_db.pl $curr_date_ $input_dep_ $input_indep_ $lrdb_type_str_ $mode_ $recompute_";
  push ( @instructions_vec_, $exec_cmd_ );

  my $yy_ = substr($curr_date_, 0, 4);
  my $mm_ = substr($curr_date_, 4, 2);
  my $dd_ = substr($curr_date_, 6, 2);

  $mm_ += $wd_nmonths_;
  if ( $mm_ > 12 ) {
    $yy_ += 1;
    $mm_ -= 12;
  }
  $curr_date_ = sprintf("%04d%02d%02d", $yy_, $mm_, $dd_);
  $itr_++;
#my $t = Time::Piece->strptime(shift, "%Y%m%d");
#$t += ONE_MONTH;
#$curr_date_ = $t->strftime("%Y%m%d");
}

if ( $distributed_ ) {
  my $file_ = "lrdb_cmds_".$input_dep_."_".$input_indep_;
  $file_ = GetCSTempFileName( $file_ );
  open FHANDLE, "> $file_" or PrintStacktraceAndDie ( "Could not open $file_ for writing\n" );
  print FHANDLE $_."\n" foreach @instructions_vec_;
  close FHANDLE;

  my $dist_exec_cmd_ = "$DISTRIBUTED_EXECUTOR -n dvctrader -m 1 -f $file_ -s 1";
  print "Command executed: $dist_exec_cmd_\n";
  my @lrdb_out_ = `$dist_exec_cmd_ 2>&1`;
    print $_ foreach @lrdb_out_;
      print "\n";
}
else {
  foreach my $exec_cmd_ ( @instructions_vec_ ) {
    my @lrdb_out_ = `$exec_cmd_ 2>&1`;
    print $_ foreach @lrdb_out_;
    print "\n";
  }
}

