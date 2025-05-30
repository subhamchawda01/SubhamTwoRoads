#!/usr/bin/perl
#
# It provides functions for Pool Performance, Similarity-Metrics and other highlights
# Note: For fetching pool-configs we use: walkforward/get_pool_configs.py
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use Math::Complex; # sqrt
use FileHandle;
use POSIX;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $MODELING_BASE_DIR = $HOME_DIR."/modelling";
my $MODELING_STRATS_DIR = $MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
my $WF_SCRIPTS_DIR = $HOME_DIR."/".$REPO."/walkforward"; 

sub GetPoolsForShortcode
{
  my $shortcode = shift;
  my $normal_or_staged_ = shift || 'N';
  my $is_ebt = shift || 0;

  my $fetch_cmd_ = "$WF_SCRIPTS_DIR/get_pools_for_shortcode.py -shc $shortcode -type $normal_or_staged_ -ebt $is_ebt";
  my @pools_ = `$fetch_cmd_ 2>/dev/null`; chomp ( @pools_ );
  return @pools_;
}

sub GetConfigsForPool
{
  my $shortcode = shift;
  my $tperiod = shift;
  my $normal_or_staged_ = shift || 'N';
  my $evtoken = shift || "INVALID";

  my $fetch_cmd_ = "$WF_SCRIPTS_DIR/get_pool_configs.py -shc $shortcode -type $normal_or_staged_ -tp $tperiod -m POOL";
  if ( defined $evtoken and $evtoken ne "INVALID" ) {
    $fetch_cmd_ .= " -evtok $evtoken";
  }

  my @configs = `$fetch_cmd_ 2>/dev/null`; chomp ( @configs );
  return @configs;
}

1
