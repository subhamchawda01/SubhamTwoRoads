#!/usr/bin/perl

# \file ModelScripts/apply_dep_filter.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This scipt takes 2 arguments: an input file and a filter name
# if the filter name is valid, it creates an output filtered file and prints to console the name of the output file

use strict;
use warnings;
use feature "switch";          # for given, when
use File::Basename;            # for basename and dirname
use File::Copy;                # for copy
use List::Util qw/max min/;    # for max
use FileHandle;


my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";
my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if($#ARGV <2) {
  print "usage: short_code reg_file filter_name [filtered_reg_file/-1] [date] \n";
  exit ( 0 );
}

my $shortcode_ = $ARGV[0];
my $this_reg_data_filename_ = $ARGV[1];
my $this_filter_ = $ARGV[2];

my $this_filtered_reg_data_filename_ = $this_reg_data_filename_."_".$this_filter_;
if ( $#ARGV > 2 && $ARGV[3] ne "-1" ) {
  $this_filtered_reg_data_filename_ = $ARGV[3];
}

my $yyyymmdd =`date '+ %Y%m%d'`; chomp($yyyymmdd);
if ( $#ARGV > 3 ) {
  $yyyymmdd = $ARGV[4];
}

my $exec_cmd = "";
given ($this_filter_) 
{
  when ( "f0" ) {    # do nothing
    copy( $this_reg_data_filename_, $this_filtered_reg_data_filename_ );
  }
  when ( "fv" ) {
    copy( $this_reg_data_filename_, $this_filtered_reg_data_filename_ );
  }
  when ( /^fsudm/ ) {
    copy( $this_reg_data_filename_, $this_filtered_reg_data_filename_ );
  }
  when ( /^fst/ ) {
    my $min_price_increment_ = `$LIVE_BIN_DIR/get_min_price_increment $shortcode_ $yyyymmdd` ; 
    chomp ( $min_price_increment_ );

    my $min_value_to_filter_ = $this_filter_;
    $min_value_to_filter_ =~ s/fst//;
    $min_value_to_filter_ *= $min_price_increment_;
    $exec_cmd = "$MODELSCRIPTS_DIR/select_rows_with_dep_in_range_2.pl $this_reg_data_filename_ 0 $min_value_to_filter_ -1 $this_filtered_reg_data_filename_";
  }
  when ( /^fsl/ ) {
    my $max_value_to_filter_ = $this_filter_;
    $max_value_to_filter_ =~ s/fsl//;
    $exec_cmd = "$MODELSCRIPTS_DIR/select_rows_with_dep_in_range_2.pl $this_reg_data_filename_ 1 -1 $max_value_to_filter_ $this_filtered_reg_data_filename_";
  }  
  when ( /^fsg/ ) {
    my $min_value_to_filter_ = $this_filter_;
    $min_value_to_filter_ =~ s/fsg//;
    $exec_cmd = "$MODELSCRIPTS_DIR/select_rows_with_dep_in_range_2.pl $this_reg_data_filename_ 1 $min_value_to_filter_ -1 $this_filtered_reg_data_filename_";
  }  
  when ( /^fsr/ ) {
    my $range_to_filter_ = $this_filter_;
    $range_to_filter_ =~ s/fsr//;
    my ($min_value_to_filter_, $max_value_to_filter_) = split('_', $range_to_filter_);
    $exec_cmd = "$MODELSCRIPTS_DIR/select_rows_with_dep_in_range_2.pl $this_reg_data_filename_ 1 $min_value_to_filter_ $max_value_to_filter_ $this_filtered_reg_data_filename_";
  } 
  when ( /^fnz/ ) {
    my $min_value_to_filter_ = $this_filter_;
    $min_value_to_filter_ =~ s/fsz//;
    $exec_cmd = "$MODELSCRIPTS_DIR/select_rows_with_indep_nonzero.pl $this_reg_data_filename_ $this_filtered_reg_data_filename_ $min_value_to_filter_";
  }
  when ( "flogit" ) {
    $exec_cmd = "$MODELSCRIPTS_DIR/logit_transform.py $this_reg_data_filename_ 2 $this_filtered_reg_data_filename_";
  }
  when ( /^old_fsg/ ) {
    my $min_value_to_filter_ = $this_filter_;
    $min_value_to_filter_ =~ s/old_fsg//;
    $exec_cmd = "$MODELSCRIPTS_DIR/select_rows_with_dep_exceeding_min.pl $this_reg_data_filename_ $min_value_to_filter_ $this_filtered_reg_data_filename_";
  }
  when ( /^old_fsl/ ) {
    my $max_value_to_filter_ = $this_filter_;
    $max_value_to_filter_ =~ s/old_fsl//;
    $exec_cmd = "$MODELSCRIPTS_DIR/select_rows_with_dep_lt_max.pl $this_reg_data_filename_ $max_value_to_filter_ $this_filtered_reg_data_filename_";
  }
  when ( /^old_fsr/ ) {
    my $range_to_filter_ = $this_filter_;
    $range_to_filter_ =~ s/old_fsr//;
    my ($min_value_to_filter_, $max_value_to_filter_) = split('_', $range_to_filter_);
    $exec_cmd = "$MODELSCRIPTS_DIR/select_rows_with_dep_in_range.pl $this_reg_data_filename_ $min_value_to_filter_ $max_value_to_filter_ $this_filtered_reg_data_filename_";
  }
  default {
    print "ERROR: INVALID_FILTER\n";
  }
}

if ( $exec_cmd ne "" ) {
  print "executing: $exec_cmd\n" ;
  `$exec_cmd`;
}
