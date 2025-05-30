#!/usr/bin/perl

# \file ModelScripts/print_highly_correlated_indicator_pairs.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes as input :
# indicator_list_file output_of_get_dep_corr

use strict;
use warnings;

package CorrLine;
use Class::Struct;
use Data::Dumper ;

# declare the struct
struct ( 'CorrLine', { corr_value_ => '$', indicator_text_ => '$' } );

package main;

#my $SPARE_DIR="/spare/local/basetrade/";
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE="$0 indicator_list_filename corrmatrix_filename";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }
my $ilf = $ARGV[0];
my $cmf = $ARGV[1];

my @indicator_text_vec_ = ();
my @indicator_corr_vec_ = ();
my @indicator_corrline_vec_ = ();

open ILF, "< $ilf" or PrintStacktraceAndDie ( "print_highly_correlated_indicator_pairs.pl could not open indicator_list_filename $ilf\n" );

push ( @indicator_text_vec_, "DEPENDANT" ) ;
while ( my $ilf_line_ = <ILF> ) 
{
    chomp ( $ilf_line_ );
    my @ilf_words_ = split ( ' ', $ilf_line_, 3 );
    if ( ( $#ilf_words_ >= 0 ) &&
	 ( $ilf_words_[0] eq "INDICATOR" ) )
    {
	push ( @indicator_text_vec_, $ilf_words_[2] ) ;
    }
}
close ILF;

open CMF, "< $cmf" or PrintStacktraceAndDie ( "print_highly_correlated_indicator_pairs.pl could not open corrmatrix_filename $cmf\n" );
my $first_indep_index_ = 0;
while ( my $cmf_line_ = <CMF> ) 
{
    chomp ( $cmf_line_ );
    my @cmf_words_ = split ( ' ', $cmf_line_ );
    if ( $#cmf_words_ >= $#indicator_text_vec_ )
    {
	for ( my $second_indep_index_ = 0 ; $second_indep_index_ <= $#cmf_words_; $second_indep_index_ ++ )
	{
	    if ( $first_indep_index_ != $second_indep_index_ )
	    {
		if ( abs ( $cmf_words_[$second_indep_index_] ) >= 0.98 )
		{
		    printf "HCP: %s %s\n", $indicator_text_vec_[$first_indep_index_], $indicator_text_vec_[$second_indep_index_] ;
		}
	    }
	}
	$first_indep_index_ ++;
    }
}
close CMF;
