#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;

my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore";
my $BIN_DIR="$HOME_DIR/LiveExec/bin";
#First look at the ATR files
my $USAGE="$0 date";
if ( $#ARGV < 0 ) { print $USAGE; exit ( 0 ); }
my $date_ = $ARGV[0];
my $cmd_ = "$BIN_DIR/calc_prev_day $date_ " ;
my $prev_date_ = `$cmd_`; chomp ( $prev_date_ ) ;

my $str_ = "scp dvcinfra\@10.4.3.11:/spare/local/ORSlogs/TMX/MSTR1/position.$prev_date_ /home/dvcinfra/";
`$str_`;
print "/home/dvcinfra/position.$prev_date_";
