#!/usr/bin/perl

# \file scripts/get_thin_book_periods.pl
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

my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE =  "$0 dep_shortcode output_file start_time end_time period_duration_in_mins fraction_to_pick [ days_to_look_behind = 120 ]";

if ( $#ARGV < 5 ) 
{ 
    printf "$USAGE\n"; 
    exit ( 0 ); 
}

my $cmd_ = "";
my $dep_shortcode_ = $ARGV[0] ;
my $period_duration_in_mins_ = $ARGV[4] ;
my $fraction_to_pick_ = $ARGV[5] ;
my $period_msecs_ = $period_duration_in_mins_ * 60 * 1000 ;
my $output_file_ = $ARGV[1] ;
my $start_time_ = $ARGV[2];
my $end_time_ = $ARGV[3] ;
my $start_mfm_ = ( floor ( $start_time_ / 100 ) * 60 + $start_time_ % 100 ) * 60 * 1000 ;
my $end_mfm_ = ( floor ( $end_time_ / 100 ) * 60 + $end_time_ % 100 ) * 60 * 1000 ;
my $days_to_look_behind_ = 120;

if ( $#ARGV > 5 )
{
	$days_to_look_behind_ = $ARGV[6];
}


my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
$yyyymmdd_ = CalcPrevWorkingDateMult ( $yyyymmdd_ , 1 );

my $current_yyyymmdd_ = $yyyymmdd_;
my @sample_dates_ =  ( );
my @period_book_sizes_ = ( );

for ( my $days_ = $days_to_look_behind_ ; $days_ != 0 ; $days_ -- ) 
{
   if ( ! ValidDate ( $current_yyyymmdd_ ) ) 
   { # Should not be here.
      PrintStacktraceAndDie ( "Invalid date : $yyyymmdd_\n" );
      exit ( 0 );
   }
	
   if ( SkipWeirdDate ( $current_yyyymmdd_ ) || NoDataDateForShortcode ( $current_yyyymmdd_, $dep_shortcode_ ) ||
	IsProductHoliday ( $current_yyyymmdd_ , $dep_shortcode_ ) ||
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
       my $period_end_mfm_ = $mfm_ + $period_msecs_ ;
       my $period_start_mfm_ = $mfm_ ; 
       my $start_utc_time_ = floor ( ( ( $period_start_mfm_ / 1000 ) / 60 ) / 60 ) * 100 + ( ( $period_start_mfm_ / 1000 ) / 60 ) % 60 ;
       my $end_utc_time_ = floor ( ( ( $period_end_mfm_ / 1000 ) / 60 ) / 60 ) * 100 + ( ( $period_end_mfm_ / 1000 ) / 60 ) % 60 ;
      	
       $cmd_ = "~/basetrade_install/bin/get_avg_l1sz_on_day $dep_shortcode_ $current_yyyymmdd_ $start_utc_time_ $end_utc_time_ | awk '{print \$3}'";
       my $book_size_ = `$cmd_`; $book_size_ = $book_size_ + 0;
       
       push ( @period_book_sizes_, $book_size_ ); 
   }

   $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
}


my @period_book_size_order_ = ();

@period_book_size_order_ = sort { $period_book_sizes_[$a] <=> $period_book_sizes_[$b] } 0 .. $#period_book_sizes_;


my $num_periods_per_day_ = ( $end_mfm_ - $start_mfm_ ) / $period_msecs_ ;
my $num_periods_ = ( $#sample_dates_ + 1 ) * $num_periods_per_day_;


my @low_book_size_periods_ = ( );

for ( my $i = 0; $i < ceil($fraction_to_pick_*$num_periods_); $i++ )
{
	push ( @low_book_size_periods_, $period_book_size_order_[$i] );
}

my @time_sorted_low_book_size_periods_ = sort { $a <=> $b } @low_book_size_periods_;


my $current_date_index_ = -1;

open OUTPUT, "> $output_file_" or PrintStacktraceAndDie ( "Could not open output_file_ $output_file_ for writing\n" );

for ( my $i = 0; $i <= $#time_sorted_low_book_size_periods_; $i++ )
{	
	my $date_index_ = floor($time_sorted_low_book_size_periods_[$i]/$num_periods_per_day_) ;
	if ( $date_index_ != $current_date_index_ )
	{

		if ( $current_date_index_ >= 0 )
		{
		  print OUTPUT "\n";
		}

		$current_date_index_= $date_index_;
		print OUTPUT $sample_dates_ [ $current_date_index_ ];
	}
	my $intraday_period_index_ = $time_sorted_low_book_size_periods_[$i] - $date_index_ * $num_periods_per_day_ ;	
	my $t_period_start_mfm_ = $intraday_period_index_ * $period_msecs_ + $start_mfm_ ;
	my $t_period_end_mfm_ = $t_period_start_mfm_ + $period_msecs_ ;
	
	print OUTPUT " ".$t_period_start_mfm_." ".$t_period_end_mfm_;
	
}
close OUTPUT;
