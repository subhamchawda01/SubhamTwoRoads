#!/usr/bin/perl

# \file ModelScripts/call_lasso.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# 

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0  filtered_reg_data_filename_ max_model_size_ regression_output_filename_ [indicator_list_filename_ (if checking for hash correlations)]";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $filtered_reg_data_filename_ = $ARGV[0];
my $max_model_size_ = $ARGV[1];
my $regression_output_filename_ = $ARGV[2];
my $exec_cmd_ = "";

if ( ExistsWithSize ( $filtered_reg_data_filename_ ) )
{
    # reducing size if required
    my $desired_sz_ = 700000000 ; #  700MB
    my $regdata_sz_ = -s $filtered_reg_data_filename_;
    my $downsample_ratio_ = int ( $regdata_sz_ / $desired_sz_ ) + 1;

    my $final_reg_data_filename_ = "";
    my $reduced_reg_data_filename_ = "";
    if( $downsample_ratio_ > 1 )
    {
	$reduced_reg_data_filename_ = $filtered_reg_data_filename_."_reduced";
	#my $exec_cmd_ = "cat $filtered_reg_data_filename_ |  awk -v r=$downsample_ratio_ '{ if ( ( NR % r) == 0 ) { print \$0 }}' > $reduced_reg_data_filename_" ;
	my $exec_cmd_ = "awk -v r=$downsample_ratio_ 'NR % r == 0' $filtered_reg_data_filename_ > $reduced_reg_data_filename_" ;
	`$exec_cmd_`;
	$final_reg_data_filename_ = $reduced_reg_data_filename_ ;
    }
    else
    {
	$final_reg_data_filename_ = $filtered_reg_data_filename_ ; # just aliasing for simplified use 
    }

    my $hash_correlations_filename_ = "";
    if( $#ARGV > 2 )
    {
	my $indicator_list_filename_ = $ARGV[3];
	$hash_correlations_filename_ = $indicator_list_filename_."_hash_correlations";
	$exec_cmd_ = "cat $indicator_list_filename_ | grep \"INDICATOR \" | awk -F '#' '{print \$2}' | awk '{if(NF<=1) {print \$1}else{print \$2}}' > $hash_correlations_filename_";
	`$exec_cmd_`;
	$exec_cmd_ = "$MODELSCRIPTS_DIR/build_lasso_model.R $final_reg_data_filename_ $max_model_size_ $regression_output_filename_ $hash_correlations_filename_"; 
    }
    else
    {
	$exec_cmd_ = "$MODELSCRIPTS_DIR/build_lasso_model.R $final_reg_data_filename_ $max_model_size_ $regression_output_filename_";
    }
    
    #print $exec_cmd_."\n";
    my @output_lines_ = `$exec_cmd_`;
    print "@output_lines_"."\n";

    if ( $reduced_reg_data_filename_ &&
	 ExistsWithSize ( $reduced_reg_data_filename_ ) )
    { 
	`rm -f $reduced_reg_data_filename_`;
    }

    if ( $hash_correlations_filename_ && 
	 ( -e $hash_correlations_filename_ ) ) 
    { `rm -f $hash_correlations_filename_`; }
}
