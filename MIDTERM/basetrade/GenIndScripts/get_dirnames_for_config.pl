#! /usr/bin/perl


if ( $#ARGV < 0 )
{
    print "USAGE: <script> <config>\n";
    exit(0);
} 

my $config = $ARGV[0] ;
my @a = `grep SELF $config | awk '{for ( i = 2 ; i <=NF ; i++ ) print \$i;}'`;
my @b = `grep PREDDURATION $config | awk '{for ( i = 2 ; i <=NF ; i++ ) print \$i;}'`;
my @c = `grep FILTER $config | awk '{for ( i = 2 ; i <=NF ; i++ ) print \$i;}'`;
my @d = `grep TIMEPERIODSTRING $config | awk '{for ( i = 2 ; i <=NF ; i++ ) print \$i;}'`;
my @e = `grep DATAGEN_BASE_FUT_PAIR $config | awk '{for ( i = 2 ; i <=NF ; i++ ) print \$i;}'`;
my @f = `grep PREDALGO $config | awk '{for ( i = 2 ; i <=NF ; i++ ) print \$i;}'`;

chomp ( @a ) ;
chomp ( @b ) ;
chomp ( @c ) ;
chomp ( @d ) ;
chomp ( @e ) ;
chomp ( @f ) ;


for ( my $i = 0 ; $i < scalar ( @b ) ; $i++ )
{
for ( my $j = 0 ; $j < scalar ( @c ) ; $j++ )
{
    my $t =  $a[ 0 ]."_".$b[ $i ]."_".$c[ $j ]."_".$f[ 0 ]."_".$d[ 0 ]."_".$e[ 0 ]."_".$e[ 1 ] ;
    print $t, "\n";
}
}
