#!/usr/bin/perl

# \file scripts/compute_prod_stdev.pl
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
use Math::Complex ; # sqrt
my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
my $TRADEINFO_DIR="/spare/local/tradeinfo/";
my $STDEV_INFO_DIR=$TRADEINFO_DIR."/StdevInfo";

my $dep_shortcode_ = "";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE =  "$0 dep_shortcode start_time end_time [ days_to_look_behind = 60 ] stdev_file";

if ( $#ARGV < 2 ) 
{ 
    printf "$USAGE\n"; 
    exit ( 0 ); 
}

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
$yyyymmdd_ = CalcPrevWorkingDateMult ( $yyyymmdd_ , 1 );

my $stdev_file_ = $STDEV_INFO_DIR."/stdev_info_".$yyyymmdd_.".txt";

$dep_shortcode_ = $ARGV[0];
my @pred_durations_ = (300);
my $start_time_ = $ARGV[1];
my $end_time_ = $ARGV[2];
my $days_to_look_behind_ = 60;

if ( $#ARGV >= 3 ) 
{
    $days_to_look_behind_ = $ARGV[3];
}

if ( $#ARGV >= 4 ) 
{
    $stdev_file_ = $ARGV[4];
}

if ( -e $stdev_file_ ) 
{
 open STDEV_OUT_FILE, "< $stdev_file_ " or PrintStacktraceAndDie ( " could not open file $stdev_file_ for reading\n" );
 my @existing_stdevs_ = <STDEV_OUT_FILE>;
 close STDEV_OUT_FILE;
 open STDEV_IN_FILE, "> $stdev_file_ " or PrintStacktraceAndDie ( " could not open file $stdev_file_ for writing\n" ) ;
 
 foreach my $line_ ( @existing_stdevs_ ) 
 {
     chomp ( $line_ ) ;
     my @words_ = split ( ' ', $line_ ) ;
     if ( $words_[0] eq $dep_shortcode_ ) 
     {
         next ;
     } 
     else 
     {
         print STDEV_IN_FILE "$line_\n";
     }
 }
 close STDEV_IN_FILE;
}

my $indicator_list_ = "/spare/local/$USER/tmp_ilist";

#building empty ilist for stdev calculation of dependent
my $cmd_ = "echo \"MODELINIT DEPBASE $dep_shortcode_ MktSizeWPrice MktSizeWPrice\" > $indicator_list_";
`$cmd_`;
$cmd_ = "echo \"MODELMATH LINEAR CHANGE\" >> $indicator_list_";
`$cmd_`;
$cmd_ = "echo \"INDICATORSTART\" >> $indicator_list_";
`$cmd_`;
$cmd_ = "echo \"INDICATOREND\" >> $indicator_list_";
`$cmd_`;

my $tmp_dgen_filename_ = "/spare/local/".$USER."/tmp_dgen".$dep_shortcode_;
my $tmp_reg_data_filename_ = "/spare/local/".$USER."/tmp_regdata".$dep_shortcode_;


my %pred_duration_to_reg_data_ = ();
foreach my $pred_duration_ ( @pred_durations_ )
{
    my $this_pred_reg_file_ = $tmp_reg_data_filename_."_".$pred_duration_;
    $pred_duration_to_reg_data_ {$pred_duration_} =  $this_pred_reg_file_;
    `> $this_pred_reg_file_`;
}

my $current_yyyymmdd_ = $yyyymmdd_;

my @sample_dates_ =  ( );

my @period_corrs_ = ( );

open STDEV_FILE, ">> $stdev_file_" or PrintStacktraceAndDie ( "Could not open output_file_ $stdev_file_ for writing\n" );

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
   
   $cmd_ = "cat $stdev_file_ | grep -c $current_yyyymmdd_ ";
   my $date_already_present_ = `$cmd_`; 
   $date_already_present_ += 0;

   if( $date_already_present_ > 0 )
   {
      $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
      next;      
   }
 
   push ( @sample_dates_, $current_yyyymmdd_ );

   my $datagen_issues_ = -1;
   foreach my $duration_ (@pred_durations_)
   {
	$cmd_ = "$LIVE_BIN_DIR/datagen $indicator_list_ $current_yyyymmdd_ $start_time_ $end_time_ 11111 $tmp_dgen_filename_ 500 1 0 0";
	`$cmd_`;
	if( not (-s $tmp_dgen_filename_) )
	{	   
	   $datagen_issues_ = 1;
	   next;
	}
	$cmd_ = "$LIVE_BIN_DIR/timed_data_to_reg_data $indicator_list_ $tmp_dgen_filename_ $duration_ na_t1 $tmp_reg_data_filename_";
	`$cmd_`;
    
    my $this_pred_duration_reg_data_ = $pred_duration_to_reg_data_{$duration_};
    my $cp_cmd_ = "cat $tmp_reg_data_filename_ >> $this_pred_duration_reg_data_";
    `$cp_cmd_`;
   }

   $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
}

my $count_ = 0 ;
my $total_ = 0.0;
foreach my $duration_  ( @pred_durations_ ) 
{
    my $reg_data_file_ = $pred_duration_to_reg_data_{$duration_};
    $cmd_ = "cat $reg_data_file_ | awk '{sum1+=\$1; sum2+=(\$1 * \$1);}END{ print sqrt(sum2/NR - (sum1*sum1)/(NR*NR)) }'";
	my $t_stdev_ = `$cmd_`; $t_stdev_ += 0.0;	
	$t_stdev_ = int($t_stdev_ * 10000 + 1.1)/10000.0;
    $total_ = $total_ + 300 * ( $t_stdev_) / $duration_;
    $count_++;
    print "$duration_ $t_stdev_\n";
    `rm -f $reg_data_file_`;
}

printf  STDEV_FILE "%s %0.4f\n", $dep_shortcode_ , $total_/$count_;

close STDEV_FILE;

$cmd_ = "rm -f $indicator_list_ $tmp_dgen_filename_ $tmp_reg_data_filename_";
`$cmd_`;
