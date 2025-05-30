#!/usr/bin/perl

use strict;
use warnings;
use Fcntl qw (:flock);
use File::Basename; # for basename and dirname
use Digest::MD5 qw(md5 md5_hex md5_base64);

my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( ! ( -d  $LIVE_BIN_DIR ) && ! ( -e $LIVE_BIN_DIR ) )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/create_enclosing_directory.pl";
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $LOCK_DIR_ = $HOME_DIR."/locks";
my $hostname_ = `hostname`;
if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
{     
  $LOCK_DIR_ = "/mnt/sdf/locks";
}

`mkdir -p $LOCK_DIR_`;
my %created_locks_ = ();

sub GetLockFileName
{
  my ($file_) = @_;
  my $basename_=`basename $file_`; chomp($basename_); $basename_ = substr($basename_, 0, 40); #shortening for safety
  my $lock_file_ = $LOCK_DIR_."/".$basename_."_".md5_hex($file_)."_virtuallock_dont_delete_this";
  return $lock_file_;
}

sub TakeLock
{
  my $file_ = shift;
  my $lock_type_ = "EX_B"; #by default take LOCK_EX with blocking call
  if ( $#_ >= 0 )
  {
    $lock_type_ = shift;
  }
  return if ( !defined $file_ );
  my $lock_file_ = GetLockFileName($file_);
  CreateEnclosingDirectory($lock_file_); #just for safety
  open ( my $lock_fh_, ">", $lock_file_ );
  my $success_ = 0;
  if ( $lock_type_ eq "EX_NB" )
  {
    $success_ = flock ( $lock_fh_ , LOCK_EX|LOCK_NB );
  }
  else
  {
    $success_ = flock ( $lock_fh_ , LOCK_EX );
  }
  $created_locks_{$lock_file_} = $lock_fh_;
  return $success_;
}

sub RemoveLock
{
  my ($file_) = @_;
  return if ( !defined $file_ );
  my $lock_file_ = GetLockFileName($file_);
  if (exists($created_locks_{$lock_file_}))
  {
    my $t_fh_ = $created_locks_{$lock_file_};
    delete($created_locks_{$lock_file_});
    close($t_fh_);              
  }
}

sub END
{
  foreach my $lockfile_ ( keys %created_locks_ )        
  {                
    close($created_locks_{$lockfile_});              
#print STDERR "unlocking leftover lock for $lockfile_\n";                                    
  }
}

1;
