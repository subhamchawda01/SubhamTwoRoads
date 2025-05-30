#!/usr/bin/perl

# \file ModelScripts/call_pca_reg.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite 217, Level 2, Prestige Omega,
#	 No 104, EPIP Zone, Whitefield,
#	 Bangalore - 560066, India
#	 +91 80 4060 0717
#
# 

use strict;
use warnings;
use File::Basename; # for basename and dirname

use POSIX; # for ceil

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $SPARE_HOME="/spare/local/".$USER."/";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin"; 

if ( $USER ne "dvctrader" )
{
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0  filtered_reg_data_filename_ num_pca_components max_model_size pca_heuristic min_correlation regression_output_filename_";

if ( $#ARGV <= 3 ) { print $USAGE."\n"; exit ( 0 ); }
my $filtered_reg_data_filename_ = $ARGV[0];
my $num_pca_components_ = $ARGV[1];
my $max_model_size_ = $ARGV[2];
my $min_correlation_ = $ARGV[4];
my $regression_output_filename_ = $ARGV[5];
my $pca_heuristic_ = $ARGV[3];

my $pca_weights_filename_ = $regression_output_filename_."_pca_weights";
my $temp_pca_tranformed_data_file_dir_ = $SPARE_HOME."transdir/"; 
`mkdir -p $temp_pca_tranformed_data_file_dir_`;
my $new_reg_data_filename_ = $temp_pca_tranformed_data_file_dir_.basename($filtered_reg_data_filename_)."_transformed";


my $desired_sz_ = 1000000000 ; #  1000MB
my $regdata_sz_ = -s $filtered_reg_data_filename_;
my $downsample_ratio_ = ceil ( $regdata_sz_ / $desired_sz_ );

my $final_reg_data_filename_ = "";
my $reduced_reg_data_filename_ = "";
if( $downsample_ratio_ > 1 )
{
    $reduced_reg_data_filename_ = $filtered_reg_data_filename_."_reduced";
    my $exec_cmd_ = "cat $filtered_reg_data_filename_ |  awk -v r=$downsample_ratio_ '{ if ( ( NR % r) == 0 ) { print \$0 }}' > $reduced_reg_data_filename_" ;
    `$exec_cmd_`;
    $final_reg_data_filename_ = $reduced_reg_data_filename_ ;
}
else
{
    $final_reg_data_filename_ = $filtered_reg_data_filename_ ; # just aliasing for simplified use 
}


{
  my $exec_cmd_ = "$MODELSCRIPTS_DIR/find_pca_comps.R $final_reg_data_filename_ $new_reg_data_filename_ $pca_weights_filename_";
  print $exec_cmd_."\n";
  my @output_lines_ = `$exec_cmd_`;
  print "@output_lines_";
}

if ( $reduced_reg_data_filename_ &&
     ExistsWithSize ( $reduced_reg_data_filename_ ) )
{ 
    `rm -f $reduced_reg_data_filename_`;
}


{
  my $exec_cmd_ = "$LIVE_BIN_DIR/callPCAREG $new_reg_data_filename_ $pca_weights_filename_ $min_correlation_ $regression_output_filename_ $max_model_size_ $num_pca_components_"; 
  print $exec_cmd_."\n";
  my @output_lines_ = `$exec_cmd_`;
  print "@output_lines_";
}

`rm -f $new_reg_data_filename_`;
`rm -f $pca_weights_filename_`;
