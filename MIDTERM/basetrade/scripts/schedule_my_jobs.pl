#!/usr/bin/perl
#use strict;
#use warnings;
use FileHandle;
use POSIX;

###############****************#############
#Given a job file this script dumps the job to the server with minimum uptime
#Having the output in the shared directory makes tracking them easy
#The process must be in background
###############****************#############


#initialise paths:
my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'};
my $REPO="basetrade";
my $bt_directory_ = $HOME_DIR."/".$REPO;
my $INSTALL_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin/";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";


#user inputs
my $USAGE="$0 cmdfile_  mode_<UNIFORM/LOADDIST, DEFAULT = LOADDIST>";
my $cmdfile_ = $ARGV [ 0 ];

my $mode_ = "LOADDIST";
if($#ARGV >=1) {
  $mode_ = $ARGV [ 1 ];
}

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my @machines_ = ("dvctrader\@10.0.1.45", "dvctrader\@10.0.1.46", "dvctrader\@10.0.1.79", "dvctrader\@10.0.1.68", "dvctrader\@10.0.1.178");
my $pointer_ = 0;
my $lines_ = `cat $cmdfile_ | wc -l`;
my $point_ = 0;
my @uptime_array_ = ();
my $min_index_ = 0;
for (my $j = 0; $j <= $#machines_; $j ++) { push ( @uptime_array_,100 ); }

########Function to find min value of an array########
sub FindMin
{ my $min_ = 1000;
  for (my $k = 0; $k <= $#machines_; $k ++) {
    if($uptime_array_[$k] <= $min_) { $min_ = $uptime_array_[$k]; $min_index_ = $k; }
  }
}


#UNIFORM mode distributes the jobs equally among all the servers
if($mode_ eq "UNIFORM")
{
for (my $j = 1; $j <= $lines_; $j ++)
{    
  my $cmd_ = `cat $cmdfile_ | head -n$j | tail -n1`; chomp($cmd_);
  my $len_ = length $cmd_;
  if($len_>0)
  {
    my $point_ = $j % ($#machines_ + 1);
    my $machine_ = $machines_[$point_];
    print "$machine_ \t$j\n"; 
    my $o1 = `ssh $machine_ '$cmd_'`; 
  }
}
}

#LOADDIST mode runs the job on the server which is least busy
if($mode_ eq "LOADDIST")
{
  for (my $j = 1; $j <= $lines_; $j ++)
  {
    my $cmd_ = `cat $cmdfile_ | head -n$j | tail -n1`; chomp($cmd_);
    my $len_ = length $cmd_;
    if($len_>0)
    {
      my $point_ = $j % ($#machines_ + 1);
      my $machine_ = $machines_[$point_];
      #print "$machine_ \t$j\n"; 
      my $uptime_ = `ssh $machine_ 'uptime'`; chomp($uptime_);
      my @uptime_arr_ = split ' ', $uptime_;
      #print "$uptime_\n";
      $le_ = (length $uptime_arr_[9]) - 1;
      $ut_ = substr $uptime_arr_[9], 0, $le_;
      $uptime_array_[$point_] = $ut_;
      #for (my $k = 0; $k <= $#machines_; $k ++) { print "$uptime_array_[$k] ";} print "\n";
      FindMin();
      print "$machines_[$min_index_] \t$uptime_array_[$min_index_] \t$j\n";
      my $o1 = `ssh $machines_[$min_index_] '$cmd_'`;
    }
  }
}


