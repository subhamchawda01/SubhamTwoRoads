#!/usr/bin/perl
#
## \file ModelScripts/find_best_params_permute.pl
##
## \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
##  Address:
##       Suite No 162, Evoma, #14, Bhattarhalli,
##       Old Madras Road, Near Garden City College,
##       KR Puram, Bangalore 560049, India
##       +91 80 4190 3551
##
## This script takes :
#


use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw(max); # for max
use FileHandle;

sub GetUTCHHMMfromEpoch;
sub GetVolatilityForShortcode;

package ResultLine;
my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "diwakar" )
{
  $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";

my $TRADELOG_DIR="/spare/local/tradeinfo/volatilitylogs/";

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/array_ops.pl"; # GetConsMedianAndSort
require "$GENPERLLIB_DIR/get_core_shortcodes.pl"; #GetCoreShortcodes
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDi
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

my $GET_UTC_FROM_EPOCH_TIME=$SCRIPTS_DIR."/unixtime2gmtstr.pl";
my $GENERATE_PRICE_EXEC_=$BIN_DIR."/generate_price_data";
my $GENERATE_VOLUME_EXEC=$BIN_DIR."/get_periodic_volume_on_day";

my $TIME_DURATION = 600; # currently look at the 10 minutes data for the shourcode
my $VOLUME_FACTOR = 10; #if the current volume is this times more than avg volume take it as high volume
my $PRICE_FACTOR = 10; #if the price in this inteval has moved this muhc time the overall move, there is trend asusmed

my $USAGE = "$0 SHORTCODE DATE [ TIME_DURATION ] [ VOLUME_FACTOR ] [ PRICE_FACTOR ]";

if ( $#ARGV < 1 )
{
  print $USAGE."\n";
  exit;
}

my $dep_shortcode_ = $ARGV[0];
my $tradingdate_ = $ARGV[1];
if ( $#ARGV > 1 )
{
  $TIME_DURATION = $ARGV[2];
}

if ( $#ARGV > 2 )
{
  $VOLUME_FACTOR = $ARGV[3];
}

if ( $#ARGV > 3 )
{
  $PRICE_FACTOR = $ARGV[4];
}

my $out_file_name_ = $TRADELOG_DIR."/".$dep_shortcode_.".".$tradingdate_.".".$TIME_DURATION."_".$VOLUME_FACTOR."_".$PRICE_FACTOR;
if ( ExistsWithSize ( $out_file_name_ )  || IsProductHoliday( $dep_shortcode_, $tradingdate_) || IsDateHoliday($tradingdate_))
{
  exit;
}


my $out_file = $TRADELOG_DIR."/vol_".$tradingdate_; #file having avg volume and max price move on a day for each product

my @core_shortcodes_ = GetCoreShortcodes ($dep_shortcode_ );
my %shc_to_vol_time_;
my %shc_to_vol_value_;

my @dep_vol_time_ = ();
my @dep_vol_value_ = ();
my @dep_price_time_ = ();
my @dep_price_value_ = ();
my $max_price_move_today_ = "";
my $average_volume_on_day_ = "";
my %shc_to_avg_vol_;
my %shc_to_vol_indices_ ;

my ( $dep_vol_time_ref_ , $dep_vol_value_ref_, $dep_price_time_ref_, $dep_price_value_ref_) = GetVolatilityForShortcode( $dep_shortcode_, "true" );
@dep_vol_time_ = @$dep_vol_time_ref_;
@dep_vol_value_ = @$dep_vol_value_ref_;
@dep_price_time_ = @$dep_price_time_ref_;
@dep_price_value_ = @$dep_price_value_ref_;
#The data in the out file will be in format
#Interval_Start Interval_End IsDepHighVol IsDepTrend IsCoreHighVol1 IsCoreHighVol2 .....

open ( OUT_FILE, ">",$out_file_name_) or PrintStacktraceAndDie ( "Could not open $out_file_name_ to write.");

#print $#dep_vol_time_." ".$#dep_price_time_."\n";
#dependent shortcode
foreach my $shc_ (@core_shortcodes_)
{
# print " This core Shortcode_ ".$shc_."\n";
  my @dummy_ = ();
  $shc_to_vol_indices_{$shc_} = 0;
  my ( $vol_time_ref_, $vol_value_ref_, $dummy1_, $dummy2_ ) = GetVolatilityForShortcode( $shc_, "" );
  if ( $vol_time_ref_ && $vol_value_ref_ )
  {
    $shc_to_vol_time_ {$shc_} = $vol_time_ref_;
    $shc_to_vol_value_{$shc_} = $vol_value_ref_;
#  print " core values: ".$#{$shc_to_vol_time_{$shc_}}." ".$#{$shc_to_vol_value_{$shc_}}."\n";
  }
  else
  {
   print "No data for this shortcode.\n";
  }
}

my $this_line_ = "";
my $price_index_ = 0;
#print "Size: ".$#dep_vol_time_." ".$#dep_vol_value_."\n";
for ( my $index_= 0; $index_ < $#dep_vol_time_; $index_++ )
{
  if ($index_ == 0)
  {
    $this_line_ = $dep_vol_time_[$index_] - $TIME_DURATION." ".$dep_vol_time_[$index_]." ";
  }
  else
  {
    $this_line_ = $dep_vol_time_[$index_ - 1]." ".$dep_vol_time_[$index_]." ";
  }
# print " Printing time only: ".$average_volume_on_day_." arr val: ".$dep_vol_value_[$index_]."\n";

  if ($dep_vol_value_[$index_]  > $VOLUME_FACTOR * $average_volume_on_day_ )
  {
    $this_line_ = $this_line_." 1 ";
  }
  else
  {
    $this_line_ = $this_line_." 0 ";
  }

  my $avg_val_ = 0;
  my $count_ = 0;
  foreach my $key_ ( keys %shc_to_vol_time_ )
  {
    my @vol_time_arr_ = @{$shc_to_vol_time_{$key_}};
    my @vol_val_arr_ = @{$shc_to_vol_value_{$key_}};
#   print "This timeL ".$#vol_time_arr_." ".$key_."\n";
    while( ( $vol_time_arr_[$shc_to_vol_indices_{$key_}] < $dep_vol_time_[$index_]) && ($shc_to_vol_indices_{$key_} <= $#vol_time_arr_) && ( $index_ <= $#dep_vol_time_ ) )
    {
      if ( $vol_time_arr_[$shc_to_vol_indices_{$key_}] < $dep_vol_value_[$index_] - $TIME_DURATION ) #we can have bettercheck with setting cindition on $index_ = 0
      {
        $shc_to_vol_indices_{$key_}++;
        next;
      }
#     print " shortcode: ".$key_." Average Vol: ".$shc_to_avg_vol_{$key_}. " This volume: ". $vol_val_arr_[$shc_to_vol_indices_{$key_}]."\n";
      if ( $vol_val_arr_[$shc_to_vol_indices_{$key_}] > $VOLUME_FACTOR * $shc_to_avg_vol_{$key_} )
      {
        $avg_val_ = $avg_val_ + 1;
        $count_++
      }
      else
      {
        $count_++;
      }
      $shc_to_vol_indices_{$key_}++;
      last;
    }
  }
  if ( $count_ == 0 )
  {
    print " Average value with count == 0 ".$avg_val_."\n";
  }
  else
  {
    $avg_val_ /= $count_;
  }
  $this_line_ = $this_line_." ".(int(10* $avg_val_)/10)." ";
#core shortcodes to be done

  my $min_price_in_this_interval_ = $dep_price_value_[$price_index_];
  my $max_price_in_this_interval_ = 0;
# print " pricetime: " . $dep_price_time_[$price_index_]." value: ".$dep_price_value_[$price_index_]." DepData: ".$dep_vol_time_[$index_ - 1]." ".$dep_vol_time_[$index_]." ".$dep_vol_time_[$index_ + 1]."\n";
  while ( $dep_price_time_[$price_index_] < $dep_vol_time_[$index_]  && $price_index_ <= $#dep_price_value_ )
  {
    if ( $min_price_in_this_interval_  > $dep_price_value_[$price_index_] )
    {
      $min_price_in_this_interval_ = $dep_price_value_[$price_index_];
    }
    if ( $max_price_in_this_interval_ < $dep_price_value_[$price_index_] )
    {
      $max_price_in_this_interval_ = $dep_price_value_[$price_index_];
    }
    $price_index_++;
  }
  
# print " newpricetime: ".$dep_price_time_[$price_index_]." value: ".$dep_price_value_[$price_index_]."\n";

# print " Min Price: " . $min_price_in_this_interval_." max price".$max_price_in_this_interval_."\n";
#  print " PriceMove in this interval: ".($max_price_in_this_interval_ - $min_price_in_this_interval_)." price move today: ".$max_price_move_today_."\n";
  if ( $max_price_in_this_interval_ - $min_price_in_this_interval_ > $PRICE_FACTOR * $max_price_move_today_ )
  {
    $this_line_ = $this_line_." 1 ";
  }
  else
  {
    $this_line_ = $this_line_." 0 ";
  }

# print "ThisLine: ".$this_line_."\n";
  print OUT_FILE $this_line_."\n";
}

exit;

sub GetVolatilityForShortcode
{
  my ( $shortcode_, $calc_price_data_ ) = @_;
  my $periodic_volume_cmd_ = $GENERATE_VOLUME_EXEC." ".$shortcode_." ".$tradingdate_." ".$TIME_DURATION;
  if ( $USER eq "diwakar")
  {
    print $periodic_volume_cmd_."\n"
  }
  my @periodic_volumes_ = `$periodic_volume_cmd_`; chomp(@periodic_volumes_);

  my @volume_time_ = ( );
  my @volume_value_ = ( );

  foreach my $volume_line_ (@periodic_volumes_)
  {
    my @volume_line_words_ = split (' ', $volume_line_);
    if ( $#volume_line_words_ >= 1 )
    {
      push ( @volume_time_, $volume_line_words_[0] );
      push ( @volume_value_, $volume_line_words_[1] );
    }
  }

# print " volume vec_len: ".$#volume_time_." ".$#volume_value_ ." shc ".$shortcode_."\n";
  my $average_ = GetAverage ( \@volume_value_ );
  if ( $calc_price_data_ )
  {
    $average_volume_on_day_ = $average_;
  }
  else
  {
    $shc_to_avg_vol_ { $shortcode_ } = $average_;
  }
  
# print " volume vec_len: ".$#volume_value_ ." shc ".$shortcode_."\n";
  my $start_time_ = min ( @volume_time_ )  ;
  my $end_time_ = max ( @volume_time_ );
# print " volume ".$start_time_." ".$end_time_."\n";
  my $start_hhmm_ = GetUTCHHMMfromEpoch ( $start_time_ );
  my $end_hhmm_ = GetUTCHHMMfromEpoch ( $end_time_ );

# print " SHortcode: " .$shortcode_."\n";
# print $average_volume_on_day_."\n";
  if ( not $calc_price_data_ )
  {
    if ( $#volume_value_ >= 0 )
    {
      return (\@volume_time_, \@volume_value_, \@volume_time_, \@volume_value_);
    }
    else
    {
      return ("", "", "", "");
    }
  }


  #generate_price_data
  my $periodic_price_cmd_ = $GENERATE_PRICE_EXEC_." SHORTCODE ".$shortcode_." -1 PORT -1 ".$tradingdate_." Midprice ".$start_hhmm_." ".$end_hhmm_. " -1 1";
  if ( $USER eq "diwakar")
  {
    print $periodic_price_cmd_."\n";
  }

  my @periodic_price_ = `$periodic_price_cmd_`; chomp ( @periodic_price_);

  my $prev_time_ = 0;
  my $min_price_ = 0;
  my $max_price_ = 0;
  my $prev_price_ = 0;

  my @price_time_ = ( );
  my @price_value_ = ( );
  for ( my $i = 0; $i < $#periodic_price_ ; $i++ )
  {
    my @price_line_words_ = split (' ', $periodic_price_[$i] );
    if ( $i == 0 ) 
    {
      $max_price_ = $price_line_words_[2];
      $min_price_ = $price_line_words_[2];
    }
    else
    {
      if ( $price_line_words_[2] > $max_price_ )
      {
        $max_price_ = $price_line_words_[2];
      }
      if ( $price_line_words_[2] < $min_price_ )
      {
        $min_price_ = $price_line_words_[2];
      }
    }
    my $this_time_ = int ($price_line_words_[0] );
    my $this_price_ = $price_line_words_[2];

#  if ( $this_time_ > $prev_time_+ $TIME_DURATION )    
    #Here the value of time duration sholuld be different fordifferent product
    #Or we can select the highest value in that time intval
    if ( $this_price_ != $prev_price_ )
      {
        push ( @price_time_, $this_time_  );
        push ( @price_value_ , $this_price_ );
        $prev_time_ = $this_time_;
        $prev_price_ = $this_price_;
      }  
  }

  $max_price_move_today_ = $max_price_ - $min_price_;
  if ( $USER eq "diwakar" )
  {
    print $max_price_." min ".$min_price_." ".$max_price_move_today_."\n";
  }

  return ( \@volume_time_, \@volume_value_, \@price_time_, \@price_value_ );
}

sub GetUTCHHMMfromEpoch
{
  my ( $this_epoch_ ) = @_;
  my $get_utc_hh_mm_cmd_ = $GET_UTC_FROM_EPOCH_TIME." ".$this_epoch_;
  if ( $USER eq "diwakar" )
  {
    print $get_utc_hh_mm_cmd_."\n";
  }
  my $utc_hhmm_str_ = `$get_utc_hh_mm_cmd_`; chomp ($utc_hhmm_str_);
  my @utc_time_words_ = split ( ' ', $utc_hhmm_str_ );
  my $utc_hhmmss_str_ = $utc_time_words_[3];
  my @hhmmss_words_ = split (':', $utc_hhmmss_str_);
  return $hhmmss_words_[0].$hhmmss_words_[1];
}
