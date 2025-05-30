#!/usr/bin/perl

my %indicator_lines_;

while ( <> )
{
    chomp;
    my @words_=split( ' ', $_ );
    if ( $#words_ >= 2 )
    {
	my $iname_ = $words_[2];
	if ( ! exists ( $indicator_lines_{$iname_} ) )
	{
	    $indicator_lines_{$iname_}=$_;
	}
    }
}

foreach $iname_ ( keys %indicator_lines_ )
{
    print $indicator_lines_{$iname_}."\n";
}
