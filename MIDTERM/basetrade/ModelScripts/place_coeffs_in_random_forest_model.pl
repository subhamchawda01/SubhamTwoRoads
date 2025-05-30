#!/usr/bin/perl

# \file ModelScripts/place_coeffs_in_random_forest_model.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite 217, Level 2, Prestige Omega,
#	 No 104, EPIP Zone, Whitefield,
#	 Bangalore - 560066, India
#	 +91 80 4060 0717
#
# This script takes :
# output_model_filename_  indicator_list_filename_  regression_output_filename_

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_numeric_value_from_line.pl"; # GetNumericValueFromLine ;

# start 
my $USAGE="$0  output_model_filename  indicator_list_filename  regression_output_filename";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $output_model_filename_ = $ARGV[0];
my $indicator_list_filename_ = $ARGV[1];
my $regression_output_filename_ = $ARGV[2];

my $ilist_pre_indicator_text_="";
my $ilist_post_indicator_text_="";
my @indicator_trailing_text_=();

open ILIST, "< $indicator_list_filename_" or PrintStacktraceAndDie ( "Could not open indicator_list_filename_ $indicator_list_filename_ for reading\n" );
my $mode_ = -1;
while ( my $iline_ = <ILIST> )
{
    if ( $mode_ eq -1 ) 
    {
	$ilist_pre_indicator_text_ .= $iline_;

	# check if mode changing to 0 ... that in indicator reading mode
	my @iwords_ = split ' ', $iline_;
	if ( ( $#iwords_ >= 0 ) &&
	     ( $iwords_[0] eq "INDICATORSTART" ) )
	{
	    $mode_ = 0;
	}
    }
    elsif ( $mode_ eq 0 )
    {
	# process indicator line
	my @iwords_ = split ' ', $iline_;
	if ( ( $#iwords_ >= 0 ) &&
	     ( $iwords_[0] eq "INDICATOR" ) )
	{
	    splice ( @iwords_, 0, 2 );
	    push ( @indicator_trailing_text_, join ( ' ', @iwords_ ) );
	}
	else
	{
	    $mode_ = 1;
	    $ilist_post_indicator_text_ .= $iline_;
	}
    }
    elsif ( $mode_ eq 1 )
    {
	$ilist_post_indicator_text_ .= $iline_;
    }
}
close ILIST;



my @regression_coefficients_=();
open REGOUTFILE, "< $regression_output_filename_" or PrintStacktraceAndDie ( "Could not open regression_output_filename_ $regression_output_filename_ for reading\n" );
my $model_size_= 0;
my $tree_text_ = "";
my $num_trees_ = 0;
my $max_nodes_ = 0;
while ( my $regline_ = <REGOUTFILE> )
{
    my @regwords_ = split ' ', $regline_;
    my ($this_index_ )= -1 ;

    if ( ( $#regwords_ >=1 ) &&
         ( $regwords_[0] eq "MODELARGS" ) )
    {
	$num_trees_ = int($regwords_[1]);
	$max_nodes_ = int($regwords_[2]);
    }   
    elsif ( ( $#regwords_ >= 1 ) &&
	 ( $regwords_[0] eq "INDICATOR" ) )
    {
	$this_index_ = int($regwords_[1]);
	
    	my $outline_ = "INDICATOR 1.00 ".$indicator_trailing_text_[$this_index_]."\n";
	$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;
	$model_size_++;	
    }
    else
    {
	chomp ($regline_);
	$tree_text_ = $tree_text_.$regline_."\n";
    }
}

chomp($ilist_post_indicator_text_);

$tree_text_ = $tree_text_."RANDOMFORESTEND\n";

close REGOUTFILE;

if ( $model_size_ > 0 ) {
    open OUTMODEL, "> $output_model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $output_model_filename_ for writing\n" );
    print OUTMODEL $ilist_pre_indicator_text_;
    print OUTMODEL $ilist_post_indicator_text_."\n";
    print OUTMODEL $tree_text_;
    close OUTMODEL;
    `sed -i 's/LINEAR/RANDOMFOREST/g' $output_model_filename_`;
    `sed -i 's/INDICATORSTART/MODELARGS $num_trees_ $max_nodes_\\nINDICATORSTART/g' $output_model_filename_`;
}
else {
    print "-1" ;
}

exit ( 0 );

