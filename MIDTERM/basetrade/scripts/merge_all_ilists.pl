#!/usr/bin/perl

# \file scripts/merge_three_ilists.pl
#
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#

use strict;
use warnings;

sub ProcLine ;

#my $SPARE_DIR="/spare/local/basetrade/";
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE="$0 file1 file2 ... ";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my @filevec_ = ();
for ( my $i = 0 ; $i <= $#ARGV; $i ++ )
{
    if ( -e $ARGV[$i] )
    {
	push ( @filevec_, $ARGV[$i] );
    }
}

my %iline_acorr_map_ ;

for ( my $i = 0 ; $i <= $#filevec_ ; $i ++ )
{
    open FILEH, "< $filevec_[$i]" or PrintStacktraceAndDie ( "$0 could not open $filevec_[$i]\n" );
    
    while ( my $txt_line_ = <FILEH> )
    {
	chomp ( $txt_line_ );
	ProcLine ( $txt_line_ );
    }
    
    close FILEH;
}

exit ( 0 );

sub ProcLine
{
    my $txt_line_ = shift;
    if ( $txt_line_ =~ /INDICATOR / )
    {
	my @txt_words_ = split ( '#', $txt_line_ );
	if ( $#txt_words_ >= 1 )
	{
	    my $iline_ = $txt_words_[0];
	    my $acorr_ = $txt_words_[1];
	    if ( ! ( exists $iline_acorr_map_{$iline_} ) )
	    { # not seen yet
		$iline_acorr_map_{$iline_} = $acorr_ ;
		printf "%s#%s\n", $iline_, $acorr_ ; 
	    }
	}
    }
}
