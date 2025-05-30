#!/usr/bin/perl

use warnings;
use strict;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

if ( $#ARGV < 1 )
{
    printf "usage: cmd input_date shortcode_list_filename \n";
    exit ( 0 );
}

my $YYYYMMDD = $ARGV[0];
my $SFILE = $ARGV[1];

open ( SFILE_HANDLE, "< $SFILE" ) or die " Could not open file $SFILE for reading \n" ;
my @shortcodes_ = <SFILE_HANDLE>; chomp ( @shortcodes_ );
close ( SFILE_HANDLE );

printf ( STDOUT "%s\n", $YYYYMMDD );
for ( my $i = 0 ; $i <= $#shortcodes_ ; $i ++ )
{
    my $t_out_ = `$BIN_DIR/get_volume_on_day $shortcodes_[$i] $YYYYMMDD`;
    printf ( STDOUT "%s", $t_out_ );
}

