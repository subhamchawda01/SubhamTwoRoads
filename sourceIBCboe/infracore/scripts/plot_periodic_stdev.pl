#!/usr/bin/perl
use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'}; 


my $USAGE="$0 filename input_date [firstcol secondcol]";
if ( $#ARGV < 1 ) { print $USAGE; exit ( 0 ); }

my $shortcode_filename_ = $ARGV[0];
my $yyyymmdd = $ARGV[1];


open ( FILE_HANDLE, "< $shortcode_filename_" )or die "Input file could not be opened: $shortcode_filename_\n";
my @shortcodes_ = <FILE_HANDLE>; chomp (@shortcodes_);
close ( FILE_HANDLE );

my $firstcol=1;
my $secondcol=2;

for ( my $i = 0; $i <= $#shortcodes_; $i++)
{
    my $symbol_ = $shortcodes_[$i];
    my $interval_ = 300;
    my $outfile_ = $HOME_DIR."/plots/".$symbol_.".stdev.".$yyyymmdd.".txt";   
    my $outpng_ =  $HOME_DIR."/plots/".$symbol_.".stdev.".$yyyymmdd.".png";   
    `~/basetrade_install/bin/get_periodic_price_stdev_on_day_V1 $symbol_ $yyyymmdd $interval_ > $outfile_`;

    open (GP, "|gnuplot -persist ") or die "no gnuplot";
    #force buffer to flush after each write
    use FileHandle;
    GP->autoflush(1);
    print GP " set terminal png; \n set out \'$outpng_\'; \n set xdata time; \n set timefmt \"\%s\"; \n plot \'$outfile_\' using $firstcol:$secondcol with boxes\n; ";
    close GP;

}
   
