#!/usr/bin/perl

# \file ModelScripts/sort_by_corr.pl
#
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
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

my $USAGE="$0 indicator_list_filename dep_correlation_filename";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }
my $ilf = $ARGV[0];
my $dcf = $ARGV[1];

my @indicator_text_vec_ = ();
my @indicator_corr_vec_ = ();
my @indicator_corrline_vec_ = ();

open ILF, "< $ilf" or PrintStacktraceAndDie ( "sort_by_corr.pl could not open indicator_list_filename $ilf\n" );
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

open DCF, "< $dcf" or PrintStacktraceAndDie ( "sort_by_corr.pl could not open dep_corr_output_filename $dcf\n" );
while ( my $dcf_line_ = <DCF> ) 
{
    chomp ( $dcf_line_ );
    my @dcf_words_ = split ( ' ', $dcf_line_ );
    if ( $#dcf_words_ >= $#indicator_text_vec_ )
    {
	@indicator_corr_vec_ = @dcf_words_;
	for ( my $indep_index_ = 0 ; $indep_index_ <= $#indicator_text_vec_; $indep_index_ ++ )
	{
	    my $new_corr_line_ = new CorrLine;
	    $new_corr_line_->corr_value_ ( $indicator_corr_vec_[$indep_index_] );
	    $new_corr_line_->indicator_text_ ( $indicator_text_vec_[$indep_index_] );
	    push ( @indicator_corrline_vec_, $new_corr_line_ );
	}
	last;
    }
}
close DCF;

sub CLCmp ($$) 
{
    my($cl1, $cl2) = @_;
    return abs($cl2->corr_value_()) <=> abs($cl1->corr_value_());
}

my @sorted_indicator_corrline_vec_ = sort CLCmp @indicator_corrline_vec_;

# print Dumper ( @sorted_indicator_corrline_vec_ ); 
# not working for some reason
# replaced with code written below
for ( my $i = 0 ; $i <= $#sorted_indicator_corrline_vec_; $i++ )
{
    printf "%.3f %s\n", $sorted_indicator_corrline_vec_[$i]->corr_value_(), $sorted_indicator_corrline_vec_[$i]->indicator_text_();
}

