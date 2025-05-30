#!/usr/bin/perl

# \file scripts/plot_pnl_diffs.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
use strict;
use warnings;
use File::Basename; # basename

my $USER = $ENV { 'USER' } ;
my $HOME_DIR = $ENV { 'HOME' } ;
my $REPO = "basetrade";

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";

my $USAGE = "$0 trades_file1 trades_file2";

if ( $#ARGV < 1 ) 
{ 
    printf "$USAGE\n"; 
    exit ( 0 );
}

my $trades_file_1_ = $ARGV [ 0 ] ; chomp ( $trades_file_1_ );
my $trades_file_2_ = $ARGV [ 1 ] ; chomp ( $trades_file_2_ );

my $temp_id_ = `date +%N`;
$temp_id_ = $temp_id_ + 0;

`mkdir -p $HOME_DIR/simtemp`;
my $temp_plot_file_ = $HOME_DIR."/simtemp/sim_real_diff".$temp_id_;

my $DIFF_SCRIPT = $SCRIPTS_DIR."/generate_trades_file_diffs.pl";
my $PLOT_SCRIPT = $SCRIPTS_DIR."/plot_multifile_cols.pl";

my $pnl_diff_tag_ = basename ( $trades_file_1_ )."-".basename ( $trades_file_2_ ).".pnl";
my $vol_diff_tag_ = basename ( $trades_file_1_ )."-".basename ( $trades_file_2_ ).".vol";

`$DIFF_SCRIPT $trades_file_1_ $trades_file_2_ $temp_plot_file_`;
`$PLOT_SCRIPT $temp_plot_file_ 2 $pnl_diff_tag_ WL $temp_plot_file_ 4 $vol_diff_tag_ WL`;

`rm -f $temp_plot_file_`;
