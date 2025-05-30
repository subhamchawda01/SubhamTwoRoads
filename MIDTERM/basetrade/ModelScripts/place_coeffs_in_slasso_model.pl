#!/usr/bin/perl

# \file ModelScripts/place_coeffs_in_model.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
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
	elsif ($iwords_[0] eq "INDICATOREND" )
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


# process regression_output_file

my @regression_coefficients_=();
open REGOUTFILE, "< $regression_output_filename_" or PrintStacktraceAndDie ( "Could not open regression_output_filename_ $regression_output_filename_ for reading\n" );
while ( my $regline_ = <REGOUTFILE> )
{	
    my @regwords_ = split ' ', $regline_;
    my $this_index_ = int($regwords_[0])-1;
    my $this_coeff_ = $regwords_[1];
    if($this_index_ >= 0)	
    {
	my $outline_ = "INDICATOR ".$this_coeff_." ".$indicator_trailing_text_[$this_index_]."\n" ;
	$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;
    }
}

close REGOUTFILE;
#`rm -f $regression_output_filename_`;
open OUTMODEL, "> $output_model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $output_model_filename_ for writing\n" );
print OUTMODEL $ilist_pre_indicator_text_;
print OUTMODEL $ilist_post_indicator_text_;
close OUTMODEL;
