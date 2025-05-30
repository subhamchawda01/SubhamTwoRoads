#!/usr/bin/perl
use strict;
use warnings;

while ( <> )
{
    my @words_ = split ( ' ', $_ );
    if ( $#words_ >= 1 )
    {
	my $mt1 = `stat -c%Y $words_[0]`;
	my $mt2 = `stat -c%Y $words_[1]`;
	if ( $mt1 > $mt2 )
	{
	    printf "diff -w %s %s\n", $words_[0], $words_[1];
	}
	else
	{
	    printf "diff -w %s %s\n", $words_[1], $words_[0];
	}
    }
}
