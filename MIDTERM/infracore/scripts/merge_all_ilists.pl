#!/usr/bin/perl

# \file scripts/merge_three_ilists.pl
#
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 162, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes as input :
# key file_with_columns

use strict;
use warnings;

sub ProcLine ;

#my $SPARE_DIR="/spare/local/infracore/";
my $HOME_DIR=$ENV{'HOME'}; 
my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";

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
    open FILEH, "< $filevec_[$i]" or die "$0 could not open $filevec_[$i]\n";

    while ( my $txt_line_ = <FILEH> )
      {
	chomp ( $txt_line_ );
	$c1 += ProcLine ( $txt_line_ );
    if ( $c1 >= $num1 )
    {
	last;
    }
}

close FILE1H;
open FILE2H, "< $file2_" or die "$0 could not open $file2_\n";

while ( my $txt_line_ = <FILE2H> )
{
    chomp ( $txt_line_ );
    $c2 += ProcLine ( $txt_line_ );
    if ( $c2 >= $num2 )
    {
	last;
    }
}

close FILE2H;
open FILE3H, "< $file3_" or die "$0 could not open $file3_\n";

while ( my $txt_line_ = <FILE3H> )
{
    chomp ( $txt_line_ );
    $c3 += ProcLine ( $txt_line_ );
    if ( $c3 >= $num3 )
    {
	last;
    }
}

close FILE3H;

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
		return 1;
	    }
	}
    }
    return 0;
}
