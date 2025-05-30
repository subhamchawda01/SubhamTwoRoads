#!/usr/bin/perl
##
### \file ModelScripts/find_best_params_permute.pl
###
### \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
###  Address:
###       Suite No 162, Evoma, #14, Bhattarhalli,
###       Old Madras Road, Near Garden City College,
###       KR Puram, Bangalore 560049, India
###       +91 80 4190 3551
###
### This script takes :
##
#
#

use strict;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw(max); # for max
use FileHandle;


my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "diwakar" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";

my $TRADELOG_DIR="/spare/local/tradeinfo/volatilitylogs/";

my $USAGE = "$0 REG_DATA SPLIT_FILE OUT_FILE [ OUT_FILE1 ] [ OUT_FILE2 ] [OUT_FILE3]";

if ( $#ARGV < 2 )
{
  print $USAGE."\n";
  exit;
}
my $reg_data_file_ = $ARGV[0];
my $split_data_file_ = $ARGV[1]; #can also be found by shortcode and date but thats probally irrelevant
my $out_file_ = $ARGV[2];
my $out_file1_ = $out_file_."_1";
my $out_file2_ = $out_file_."_2";
my $out_file3_ = $out_file_."_3";

if ($#ARGV > 2)
{
  $out_file1_ = $ARGV[3];
}

if ( $#ARGV > 3 )
{
  $out_file2_ = $ARGV[4];
}

if ( $#ARGV > 4 )
{
  $out_file3_ = $ARGV[5];
}

open ( SPLIT_FILE, "<", $split_data_file_ ) or PrintStacktraceAndDie ( "Could not open $split_data_file_ to  read.\n");
open ( REG_DATA, "<", $reg_data_file_ ) or PrintStacktraceAndDie ( "Could not open $reg_data_file_ to read. \n");
open ( OUT_FILE, ">", $out_file_ ) or PrintStacktraceAndDie ( " Could not open $out_file_ to write. \n");
open ( OUT_FILE1, ">", $out_file1_ ) or PrintStacktraceAndDie ( " Could not open $out_file1_ to write.\n");
open ( OUT_FILE2, ">", $out_file2_ ) or PrintStacktraceAndDie ( " Could not open $out_file2_ to write.\n");
open ( OUT_FILE3, ">", $out_file3_ ) or PrintStacktraceAndDie ( " Could not open $out_file3_ to write.\n");

my $start_interval_msec_ = 0;
my $end_interval_msec_ = 0;
my $high_dependent_volume_ = "";
my $high_core_shortcode_volume_ = "";
my $dependent_trend_ = "";
while ( my $reg_line_ = <REG_DATA> )
{
  my @reg_data_words_ = split (' ', $reg_line_ );
  my $this_mfm_ = $reg_data_words_[$#reg_data_words_];
  if (  $this_mfm_ > $end_interval_msec_ )
  {
#read the next tag
    $high_dependent_volume_ = "";
    $high_core_shortcode_volume_ = "";
    $dependent_trend_ = "";

    while (my $split_line_ = <SPLIT_FILE> )
    {
      my @split_line_words_ = split(' ', $split_line_ );
      $start_interval_msec_ = ( $split_line_words_[0] % 86400 ) * 1000;
      $end_interval_msec_ = ( $split_line_words_[1] % 86400 ) * 1000;
      if ( $end_interval_msec_ < $this_mfm_ )
      {
        next;
      }
      if ( $split_line_words_[2] ne "0" )
      {
        $high_dependent_volume_ = $split_line_words_[2];
      }
      if ( $split_line_words_[3] ne "0" )
      {
        $high_core_shortcode_volume_ = $split_line_words_[3];
      }
        
      if ( $split_line_words_[4] ne "0" )
      {
        $dependent_trend_ = $split_line_words_[4];
      }
      last;
    }
  }
  else
  {
    if ( $high_dependent_volume_ )
    {
      print OUT_FILE join( ' ', @reg_data_words_[0..$#reg_data_words_-1] )."\n";
    }
    if ( $high_core_shortcode_volume_ )
    {
      print OUT_FILE1 join( ' ', @reg_data_words_[0..$#reg_data_words_-1] )."\n";
    }
    if ( $dependent_trend_ )
    {
      print OUT_FILE2 join( ' ', @reg_data_words_[0..$#reg_data_words_-1] )."\n";
    }
    if ( not ( $high_dependent_volume_ || $high_core_shortcode_volume_ || $dependent_trend_ ) ) 
    {
      print OUT_FILE3 join( ' ', @reg_data_words_[0..$#reg_data_words_-1] )."\n";
    }
  }
}
