#!/usr/bin/perl
use strict;
use warnings;

my $USER = $ENV { 'USER' } ;
my $HOME_DIR = $ENV { 'HOME' } ;

my $USAGE = "$0 SHORTCODE PRICE_LEVEL B/S DATE STARTTIME ENDTIME\n" ;
if ( $#ARGV < 1 ) 
{ 
    print $USAGE; 
    exit ( 0 ); 
}

my $shortcode_ = $ARGV [ 0 ] ; chomp ( $shortcode_ ) ;
my $price_ = $ARGV [ 1 ] ; chomp ( $price_ ) ;

my $buysell_ = $ARGV [ 2 ] ; chomp ( $buysell_ ) ;

my $yyyymmdd_ = $ARGV [ 3 ] ; chomp ( $yyyymmdd_ ) ;

my $start_time_ = $ARGV [ 4 ] ; chomp ( $start_time_ ) ;
my $end_time_ = $ARGV [ 5 ] ; chomp ( $end_time_ ) ;

my $plot_script_ = $HOME_DIR."/plotscripts/plot_multifile_cols.pl";

my $temp_id_ = `date +%N` ; $temp_id_ = $temp_id_ + 0 ;

my $temp_file_ = "/home/sghosh/simtemp/price_level.".$temp_id_ ;

if ( index ( $shortcode_ , "BR_IND" ) >= 0 || index ( $shortcode_ , "BR_DOL" ) >= 0 || index ( $shortcode_ , "BR_WIN" ) >= 0 || index ( $shortcode_ , "BR_WDO" ) >= 0 )
{
    if ( $buysell_ eq "B" || $buysell_ eq "b" )
    {
	`/home/sghosh/infracore_install/bin/mkt_book_print SIM $shortcode_ $yyyymmdd_ $start_time_ $end_time_ NTP | grep $price_ | awk '{ if ( \$5 == $price_ && \$2 < 6 ) { print \$1\" \"\$2\" \"\$3\" \"\$4\" \"\$5\" \"\$6 ; } }' | sort -g -k1 | uniq > $temp_file_` ;
    }
    else
    {
	`/home/sghosh/infracore_install/bin/mkt_book_print SIM $shortcode_ $yyyymmdd_ $start_time_ $end_time_ NTP | grep $price_ | awk '{ if ( \$9 == $price_ && \$12 < 6 ) { print \$1\" \"\$12\" \"\$11\" \"\$10\" \"\$9\" \"\$8 ; } }' | sort -g -k1 | uniq > $temp_file_` ;
    }
}
else
{
    if ( $buysell_ eq "B" || $buysell_ eq "b" )
    {
	`/home/sghosh/infracore_install/bin/mkt_book_print SIM $shortcode_ $yyyymmdd_ $start_time_ $end_time_ | grep $price_ | awk '{ if ( \$5 == $price_ && \$2 < 6 ) { print \$1\" \"\$2\" \"\$3\" \"\$4\" \"\$5\" \"\$6 ; } }' | sort -g -k1 | uniq > $temp_file_` ;
    }
    else
    {
	`/home/sghosh/infracore_install/bin/mkt_book_print SIM $shortcode_ $yyyymmdd_ $start_time_ $end_time_ | grep $price_ | awk '{ if ( \$9 == $price_ && \$12 < 6 ) { print \$1\" \"\$12\" \"\$11\" \"\$10\" \"\$9\" \"\$8 ; } }' | sort -g -k1 | uniq > $temp_file_` ;
    }
}

#`/tmp/plot_multifile_cols.pl $temp_file_ 2 level`;
`/tmp/plot_multifile_cols.pl $temp_file_ 3 size`;
`/tmp/plot_multifile_cols.pl $temp_file_ 4 orders`;
