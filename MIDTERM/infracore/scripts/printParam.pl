#!/usr/bin/perl

use warnings;
use strict;

if ( $#ARGV < 0 )
{
    printf "usage: cmd file\n";
    exit ( 0 );
}

my $SFILE=$ARGV[0];
my $PFILE_="";

open ( SFILE_HANDLE, "< $SFILE" ) or die " Could not open file $SFILE for reading \n" ;
while ( my $sline_ = <SFILE_HANDLE> )
{
    my @words_ = split ( ' ', $sline_ );
    if ( $#words_ >= 4 )
    {
	$PFILE_=$words_[4];
	last;
    }
}

close ( SFILE_HANDLE );

print "$PFILE_\n";
