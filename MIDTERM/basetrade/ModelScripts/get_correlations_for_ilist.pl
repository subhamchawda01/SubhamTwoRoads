#!/usr/bin/perl

use strict;
use warnings;
use FileHandle;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my  $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";

my $SPARE_HOME="/spare/local/".$USER."/";
my $SPARE_GCI_DIR=$SPARE_HOME."GCI/";
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $yyyymmdd_=`date +%Y%m%d`; chomp ( $yyyymmdd_ );

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDi

require "$GENPERLLIB_DIR/exists_with_size.pl";
require "$GENPERLLIB_DIR/create_enclosing_directory.pl";
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl";
require "$GENPERLLIB_DIR/calc_prev_date.pl";
require "$GENPERLLIB_DIR/calc_next_date.pl";

my $ilist_ = "dummy";
my $start_date_ = 0;
my $num_days_lookback_ = 0;
my $starthhmm_ = 0;
my $endhhmm_ = 0;

my @pred_durations_ = ( );

# start 
my $USAGE="$0 shortcode ilist start_date num_days_lookback starthhmm endhhmm pred_duration(s)";

if ( $#ARGV < 6 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV[0];
$ilist_ = $ARGV[1];
$start_date_ = $ARGV[2];
$num_days_lookback_ = $ARGV[3];
$starthhmm_ = $ARGV[4];
$endhhmm_ = $ARGV[5];

for ( my $i=6; $i <= $#ARGV; $i++ )
{
   push ( @pred_durations_, $ARGV[$i] );
}

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
#my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); 


my $current_yyyymmdd_ = $start_date_;
my @sample_dates_ =  ( );

for ( my $days_ = $num_days_lookback_ ; $days_ != 0 && $current_yyyymmdd_ > 20100101 ; $days_ -- )
{
   if ( ! ValidDate ( $current_yyyymmdd_ ) )
   { # Should not be here.      
      PrintStacktraceAndDie ( "Invalid date : $current_yyyymmdd_\n" );
      exit ( 0 );
   }

   if ( SkipWeirdDate ( $current_yyyymmdd_ ) || NoDataDateForShortcode ( $current_yyyymmdd_, $shortcode_ ) ||
        IsProductHoliday ( $current_yyyymmdd_ , $shortcode_ ) ||
        IsDateHoliday ( $current_yyyymmdd_ ) )
   {
      $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
      $days_ ++;
       next;
   }

   push ( @sample_dates_, $current_yyyymmdd_ );
   $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
}

my $cmd_="";

my $dgenoutfile_ = $SPARE_HOME."/dgenoutfile".$shortcode_;
my $regoutfile_ = $SPARE_HOME."/regoutfile".$shortcode_;

my $regdatafile_suffix_ = "regdatafile".$shortcode_;

my @regdatafiles_ = ( );

for ( my $i=0; $i <= $#pred_durations_; $i++ )
{
    my $duration_ = $pred_durations_[$i];
    my $regdatafile_ = $SPARE_HOME."/".$regdatafile_suffix_."_".$shortcode_."_".$duration_;
    push ( @regdatafiles_, $regdatafile_ );
    $cmd_ = "> $regdatafile_";
    `$cmd_`;
}

foreach my $date ( @sample_dates_ )
{
   $cmd_ = "$LIVE_BIN_DIR/datagen $ilist_ $date $starthhmm_ $endhhmm_ $unique_gsm_id_ $dgenoutfile_ 2000 0 0 0 0";
   `$cmd_`;   
   for ( my $i=0; $i <= $#pred_durations_; $i++ )
   {
      my $duration_ = $pred_durations_[$i] * 1000;
      my $regdatafile_ = $regdatafiles_[$i];
      $cmd_ = "$LIVE_BIN_DIR/timed_data_to_reg_data $ilist_ $dgenoutfile_ $duration_ na_t3 $regoutfile_";
      `$cmd_`;
      $cmd_ = "cat $regoutfile_ >> $regdatafile_";      
      `$cmd_`;
   }
}

$cmd_ = "rm -f $dgenoutfile_ $regoutfile_";
`$cmd_`;

for ( my $i=0; $i <= $#pred_durations_; $i++ )
{
   my $regdatafile_ = $regdatafiles_[$i];
   my $duration_ = $pred_durations_[$i];
   $cmd_ = "$LIVE_BIN_DIR/get_dep_corr $regdatafile_";
   my $corr_line_ = `$cmd_`;
   chomp ($corr_line_);
   my @corr_words_ = split ( ' ', $corr_line_ );
   print $duration_."\n";
   print "@corr_words_\n";
   $cmd_ = "rm -f $regdatafile_";
   `$cmd_`;
}
