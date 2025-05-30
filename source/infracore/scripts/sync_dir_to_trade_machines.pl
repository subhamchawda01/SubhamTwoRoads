#!/usr/bin/perl

# \file scripts/sync_dir_to_all_machines.pl
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
use POSIX;
use List::Util qw[min max]; # max , min
use File::Basename; # for basename and dirname

my $USER = $ENV { 'USER' };
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."/scripts";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # for ExistsWithSize
require "$GENPERLLIB_DIR/ec2_utils.pl"; # IsEc2Machine
require "$GENPERLLIB_DIR/get_all_machines_vec.pl"; # GetTradeMachinesVec

if ( $#ARGV < 0 )
{
    print "USAGE: full_dirname [target_dirname / -1]  [username=$USER] [delete_rest? 0/1]\n";
    exit (0);
}

my $delete_flag_ = 0;

my $this_dirname_ = $ARGV[0];
my $target_dirname_ = $this_dirname_;
if ( $#ARGV >= 1 && $ARGV[1] != -1 )
{
  $target_dirname_ = $ARGV[1];
}
if ( $#ARGV >= 2 ) 
{
  $USER = $ARGV[2];
}
if ( $#ARGV >= 3 )
{
  $delete_flag_ = $ARGV[3];
}
my $delete_word_ = ($delete_flag_==1) ? "--delete-after" : "";

if ( $this_dirname_ =~ /\/$/ )
{
    $this_dirname_ = substr ( $this_dirname_, 0, length ( $this_dirname_ ) - 1 );
}
my $base_dirname_ = dirname ($this_dirname_);
my $base_target_dirname_ = dirname ($target_dirname_);

if ( IsEc2Machine() ) {
#use ny11 to sync to other ny and prod servers
  my $ny11_ip = "10.23.74.51";
  `ssh $USER\@$ny11_ip mkdir -p $this_dirname_`;
  `rsync -Lravz $delete_word_ $this_dirname_ $USER\@$ny11_ip:$base_dirname_`;
  `ssh $USER\@$ny11_ip /home/$USER/$REPO/scripts/sync_dir_to_trade_machines.pl $this_dirname_ $target_dirname_ $USER $delete_flag_`;
} 
else {
  my $HOSTNAMEIP=`hostname -i`; chomp($HOSTNAMEIP);
  my @trade_machines_ = GetTradeMachinesVec();

  print "Total Machines: ".@trade_machines_."\n";
  foreach my $remote_machine_ ( @trade_machines_ )
  {
      if ( $remote_machine_ ne $HOSTNAMEIP )
      {
          print "Syncing to ".$remote_machine_." ...\n";
          `ssh $USER\@$remote_machine_ mkdir -p $target_dirname_`;
          `rsync -Lravz $delete_word_ $this_dirname_ $USER\@$remote_machine_:$base_target_dirname_`;
      }
  }
}
1;
