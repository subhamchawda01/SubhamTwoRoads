#!/usr/bin/perl

use strict;
use warnings;
use FileHandle;
use List::Util qw/max min/; # for max

sub GetNewStratName ;

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; #GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult 
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllPIDSTerminated

my $USAGE="$0 strat start_date end_date";
if($#ARGV < 2)	{print $USAGE."\n"; exit(1);}
my $strat_ = shift;
my $sd_ = shift; $sd_ = GetIsoDateFromStrMin1($sd_);
my $ed_ = shift; $ed_ = GetIsoDateFromStrMin1($ed_);

#Make Working Dir
my $uid_ = `date +%N`; chomp($uid_);
my $work_dir_ = $SPARE_HOME."/RSP/".$uid_;
`mkdir -p $work_dir_`;
print STDERR "WORK_DIR: $work_dir_\n";
my $logfile_ = $work_dir_."/log.txt";
open my $LOGFH , "> $logfile_" or PrintStacktraceAndDie ( "Could not open $logfile_ for writing\n" );
print STDERR "LOGFILE: $logfile_\n";

#Make temp copy of strat for running sim
my $strat_basename_ = `basename $strat_`; chomp($strat_basename_);
`cp $strat_ $work_dir_`;
$strat_ = $work_dir_."/".$strat_basename_;

print $LOGFH "STRAT: $strat_\n";
print $LOGFH "SD: $sd_\n";
print $LOGFH "ED: $ed_\n";

my $curr_date_ = $ed_;
my @parallel_sim_cmds_ = ();
my @dates_vec_ = ();
while ( ValidDate($curr_date_) and $curr_date_ >= $sd_ )
{
  my $this_sim_cmd_ = "$BINDIR/sim_strategy SIM $strat_ $uid_ $curr_date_ > $work_dir_/$curr_date_ 2>>$work_dir_/err";
  push ( @parallel_sim_cmds_, $this_sim_cmd_ );
  push ( @dates_vec_, $curr_date_);
  $curr_date_ = CalcPrevWorkingDateMult($curr_date_,1);
}
print $LOGFH "DATEVEC: @dates_vec_\n";

RunParallelProcesses(\@parallel_sim_cmds_, GetMaxCoresToUseInParallel(), $LOGFH);

foreach my $t_date_ ( @dates_vec_ )
{
  my $this_resfile_ = $work_dir_."/".$t_date_ ;
  if ( -s $this_resfile_ )
  {
    my $simline_ = `cat $this_resfile_ | head -n1`; chomp($simline_);
    my @simwords_ = split( ' ', $simline_);
    if ( @simwords_ == 7 && $simwords_[0] eq "SIMRESULT" )
    {
      $simwords_[0] = $t_date_;
      print "@simwords_\n";
    }
  }
}
close $LOGFH;
`rm -r $work_dir_`;

