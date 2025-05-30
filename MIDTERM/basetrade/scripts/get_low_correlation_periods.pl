#!/usr/bin/perl

# \file scripts/get_low_correlation_periods.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use POSIX;
use List::Util qw[min max]; # max , min
use Scalar::Util qw(looks_like_number);

my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $INFRACORE_SCRIPTS_DIR=$HOME_DIR."/infracore_install/scripts";

my $dep_shortcode_ = "";
my $indep_shortcode_ = "";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_unix_time_from_utc.pl"; # GetUnixtimeFromUTC

sub GetCorrelationForPeriod ;

my $USAGE =  "$0 dep_shortcode indep_shortcode output_file start_time end_time period_duration_in_mins fraction_to_pick LOW[HIGH] [ days_to_look_behind = 60 ]";

if ( $#ARGV < 7 ) 
{ 
    printf "$USAGE\n"; 
    exit ( 0 ); 
}

$dep_shortcode_ = $ARGV[0] ;
$indep_shortcode_ = $ARGV[1] ;
my $period_duration_in_mins_ = $ARGV[5] ;
my $fraction_to_pick_ = $ARGV[6] ;
my $period_msecs_ = $period_duration_in_mins_ * 60 * 1000 ;
my $output_file_ = $ARGV[2] ;
my $start_time_ = $ARGV[3];
my $end_time_ = $ARGV[4] ;
my $start_mfm_ = ( floor ( $start_time_ / 100 ) * 60 + $start_time_ % 100 ) * 60 * 1000 ;
my $end_mfm_ = ( floor ( $end_time_ / 100 ) * 60 + $end_time_ % 100 ) * 60 * 1000 ;
my $days_to_look_behind_ = 60;

my $high_low_ = $ARGV[7] ;
my $uniq_id_ = `date +%N`; chomp($uniq_id_);
my $indicator_list_ = "/spare/local/tradeinfo/datageninfo/corr_ilists/".$dep_shortcode_."_".$indep_shortcode_."_ilist";
my $tmp_dgen_filename_ = "/spare/local/".$USER."/tmp_dgen_$uniq_id_";

my $timed_corr_file_ = "/spare/local/".$USER."/timed_corr_".$dep_shortcode_."_".$indep_shortcode_;

if ( $USER eq "dvctrader" )
{
	$timed_corr_file_ = "/spare/local/tradeinfo/datageninfo/timed_corr_results/timed_corr_".$dep_shortcode_."_".$indep_shortcode_;
}
my $corr_signs_filename_ = "/spare/local/tradeinfo/datageninfo/corr_ilists/corr_signs";

my $cmd_ = "cat $corr_signs_filename_ | grep $dep_shortcode_ | grep $indep_shortcode_ | awk '{print \$3}' | head -n1";
my $corr_sign_ = `$cmd_`; $corr_sign_ = $corr_sign_ + 0;

if($corr_sign_ < 0 )
{
    if($high_low_ eq "LOW" )
    {
	$high_low_ = "HIGH";
    }
    else
    {
	$high_low_ = "LOW";
    }
}

if ( $#ARGV > 7 )
{
	$days_to_look_behind_ = $ARGV[8];
}


my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
$yyyymmdd_ = CalcPrevWorkingDateMult ( $yyyymmdd_ , 1 );

my $current_yyyymmdd_ = $yyyymmdd_;
my @periods_corrs_ = ( );

my @sample_dates_ =  ( );

my @period_corrs_ = ( );

my $num_lines_ = 0;
my $last_date_ = 0;
if (-e $timed_corr_file_)
{
$num_lines_ = `wc -l $timed_corr_file_ | awk '{print \$1}'`;
chomp ($num_lines_);
$num_lines_ = int($num_lines_) + 0;
}

if ($num_lines_ > 0 )
{
$last_date_ = `head -n1 $timed_corr_file_ | awk '{print \$1}'`;
chomp ($last_date_);
$last_date_ = int($last_date_) + 0;
}
`cp $timed_corr_file_ $timed_corr_file_"_bkp"`;
open TIMED_CORR, "> $timed_corr_file_" or PrintStacktraceAndDie ( "Could not open output_file_ $timed_corr_file_ for writing\n" );

for ( my $days_ = $days_to_look_behind_ ; $days_ != 0 && $current_yyyymmdd_>$last_date_; $days_ -- ) 
{
   if ( ! ValidDate ( $current_yyyymmdd_ ) ) 
   { # Should not be here.
      PrintStacktraceAndDie ( "Invalid date : $yyyymmdd_\n" );
      exit ( 0 );
   }
	
   if ( SkipWeirdDate ( $current_yyyymmdd_ ) || NoDataDateForShortcode ( $current_yyyymmdd_, $dep_shortcode_ ) ||
	NoDataDateForShortcode ( $current_yyyymmdd_, $indep_shortcode_ ) ||
	IsProductHoliday ( $current_yyyymmdd_ , $dep_shortcode_ ) ||
	IsProductHoliday ( $current_yyyymmdd_ , $indep_shortcode_ ) || 
        IsDateHoliday ( $current_yyyymmdd_ ) ) 
   {
      $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
      $days_ ++;
       next;
   }
   
   push ( @sample_dates_, $current_yyyymmdd_ );

   # Get msecs for bad periods across all queries.
   for ( my $mfm_ = $start_mfm_ ; $mfm_ <= $end_mfm_ - $period_msecs_ ; $mfm_ = $mfm_ + $period_msecs_ )
   {
       my $corr_ = GetCorrelationForPeriod ($mfm_ , ($mfm_ + $period_msecs_ ), $current_yyyymmdd_ );
       push ( @period_corrs_, $corr_ ); 
   }

   $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
}

close TIMED_CORR;
`cat $timed_corr_file_"_bkp" >> $timed_corr_file_`;
`rm $timed_corr_file_"_bkp"`;
my @period_corr_order_ = ();

if ($high_low_ eq "LOW" )
{
   @period_corr_order_ = sort { $period_corrs_[$a] <=> $period_corrs_[$b] } 0 .. $#period_corrs_;
}
else
{
   @period_corr_order_ = sort { $period_corrs_[$b] <=> $period_corrs_[$a] } 0 .. $#period_corrs_; 
}


my $num_periods_per_day_ = ( $end_mfm_ - $start_mfm_ ) / $period_msecs_ ;
my $num_periods_ = ( $#sample_dates_ + 1 ) * $num_periods_per_day_;


my @low_corr_periods_ = ( );

for ( my $i = 0; $i < ceil($fraction_to_pick_*$num_periods_); $i++ )
{
	push ( @low_corr_periods_, $period_corr_order_[$i] );
}

my @time_sorted_low_corr_periods_ = sort { $a <=> $b } @low_corr_periods_;


my $current_date_index_ = -1;

open OUTPUT, "> $output_file_" or PrintStacktraceAndDie ( "Could not open output_file_ $output_file_ for writing\n" );

for ( my $i = 0; $i <= $#time_sorted_low_corr_periods_; $i++ )
{	
	my $date_index_ = floor($time_sorted_low_corr_periods_[$i]/$num_periods_per_day_) ;
	if ( $date_index_ != $current_date_index_ )
	{

		if ( $current_date_index_ >= 0 )
		{
		  print OUTPUT "\n";
		}

		$current_date_index_= $date_index_;
		print OUTPUT $sample_dates_ [ $current_date_index_ ];
	}
	my $intraday_period_index_ = $time_sorted_low_corr_periods_[$i] - $date_index_ * $num_periods_per_day_ ;	
	my $t_period_start_mfm_ = $intraday_period_index_ * $period_msecs_ + $start_mfm_ ;
	my $t_period_end_mfm_ = $t_period_start_mfm_ + $period_msecs_ ;
	
	print OUTPUT " ".$t_period_start_mfm_." ".$t_period_end_mfm_;
	
}
close OUTPUT;

sub GetCorrelationForPeriod
{

    my ( $start_mfm_ , $end_mfm_ , $yyyymmdd_ ) = @_ ;

    my $start_utc_time_ = floor ( ( ( $start_mfm_ / 1000 ) / 60 ) / 60 ) * 100 + ( ( $start_mfm_ / 1000 ) / 60 ) % 60 ;
    my $end_utc_time_ = floor ( ( ( $end_mfm_ / 1000 ) / 60 ) / 60 ) * 100 + ( ( $end_mfm_ / 1000 ) / 60 ) % 60 ; 
	
    my $exec_cmd_ = $LIVE_BIN_DIR."/datagen $indicator_list_ $yyyymmdd_ $start_utc_time_ $end_utc_time_ 2222 $tmp_dgen_filename_ 500 2 0 0" ;

    if ( not ( -s $tmp_dgen_filename_ ) )
    {
	print $exec_cmd_."\n";
    }
    `$exec_cmd_` ;
    
    my $tmp_indicators_regdata_filename_ = $tmp_dgen_filename_."_inds" ;
    $exec_cmd_ = "cat $tmp_dgen_filename_ | awk '{print \$(NF-1),\$NF}' > $tmp_indicators_regdata_filename_" ;

    if ( not ( -s $tmp_indicators_regdata_filename_ ) )
    {
	print $exec_cmd_."\n";
    }

    `$exec_cmd_` ;

    $exec_cmd_ = $LIVE_BIN_DIR."/get_dep_corr $tmp_indicators_regdata_filename_" ;
    my $t_corr_ = `$exec_cmd_`; chomp($t_corr_); 
    if ( ! looks_like_number ( $t_corr_ ) )
    {
	print $exec_cmd_."\n";
	`cp $tmp_indicators_regdata_filename_ ~/tmp_indicators_regdata_filename_`;	
    }
    $t_corr_ = $t_corr_ + 0.0;
   
    $exec_cmd_ = "rm -f $tmp_dgen_filename_";
    `$exec_cmd_`;

    $exec_cmd_ = "rm -f $tmp_indicators_regdata_filename_";
    `$exec_cmd_`;
   
    $exec_cmd_ = "~/basetrade_install/bin/get_volume_on_day $dep_shortcode_ $yyyymmdd_ $start_utc_time_ $end_utc_time_ | awk '{print \$3}'";
    my $t_dep_volume_ = `$exec_cmd_`; $t_dep_volume_ = $t_dep_volume_ + 0 ; 

    $exec_cmd_ = "~/basetrade_install/bin/get_volume_on_day $indep_shortcode_ $yyyymmdd_ $start_utc_time_ $end_utc_time_ | awk '{print \$3}'";
    my $t_indep_volume_ = `$exec_cmd_`; $t_indep_volume_ = $t_indep_volume_ + 0 ;    
 
    my $vol_ratio_ = int(($t_indep_volume_/max(1,$t_dep_volume_)) * 100 )/100.0;
    print TIMED_CORR $yyyymmdd_." ".$start_utc_time_." ".$end_utc_time_." ".$t_corr_." ".$t_dep_volume_." ".$t_indep_volume_." ".$vol_ratio_."\n" ;     
    return $t_corr_ ;

}
