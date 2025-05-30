#!/usr/bin/perl

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname

if ( $#ARGV != 1 ) { print "EXEC INFILE OUTFILE"; exit ( 0 ); }
my $filename = $ARGV[0];
my $outfile_ = $ARGV[1];

#open the file

open PCA_PORT_PRICE, "< $filename " or die "Could not open the infile $filename\n";

open PRICE_OUT_FILE, "> $outfile_ " or die "Could not open the outfile $outfile_\n";

my %time_to_price_map = ();

while ( my $thisline_ = <PCA_PORT_PRICE> ) 
{
    my @this_words_ = split (  ' ', $thisline_ );
    if ( $this_words_[1] eq "PCA" ) 
    {
	$time_to_price_map{$this_words_[0]} = $this_words_[2];	
    }

    if ( $this_words_[1] eq "NOR" ) 
    {
	if ( exists  $time_to_price_map{$this_words_[0]} )
	{
#	    printf PRICE_OUT_FILE "%s  %f  %f \n", $this_words_[0], $time_to_price_map{$this_words_[0]}, $this_words_[2] ;
	    printf PRICE_OUT_FILE " %f  %f \n", $time_to_price_map{$this_words_[0]}, $this_words_[2] ;
	}
    }
}
close ( PCA_PORT_PRICE );
close ( PCA_OUT_FILE );
