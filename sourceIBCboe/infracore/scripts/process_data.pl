#!/usr/bin/perl
use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

my $USAGE="$0 SHORTCODE DATE STARTTIME ENDTIME\n";
if ( $#ARGV < 1 ) 
{ 
    print $USAGE; 
    exit ( 0 ); 
}

my $shortcode_ = $ARGV[0];
my $yyyymmdd_ = $ARGV[1];

my $start_time_ = -1;
my $end_time_ = -1;

if ($#ARGV >= 2) 
{
    $start_time_ = $ARGV[2];
}
if ($#ARGV >= 3) 
{
    $end_time_ = $ARGV[3];
}

{
    my $plot_dir_ = $HOME_DIR."/plots";
    `mkdir -p $plot_dir_`;
}

{ # MDS data processing.
    my $MDS_L1_EXEC=$HOME_DIR."/infracore_install/bin/mds_log_l1_trade";

    my $l1_file_name_ = $HOME_DIR."/plots/".$shortcode_."_".$yyyymmdd_."_l1";
    my $trades_file_name_ = $HOME_DIR."/plots/".$shortcode_."_".$yyyymmdd_."_trades";

    if ($start_time_ == -1) 
    {
	`$MDS_L1_EXEC $shortcode_ $yyyymmdd_ | /bin/grep L1 | awk '{ \$1=\"\"; if ( \$5 != 0 && \$6 != 0 ) { print \$0\" \"(\$3 / \$4)\" \"(\$8 / \$7); } }' > $l1_file_name_`;
	`$MDS_L1_EXEC $shortcode_ $yyyymmdd_ | /bin/grep TRADE | awk '{ \$1=\"\"; print \$0; }' > $trades_file_name_`;
    } elsif ($start_time_ != -1 && $end_time_ == -1) 
    {
	`$MDS_L1_EXEC $shortcode_ $yyyymmdd_ | /bin/grep L1 | awk '{ \$1=\"\"; if ( \$5 != 0 && \$6 != 0 && \$2 >= $start_time_ ) { print \$0\" \"(\$3 / \$4)\" \"(\$8 / \$7); } }' > $l1_file_name_`;
	`$MDS_L1_EXEC $shortcode_ $yyyymmdd_ | /bin/grep TRADE | awk '{ \$1=\"\"; if ( \$2 >= $start_time_ ) { print \$0; } }' > $trades_file_name_`;
    } elsif ($start_time_ != -1 && $end_time_ != -1) 
    {
	`$MDS_L1_EXEC $shortcode_ $yyyymmdd_ | /bin/grep L1 | awk '{ \$1=\"\"; if ( \$5 != 0 && \$6 != 0 && \$2 >= $start_time_ && \$2 <= $end_time_ ) { print \$0\" \"(\$3 / \$4)\" \"(\$8 / \$7); } }' > $l1_file_name_`;
	`$MDS_L1_EXEC $shortcode_ $yyyymmdd_ | /bin/grep TRADE | awk '{ \$1=\"\"; if ( \$2 >= $start_time_ && \$2 <= $end_time_ ) { print \$0; } }' > $trades_file_name_`;
    }
}

{ # ORS data processing.
    my $ORS_EXEC=$HOME_DIR."/infracore_install/bin/ors_binary_reader";

    my $ors_file_name_ = $HOME_DIR."/plots/".$shortcode_."_".$yyyymmdd_."_ors";

    if ($start_time_ == -1) 
    {
	`$ORS_EXEC $shortcode_ $yyyymmdd_ PLOTTABLE > $ors_file_name_`;
    } elsif ($start_time_ != -1 && $end_time_ == -1) 
    {
	`$ORS_EXEC $shortcode_ $yyyymmdd_ PLOTTABLE | awk '{ if ( \$1 >= $start_time_ ) { print \$0; } }' > $ors_file_name_`;
    } elsif ($start_time_ != -1 && $end_time_ != -1) 
    {
	`$ORS_EXEC $shortcode_ $yyyymmdd_ PLOTTABLE | awk '{ if (\$1 >= $start_time_ && \$1 <= $end_time_ ) { print \$0; } }' > $ors_file_name_`;
    }
}
