#!/usr/bin/perl

# \file scripts/generate_and_plot_cxl_seqd_to_conf.pl
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
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use List::Util qw/max min/; # for max
use FileHandle;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_iso_date_from_str.pl"; # GetIsoDateFromStr

my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
$LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";

my $GET_CSTC_EXEC = $LIVE_BIN_DIR."/get_cxl_seqd_to_conf";
my $PLOT_SCRIPT = $HOME_DIR."/".$REPO."/scripts/plot_multifile_cols.pl";
my $SAVEPLOT_SCRIPT = $HOME_DIR."/".$REPO."/scripts/saveplot_multifile_cols.pl";

my $USAGE = "$0 SHORTCODE YYYYMMDD [ PLOT = 1 ] [ MAIL = 0 ]";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];
my $yyyymmdd_ = GetIsoDateFromStr ( $ARGV [ 1 ] );
my $to_plot_ = 1;
if ( $#ARGV > 1 ) { $to_plot_ = $ARGV [ 2 ]; };
my $to_mail_ = 0;
if ( $#ARGV > 2 ) { $to_mail_ = $ARGV [ 3 ]; };

my $exec_cmd_ = "$GET_CSTC_EXEC $shortcode_ $yyyymmdd_";
if ( $USER eq "sghosh" && $to_mail_ == 1 )
{ # Most likely a cron job.
    my $exec_cmd_ = "$GET_CSTC_EXEC $shortcode_ $yyyymmdd_ /home/sghosh/master/temp";
    `mkdir -p /home/sghosh/master/temp`;
}    

# Exec returns the file name.
my $output_filename_ = `$exec_cmd_`; chomp ( $output_filename_ );

if ( -e $output_filename_ )
{
    my $temp_dir_ = $HOME_DIR."/GAPL";
    if ( $USER eq "sghosh" && $to_mail_ == 1 )
    {
	$temp_dir_ = "/home/sghosh/master/temp";
    }

    `mkdir -p $temp_dir_`;

    my $t_all_file_name_ = $temp_dir_."/cstc.".$shortcode_.".".$yyyymmdd_;
    `grep \"GetCxlSeqdToConfTimes\" $output_filename_ | awk '{ print \$2\" \"( \$3 * 1000.0 ); }' > $t_all_file_name_`;

    if ( $to_plot_ == 1 )
    {
	`$PLOT_SCRIPT $t_all_file_name_ 2 $shortcode_.$yyyymmdd_.cstc NL`;
    }

    if ( $to_mail_ == 1 )
    {
	my $t_output_png_ = $temp_dir_."/plot.".$shortcode_.".".$yyyymmdd_.".png";
	`$SAVEPLOT_SCRIPT $t_all_file_name_ 2 $shortcode_.$yyyymmdd_.cstc NL`;
    }

    `rm -f $t_all_file_name_`;
    `rm -f $output_filename_`;
}

exit ( 0 );
