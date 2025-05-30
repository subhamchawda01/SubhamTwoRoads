#!/usr/bin/perl

# \file ModelScripts/t_check_strats_os.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
# stratname start_date end_date
# outputs pnl_avg,  and comparative metrics to globalresults

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $GENPERLLIB_DIR=$HOME_DIR."/basetrade_install/GenPerlLib/";
my $SIM_EXEC=$HOME_DIR."/LiveExec/bin/sim_strategy ";
my $COMP_SCRIPT=$HOME_DIR."/basetrade_install/bin/summarize_strategy_results ";

my $USAGE="$0 stratfile/stratfile_list startdate enddate shortcode strats_dir_path";

if ($#ARGV < 4 ) { print $USAGE."\n"; exit( 0 ); }

my $strat_name_ = $ARGV[0];
my $start_date_ = $ARGV[1];
my $end_date_ = $ARGV[2];
my $shortcode_ = $ARGV[3];
my $strats_dir_path_ = $ARGV[4];
my @strat_names_ = ();
my @dates_ = ();

my $is_single_file_ = 0;

#require "$GENPERLLIB_DIR/skip_weird_date.pl";
#require "$GENPERLLIB_DIR/is_product_holiday.pl";
#require "$GENPERLLIB_DIR/is_date_holiday.pl";
require "$GENPERLLIB_DIR/valid_date.pl";
require "$GENPERLLIB_DIR/no_data_date.pl";
require "$GENPERLLIB_DIR/calc_prev_date.pl";
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl";

#ec2_globalresults has different paths on aws and ny4
my $ec2_path=" ~/ec2_globalresults ";
my $hostname_ = `hostname`;
if ( substr($hostname_,0,3) eq "sdv" ) {
  $ec2_path=" /NAS1/ec2_globalresults ";
}


open SFILE,"< $strat_name_" or PrintStacktraceAndDie ( "Could not open file $strat_name_ for reading \n" );
my $sline_ = <SFILE>;
chomp ( $sline_ );
my @sline_tokens_ = split( ' ', $sline_ );
if( $#sline_tokens_ > 0  && $sline_tokens_[0] eq "STRATEGYLINE" )
{
    push ( @strat_names_, $strat_name_ );
    $is_single_file_ = 1;
}
else
{
    push ( @strat_names_, $sline_ );
}

if( $is_single_file_ == 0 )
{
    while( $sline_ = <SFILE> )
    {
        chomp ( $sline_ );
	push ( @strat_names_, $sline_ );
    }
}
close SFILE;
printf "Number of strats %d\n", $#strat_names_ + 1;

my $sim_date_ = $end_date_;
for ( my $t_day_index_ = 0; $t_day_index_ < 1000; $t_day_index_ ++ )
{
    if ( ( ! ValidDate ( $sim_date_ ) ) || $sim_date_ < $start_date_ )
    {
	last;
    }
    if ( SkipWeirdDate ( $sim_date_ ) || 
	 NoDataDateForShortcode ( $sim_date_, $shortcode_ ) || 
	 ( IsDateHoliday( $sim_date_ ) || IsProductHoliday ( $sim_date_, $shortcode_ ) ) 
	 )
    {
        $sim_date_ = CalcPrevWorkingDateMult( $sim_date_, 1 );
	next;
    }
    push ( @dates_, $sim_date_ );
    $sim_date_ = CalcPrevWorkingDateMult( $sim_date_, 1 );
}
printf "Number of days %d\n", $#dates_ + 1;

for ( my $t_ctr_ = 0; $t_ctr_ <= $#strat_names_; $t_ctr_++ )
{
    my $traded_volumes_ = 0;
    my $pnls_ = 0;
    my $d_counter_ = 0;
    my $better_ctr_ = 0;
    my $t_current_strat_ = $strat_names_[$t_ctr_];
    for ( my $d_ctr_ = 0; $d_ctr_ <= $#dates_; $d_ctr_ ++ )
    {
	my $curr_date_ = $dates_[$d_ctr_];
	my $cmdline_ = $SIM_EXEC."SIM ".$strat_names_[$t_ctr_]." 1234 ".$curr_date_." ADD_DBG_CODE -1";
	my $out_line_ = `$cmdline_ 2>/dev/null`;
	chomp( $out_line_ );
	my @out_line_tokens_ = split( ' ', $out_line_ );
	if( $out_line_tokens_[2] > 0 )
	{
	    $traded_volumes_ +=  $out_line_tokens_[2];
	    $pnls_ += $out_line_tokens_[1];
	    $d_counter_ += 1;
	}
    }
    my $pnl_avg_ = $pnls_/$d_counter_;
    my $vol_avg_ = $traded_volumes_/$d_counter_;
     
    my $c_cmdline_ = $COMP_SCRIPT." ".$shortcode_." ".$strats_dir_path_.$ec2_path.$start_date_." ".$end_date_." INVALIDFILE kCNAPnlAdjAverage 0 INVALIDFILE 0 | grep STATISTICS";
    my @statistics_lines_ = `$c_cmdline_`;
    chomp ( @statistics_lines_ );
    for ( my $s_ctr_ = 0; $s_ctr_ <= $#statistics_lines_; $s_ctr_++ )
    {
	my $t_stat_ = $statistics_lines_[$s_ctr_];
	my @t_stat_tokens_ = split( ' ', $t_stat_ );
	if ( $t_stat_tokens_[ 1 ] < $pnl_avg_ )
	{
	    $better_ctr_ += 1;
	}	
    }
    printf "Strat_Name:\t %s\n", $strat_names_[$t_ctr_];
    printf "PNL_Avg:\t %f\tVol_Avg:\t %f\n", $pnl_avg_, $vol_avg_;
    printf "Better than %f strats in pool\n\n", $better_ctr_/($#statistics_lines_ + 1)*100;

}
