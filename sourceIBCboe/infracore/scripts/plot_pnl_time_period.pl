#!/usr/bin/perl
use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
require "$GENPERLLIB_DIR/get_iso_date_from_str.pl"; # GetIsoDateFromStr
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

my $usage_ = "plot_pnl_time_period.pl trades_file_1 trades_file_2 start_time end_time";

if ($#ARGV < 3) {
    print $usage_."\n";
    exit (0);
}

my $trades_file_1_ = $ARGV [ 0 ] ; chomp ( $trades_file_1_ ) ;
my $trades_file_2_ = $ARGV [ 1 ] ; chomp ( $trades_file_2_ ) ;

my $start_time_ = $ARGV [ 2 ] ; chomp ( $start_time_ ) ;
my $end_time_ = $ARGV [ 3 ] ; chomp ( $end_time_ ) ;

my $temp_dir_ = $HOME_DIR."/simtemp" ;
`mkdir -p $temp_dir_`;

my $temp_id_ = `date +%N` ; $temp_id_ = $temp_id_ + 0 ;
my $t_trades_file_1_ = $HOME_DIR."/simtemp/trades".$temp_id_ ;
$temp_id_ = `date +%N` ; $temp_id_ = $temp_id_ + 0 ;
my $t_trades_file_2_ = $HOME_DIR."/simtemp/trades".$temp_id_ ;

`cat $trades_file_1_ | awk '{ if ( \$1 > $start_time_ && \$1 < $end_time_ ) { print \$0 ; } }' > $t_trades_file_1_` ;
`cat $trades_file_2_ | awk '{ if ( \$1 > $start_time_ && \$1 < $end_time_ ) { print \$0 ; } }' > $t_trades_file_2_` ;

`/tmp/plot_multifile_cols.pl $t_trades_file_1_ 9 pnls_1 $t_trades_file_2_ 9 pnls_2`;

`$HOME_DIR/infracore/scripts/plot_pnl_diffs.pl $t_trades_file_1_ $t_trades_file_2_`;

`rm -f $t_trades_file_1_`;
`rm -f $t_trades_file_2_`;
