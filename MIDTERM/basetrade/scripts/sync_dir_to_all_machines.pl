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
require "$GENPERLLIB_DIR/get_all_machines_vec.pl"; # IsEc2Machine

if ( $#ARGV < 0 )
{
    print "USAGE: full_dirname [username=$USER] [delete_rest? 0/1] [rsync_options]\n";
    exit (0);
}

my $delete_flag_ = 0;

my $this_dirname_ = $ARGV[0];
if ( $#ARGV >= 1 ) 
{
  $USER = $ARGV[1];
}
if ( $#ARGV >= 2 )
{
  $delete_flag_ = $ARGV[2];
}
my $rsync_options_ = "";
if ( $#ARGV >= 3 )
{
    $rsync_options_ = join(" ", @ARGV[3..$#ARGV]);
}

my $delete_word_ = ($delete_flag_==1) ? "--delete-after" : "";

# If there is a trailing '/', remove it. rsync behaves differently in that case
if ( $this_dirname_ =~ /\/$/ )
{
    $this_dirname_ = substr ( $this_dirname_, 0, length ( $this_dirname_ ) - 1 );
}
my $base_dirname_ = dirname ($this_dirname_);

if ( IsEc2Machine() ) {
#sync to other workers
  my @worker_machines_ = GetWorkerIPs();
#using dvctrader for ec2 irrespective of the input
  my $ec2_username_ = "dvctrader";
  foreach my $remote_machine_ ( @worker_machines_ )
  {
    `ssh $ec2_username_\@$remote_machine_ mkdir -p $this_dirname_`;
    `rsync -Lravz $delete_word_ $this_dirname_ $ec2_username_\@$remote_machine_:$base_dirname_`;
  }

#use ny11 to sync to other ny and prod servers
  my $ny11_ip = "10.23.74.51";
  `ssh $USER\@$ny11_ip mkdir -p $this_dirname_`;
  `rsync -Lravz $delete_word_ $rsync_options_ $this_dirname_ $USER\@$ny11_ip:$base_dirname_`;
  `ssh $USER\@$ny11_ip /home/$USER/$REPO/scripts/sync_dir_to_all_machines.pl $this_dirname_ $USER $delete_flag_ $rsync_options_`;
} 
else {
  my $HOSTNAMEIP=`hostname -i`; chomp($HOSTNAMEIP);
  my @all_machines_ = GetAllMachinesVec();

  print "Total Machines: ".@all_machines_."\n";
  foreach my $remote_machine_ ( @all_machines_ )
  {
    if ( $remote_machine_ ne $HOSTNAMEIP )
    {
      print "Syncing to ".$remote_machine_." ...\n";
      `ssh $USER\@$remote_machine_ mkdir -p $this_dirname_`;
      `rsync -Lravz $delete_word_ $rsync_options_ $this_dirname_ $USER\@$remote_machine_:$base_dirname_`;
    }
  }
}
1;
