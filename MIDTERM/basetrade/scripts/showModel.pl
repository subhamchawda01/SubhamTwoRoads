#!/usr/bin/perl

use warnings;
use strict;

if ( $#ARGV < 0 )
{
    printf "usage: cmd file\n";
    exit ( 0 );
}

my $SFILE=$ARGV[0];
my $MFILE_="";

my $filesize = -s "$SFILE" ;

if ( $filesize < 4 )
 {
   exit(0);
 }

open ( SFILE_HANDLE, "< $SFILE" ) or die " Could not open file $SFILE for reading \n" ;
while ( my $sline_ = <SFILE_HANDLE> )
{
    my @words_ = split ( ' ', $sline_ );
    if ( $#words_ >= 3 )
    {
	$MFILE_=$words_[3];
	last;
    }
}

close ( SFILE_HANDLE );
system ( "cat $MFILE_" );
