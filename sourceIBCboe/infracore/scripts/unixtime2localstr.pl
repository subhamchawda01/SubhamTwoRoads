#!/usr/bin/perl

use strict;
use warnings;

my $USAGE="$0 unixtime <1> ";


if ( $#ARGV < 0 ) { print $USAGE; exit ( 0 ); }
my $unixtime_ = $ARGV[0];
my $format_ = 0;


if ( scalar ( @ARGV ) > 1 )
{
    $format_ = $ARGV[1];
}

if ( $format_ == 0 )
{
    print scalar localtime($unixtime_)."\n";
}
elsif ( $format_ == 1 )
{
    my @datetime_ =  localtime ( $unixtime_ ) ;
    print sprintf ( "%02d%02d%02d %02d%02d%02d\n", $datetime_[ 5 ] + 1900, $datetime_[ 4 ] + 1, $datetime_[ 3 ], $datetime_[ 2 ], $datetime_[ 1 ], $datetime_[ 0 ] ) ; 

}
else
{
    print scalar localtime($unixtime_)."\n";
}

