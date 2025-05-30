#!/usr/bin/perl
use Time::Local;

if ( scalar ( @ARGV ) != 4 ) 
{
    print "$0 yyyymmdd hhmmss fromtz totz \n" ;
    exit ( 0 ) ;
}
my $year_ = substr $ARGV [ 0 ] , 0 , 4 ;
my $month_ = substr $ARGV [ 0 ] , 4 , 2 ;
my $day_ = substr $ARGV [ 0 ] , 6 , 2 ;
my $hours_ = substr $ARGV [ 1 ] , 0 , 2 ;
my $min_ = substr $ARGV [ 1 ] , 2 , 2 ;
my $sec_ = substr $ARGV [ 1 ] , 4 , 2  ;

my $fromtz_ = $ARGV [ 2 ] ;
my $totz_ = $ARGV [ 3 ] ;

$month_ = $month_ - 1 ;

my $time_ = timelocal ( $sec_, $min_, $hours_, $day_, $month_, $year_ ) ;

if ( $fromtz_ eq "TOR" || $fromtz_ eq "EST" )
{
    $ENV{TZ} = 'America/Toronto';
    $time_ = timelocal ( $sec_, $min_, $hours_, $day_, $month_, $year_ ) ;
}
elsif ( $fromtz_ eq "NY" || $fromtz_ eq "EST" )
{
    $ENV{TZ} = 'America/New_York' ;
    $time_ = timelocal ( $sec_, $min_, $hours_, $day_, $month_, $year_ ) ;
}
elsif ( $fromtz_ eq "CHI" || $fromtz_ eq "CST" )
{
    $ENV{TZ} = 'America/Chicago' ;
    $time_ = timelocal ( $sec_, $min_, $hours_, $day_, $month_, $year_ ) ;
}
elsif ( $fromtz_ eq "BRZ" || $fromtz_ eq "BRT" )
{
    $ENV{TZ} = 'America/Sao_Paulo' ;
    $time_ = timelocal ( $sec_, $min_, $hours_, $day_, $month_, $year_ ) ;
}
elsif ( $fromtz_ eq "BSL" || $fromtz_ eq "BST" || $fromtz_ eq "LDN" )
{
    $ENV{TZ} = 'Europe/London' ;
    $time_ = timelocal ( $sec_, $min_, $hours_, $day_, $month_, $year_ ) ;
}
elsif ( $fromtz_ eq "FR" || $fromtz_ eq "CET" )
{
    $ENV{TZ} = 'Europe/Berlin' ;
    $time_ = timelocal ( $sec_, $min_, $hours_, $day_, $month_, $year_ ) ;
}
elsif ( $fromtz_ eq "IND" || $fromtz_ eq "IST" )
{
    $ENV{TZ} = 'Asia/Kolkata' ;
    $time_ = timelocal ( $sec_, $min_, $hours_, $day_, $month_, $year_ ) ;
}
elsif ( $fromtz_ eq "HK" || $fromtz_ eq "HKT" )
{
    $ENV{TZ} = 'Asia/Hong_Kong' ;
    $time_ = timelocal ( $sec_, $min_, $hours_, $day_, $month_, $year_ ) ;
}
elsif ( $fromtz_ eq "TOK" || $fromtz_ eq "JST" )
{
    $ENV{TZ} = 'Asia/Tokyo' ;
    $time_ = timelocal ( $sec_, $min_, $hours_, $day_, $month_, $year_ ) ;
}
elsif ( $fromtz_ eq "SYD" || $fromtz_ eq "AST" )
{
    $ENV{TZ} = 'Australia/Sydney' ;
    $time_ = timelocal ( $sec_, $min_, $hours_, $day_, $month_, $year_ ) ;
}
elsif ( $fromtz_ eq "MOS" || $fromtz_ eq "MSK" )
{
    $ENV{TZ} = 'Europe/Moscow' ;
    $time_ = timelocal ( $sec_, $min_, $hours_, $day_, $month_, $year_ ) ;
}
elsif ( $fromtz_ eq "UTC" || $fromtz_ eq "GMT" )
{
   $ENV{TZ} = 'GST+0' ;
    $time_ = timelocal ( $sec_, $min_, $hours_, $day_, $month_, $year_ ) ;
}


