#!/usr/bin/perl

use warnings;
use strict;

if ( $#ARGV < 0 )
{
    printf "usage: gmon_results_file\n";
    exit ( 0 );
}

my $results_file_name_ = $ARGV[0];

open ( RESULTS_FILE_HANDLE, "< $results_file_name_" ) or die " Could not open file $results_file_name_ for reading \n" ;

my $reading_act_lines_ = 0;
while ( my $inline_ = <RESULTS_FILE_HANDLE> )
{
    chomp ( $inline_ );
    if ( $reading_act_lines_ == 0 )
    {
	if ( $inline_ =~ "time   seconds   seconds    calls   s/call   s/call  name" )
	{
	    $reading_act_lines_ = 1;
	}
    }
    else
    {
	my @words_ = split ( ' ', $inline_ );
	if ( $#words_ >= 6 )
	{
	    if ( ( int ( $words_[3] ) > 0 ) &&
		 ( $words_[2] > 0 ) )
	    {
		$words_[4] = int ( (1000000000 * $words_[2])/$words_[3] );
#		$words_[5] = int ( (1000000000 * $words_[1])/$words_[3] );
		print join ( ' ', @words_ )."\n" ;
	    }
	}
	else
	{
	    if ( $#words_ <= 0 )
	    {
		last;
	    }
	}
    }
}

close ( RESULTS_FILE_HANDLE );
