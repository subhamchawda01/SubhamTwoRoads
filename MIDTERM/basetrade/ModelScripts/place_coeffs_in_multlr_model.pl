#!/usr/bin/perl

# \file ModelScripts/place_coeffs_in_siglr_model.pl
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
my ($model_corr_, $model_stdev_, $model_rsquared_, $dep_stdev_, $residual_stdev_, $model_mse_ , $model_domination_ )=(-1.0, -1.0, -1.0, -1.0 ,-1.0, -1.0, -1.0);
my $model_size_= 0;
while ( my $regline_ = <REGOUTFILE> )
{
    my @regwords_ = split ' ', $regline_;
    my ($this_index_, $this_coeff_ )=(-1,-1,-1);
    if ( ( $#regwords_ >= 2 ) &&
	 ( $regwords_[0] eq "OutCoeff" ) )
    {
	$this_index_ = int($regwords_[1]);
	$this_coeff_ = $regwords_[2];
	
	if ( $this_coeff_ != 0 )
	{
	   my $outline_ = "INDICATOR $this_coeff_ ".$indicator_trailing_text_[$this_index_]."\n";
	   $ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;
	   $model_size_++;
	}
    }
    elsif ( $regwords_[0] eq "RSquared" )    { $model_rsquared_ = $regwords_[1]; }
    elsif ( $regwords_[0] eq "Correlation" ) { $model_corr_     = $regwords_[1]; }
    elsif ( $regwords_[0] eq "StdevResidual" ) { $dep_stdev_ = $regwords_[1]; }
    elsif ( $regwords_[0] eq "StdevModel" )  { $model_stdev_    = $regwords_[1]; }
    elsif ( $regwords_[0] eq "StdevDependent" ) { $residual_stdev_ = $regwords_[1]; }
    elsif ( $regwords_[0] eq "MSE" ) { $model_mse_ = $regwords_[1]; }
    elsif ( $regwords_[0] eq "Domination" ) { $model_domination_ = $regwords_[1]; }
}

chomp($ilist_post_indicator_text_);
$ilist_post_indicator_text_ .= " # Corr: $model_corr_ StDev: $model_stdev_ RS: $model_rsquared_ Domination : $model_domination_\n";

close REGOUTFILE;

if ( $model_size_ > 0 ) {
    open OUTMODEL, "> $output_model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $output_model_filename_ for writing\n" );
    print OUTMODEL $ilist_pre_indicator_text_;
    print OUTMODEL $ilist_post_indicator_text_;
    close OUTMODEL;
}
else {
    print "-1" ;
}

exit ( 0 );