if ( $totz_ eq "TOR" || $totz_ eq "EST" )
{
    $ENV{TZ} = 'America/Toronto';
    my @datetime_ =  localtime ( $time_ ) ;
    print sprintf ( "%02d%02d%02d %02d%02d%02d\n", $datetime_[ 5 ] + 1900, $datetime_[ 4 ] + 1, $datetime_[ 3 ], $datetime_[ 2 ], $datetime_[ 1 ], $datetime_[ 0 ] ) ; 
}
elsif ( $totz_ eq "NY" || $totz_ eq "EST" )
{
    $ENV{TZ} = 'America/New_York' ;
    my @datetime_ =  localtime ( $time_ ) ;
    print sprintf ( "%02d%02d%02d %02d%02d%02d\n", $datetime_[ 5 ] + 1900, $datetime_[ 4 ] + 1, $datetime_[ 3 ], $datetime_[ 2 ], $datetime_[ 1 ], $datetime_[ 0 ] ) ; 
}
elsif ( $totz_ eq "CHI" || $totz_ eq "CST" )
{
    $ENV{TZ} = 'America/Chicago' ;
    my @datetime_ =  localtime ( $time_ ) ;
    print sprintf ( "%02d%02d%02d %02d%02d%02d\n", $datetime_[ 5 ] + 1900, $datetime_[ 4 ] + 1, $datetime_[ 3 ], $datetime_[ 2 ], $datetime_[ 1 ], $datetime_[ 0 ] ) ; 
}
elsif ( $totz_ eq "BRZ" || $totz_ eq "BRT" )
{
    $ENV{TZ} = 'America/Sao_Paulo' ;
    my @datetime_ =  localtime ( $time_ ) ;
    print sprintf ( "%02d%02d%02d %02d%02d%02d\n", $datetime_[ 5 ] + 1900, $datetime_[ 4 ] + 1, $datetime_[ 3 ], $datetime_[ 2 ], $datetime_[ 1 ], $datetime_[ 0 ] ) ; 
}
elsif ( $totz_ eq "BSL" || $totz_ eq "LDN" || $totz_ eq "BST" )
{
    $ENV{TZ} = 'Europe/London' ;
    my @datetime_ =  localtime ( $time_ ) ;
    print sprintf ( "%02d%02d%02d %02d%02d%02d\n", $datetime_[ 5 ] + 1900, $datetime_[ 4 ] + 1, $datetime_[ 3 ], $datetime_[ 2 ], $datetime_[ 1 ], $datetime_[ 0 ] ) ; 
}
elsif ( $totz_ eq "FR" || $totz_ eq "CET" )
{
    $ENV{TZ} = 'Europe/Berlin' ;
    my @datetime_ =  localtime ( $time_ ) ;
    print sprintf ( "%02d%02d%02d %02d%02d%02d\n", $datetime_[ 5 ] + 1900, $datetime_[ 4 ] + 1, $datetime_[ 3 ], $datetime_[ 2 ], $datetime_[ 1 ], $datetime_[ 0 ] ) ; 
}
elsif ( $totz_ eq "IND" || $totz_ eq "IST" )
{
    $ENV{TZ} = 'Asia/Kolkata' ;
    my @datetime_ =  localtime ( $time_ ) ;
    print sprintf ( "%02d%02d%02d %02d%02d%02d\n", $datetime_[ 5 ] + 1900, $datetime_[ 4 ] + 1, $datetime_[ 3 ], $datetime_[ 2 ], $datetime_[ 1 ], $datetime_[ 0 ] ) ; 
}
elsif ( $totz_ eq "HK" || $totz_ eq "HKT" )
{
    $ENV{TZ} = 'Asia/Hong_Kong' ;
    my @datetime_ =  localtime ( $time_ ) ;
    print sprintf ( "%02d%02d%02d %02d%02d%02d\n", $datetime_[ 5 ] + 1900, $datetime_[ 4 ] + 1, $datetime_[ 3 ], $datetime_[ 2 ], $datetime_[ 1 ], $datetime_[ 0 ] ) ; 
}
elsif ( $totz_ eq "TOK" || $totz_ eq "JST" )
{
    $ENV{TZ} = 'Asia/Tokyo' ;
    my @datetime_ =  localtime ( $time_ ) ;
    print sprintf ( "%02d%02d%02d %02d%02d%02d\n", $datetime_[ 5 ] + 1900, $datetime_[ 4 ] + 1, $datetime_[ 3 ], $datetime_[ 2 ], $datetime_[ 1 ], $datetime_[ 0 ] ) ; 
}
elsif ( $totz_ eq "SYD" || $totz_ eq "ASX" )
{
    $ENV{TZ} = 'Australia/Sydney' ;
    my @datetime_ =  localtime ( $time_ ) ;
    print sprintf ( "%02d%02d%02d %02d%02d%02d\n", $datetime_[ 5 ] + 1900, $datetime_[ 4 ] + 1, $datetime_[ 3 ], $datetime_[ 2 ], $datetime_[ 1 ], $datetime_[ 0 ] ) ; 
}
elsif ( $totz_ eq "MOS" || $totz_ eq "MSK" )
{
    $ENV{TZ} = 'Europe/Moscow' ;
    my @datetime_ =  localtime ( $time_ ) ;
    print sprintf ( "%02d%02d%02d %02d%02d%02d\n", $datetime_[ 5 ] + 1900, $datetime_[ 4 ] + 1, $datetime_[ 3 ], $datetime_[ 2 ], $datetime_[ 1 ], $datetime_[ 0 ] ) ; 
}
elsif ( $totz_ eq "UTC" || $totz_ eq "GMT" )
{
    $ENV{TZ} = 'GST+0' ;
    my @datetime_ =  localtime ( $time_ ) ;
    print sprintf ( "%02d%02d%02d %02d%02d%02d\n", $datetime_[ 5 ] + 1900, $datetime_[ 4 ] + 1, $datetime_[ 3 ], $datetime_[ 2 ], $datetime_[ 1 ], $datetime_[ 0 ] ) ; 
}







