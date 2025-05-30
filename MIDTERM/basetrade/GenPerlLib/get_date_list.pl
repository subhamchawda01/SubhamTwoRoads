#!/usr/bin/perl

# \file scripts/get_list_of_dates_for_shortcode.pl
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

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
#my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
#my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $WKD_TEMP_DIR = "/spare/local/pickstrats_logs/temp_dir";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/sample_data_utils.pl"; # GetFeatureSum
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; #GetIsoDateFromStrMin1

my %eco_tag_to_full_eco_str_;
LoadEcoTags();

sub LoadEcoTags
{
  my $file_ = "/spare/local/tradeinfo/eco_tags";
  if ( open FILE , "< $file_" )
  {
    my @lines_ = <FILE>; chomp(@lines_);
    close FILE;
    foreach my $line_ (@lines_)
    {
      #NFP=>Nonfarm_Payrolls
      my @words_ = split("=>", $line_);
      next if ( $#words_ < 1) ;
      $eco_tag_to_full_eco_str_{$words_[0]} = $words_[1];
    }
  }
}

sub GetFullEcoStrFromTag
{
  my $eco_tag_ = shift;
  exists($eco_tag_to_full_eco_str_{$eco_tag_}) ? $eco_tag_to_full_eco_str_{$eco_tag_} : $eco_tag_;
}

sub GetAllEcoTags
{
  return [ keys %eco_tag_to_full_eco_str_ ];
}

sub GetEcoTagsMap
{
  return \%eco_tag_to_full_eco_str_;
}

sub GetAllDatesForShc
{
  #reading args
  my $args_ = shift;

  my $shortcode_ = $$args_{'SHC'}; $shortcode_ = "" unless defined $shortcode_;
  my $end_date_ = $$args_{'ED'}; $end_date_ = 'TODAY' unless defined $end_date_; $end_date_ = GetIsoDateFromStrMin1( $end_date_, $shortcode_ );
  if ( !ValidDate($end_date_) )
  {
    print STDERR "WARNING! GetAllDatesForShc:: invalid input ED: ".$$args_{'ED'}."[$end_date_ After Processing] Reason: [ !ValidDate ]\n";
    return [];
  }

  #giving preference to DAYS over SD but if both not specified the using DAYS = 100
  my $days_to_look_behind_ = $$args_{'DAYS'}; 
  my $start_date_ = -1;

  if ( defined $days_to_look_behind_ ) 
  { 
    #DAYS specified , using only DAYS
    $days_to_look_behind_ = int($days_to_look_behind_);
    if ( $days_to_look_behind_ <= 0 || $days_to_look_behind_ > 1000 )
    {
      print STDERR "WARNING! GetAllDatesForShc:: invalid input DAYS: ".$$args_{'DAYS'}."[$days_to_look_behind_ After Processing] Reason: [ <=0 || >1000 ]\n";
      return [];
    }
  }  
  else
  {
    $start_date_ = $$args_{'SD'}; 
    if ( defined $start_date_ ) 
    { 
      #DAYS not specified but SD specified, using SD with DAYS = 1000(sanity, avoid inf loop) 
      $start_date_ = GetIsoDateFromStrMin1( $start_date_, $shortcode_ );
      if ( !ValidDate($start_date_) )
      {
        print STDERR "WARNING! GetAllDatesForShc:: invalid input SD: ".$$args_{'SD'}."[$start_date_ After Processing] Reason: [ !ValidDate ]\n";
        return [];
      } 
      $days_to_look_behind_ = 1000; 
    }
    else
    {
      #both are not specified, using only DAYS = 100
      $days_to_look_behind_ = 100;
      $start_date_ = -1;
    }
  }  
  #done reading args

  if ( IsDateHoliday($end_date_) || ( $shortcode_ && IsProductHoliday($end_date_, $shortcode_) ) ) { $end_date_ = CalcPrevWorkingDateMult($end_date_,1,$shortcode_); }

  my $current_yyyymmdd_ = $end_date_;
  my $days_ = $days_to_look_behind_;
  my @sample_dates_ =  ( );
  while ( $days_ > 0 && $start_date_ <= $current_yyyymmdd_ ) 
  {
    push ( @sample_dates_, $current_yyyymmdd_ );   
    $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 , $shortcode_);
    $days_--;
  }
  return \@sample_dates_;
}

#SHC is mandatory argument
sub GetSimilarDaysForShc
{
  #reading args
  my $args_ = shift;

  my $shortcode_ = $$args_{'SHC'};
  my $cutoff_ = $$args_{'WKD'}; $cutoff_ = defined $cutoff_ ? $cutoff_ + 0.0 : 0.2;
  if ( !defined $shortcode_ || !defined $cutoff_ || $cutoff_ <= 0 )
  {
    print STDERR "WARNING! GetSimilarDaysForShc:: no input SHC\n";
    return [];
  }

  #not checking validity of these args, may have been helpful to debug
  my $end_date_ = $$args_{'ED'}; $end_date_ = 'TODAY-1' unless defined $end_date_; $end_date_ = GetIsoDateFromStrMin1( $end_date_, $shortcode_ );
  my $target_date_ = CalcNextDate( $end_date_ );
  my $stime_ = $$args_{'START_TIME'}; $stime_ = '0000' unless defined $stime_;
  my $etime_ = $$args_{'END_TIME'};  $etime_ = '2400' unless defined $etime_;

  if ( !ValidDate($end_date_) )
  {
    print STDERR "WARNING! GetSimilarDaysForShc:: invalid input ED: ".$$args_{'ED'}."[$end_date_ After Processing] Reason: [ !ValidDate ]\n";
    return [];
  }
  #done reading args
  
  my $all_dates_ = GetAllDatesForShc($args_);
  my $start_date_ = min ( @$all_dates_ );
  
  my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
  my $features_file_ = $WKD_TEMP_DIR."/wkodii_featuresdata_".$unique_gsm_id_;
  my $generate_features_exec_ = $HOME_DIR."/".$REPO."/WKoDii/get_day_features.pl";
  my $generate_features_cmd_ = "$generate_features_exec_ $shortcode_ $end_date_ $start_date_ -1 DAY $stime_ $etime_ > $features_file_";
  `$generate_features_cmd_ 2>/dev/null`;

  my $distance_metric_ = "Mahalanobis";
  my $exec_cmd_ = "python ".$HOME_DIR."/".$REPO."/WKoDii/obtain_weights_on_days.py $target_date_ -1 0 $features_file_ ARIMA $distance_metric_ 2>/dev/null";
  my $tradingdates_similarity_json_string_ = `$exec_cmd_ 2>/dev/null`; chomp( $tradingdates_similarity_json_string_ );

  `rm -rf $features_file_ 2>/dev/null`;

  my $tradingdates_similarity_ref_ = decode_json $tradingdates_similarity_json_string_ || return [];
  my @valid_tradingdates_ = grep { $$tradingdates_similarity_ref_{ $_ } > 0 } keys %$tradingdates_similarity_ref_;
  my @valid_tradingdates_sorted_ = sort { $$tradingdates_similarity_ref_{ $a } <=> $$tradingdates_similarity_ref_{ $b } } keys %$tradingdates_similarity_ref_;

  my $cutoff_idx_ = max( 0, $cutoff_ >= 1 ? $cutoff_ : min( $cutoff_ * @valid_tradingdates_, $#valid_tradingdates_ ) );
  my @filtered_dates_ = @valid_tradingdates_sorted_[ max(0, $#valid_tradingdates_sorted_ - $cutoff_idx_ + 1)..$#valid_tradingdates_sorted_ ];
  return \@filtered_dates_;
}

#SHC is mandatory argument
sub GetHVDatesForShc
{
  #reading args
  my $args_ = shift;

  my $shortcode_ = $$args_{'SHC'};
  if ( !defined $shortcode_ ) 
  {  
    print STDERR "WARNING! GetHVDatesForShc:: no input SHC\n";
    return [];
  } 

  #not checking validity of these args, may have been helpful to debug 
  my $hv_days_frac_ = $$args_{'HV_DAYS_FRAC'};  $hv_days_frac_ = defined $hv_days_frac_ ? $hv_days_frac_ + 0.0 : 0.2;
  my $stime_ = $$args_{'START_TIME'}; $stime_ = '0000' unless defined $stime_;
  my $etime_ = $$args_{'END_TIME'};  $etime_ = '2400' unless defined $etime_;
  my $low_vol = $$args_{'LOW_VOL'}; $low_vol = defined $low_vol ? int($low_vol) : 0;
  #done reading args

  my $all_dates_ = GetAllDatesForShc($args_);
  my %date_to_vol_map_ = ();
  my @zero_vol_days_ = ();
  foreach my $t_date_ ( @$all_dates_ )
  {
    my ($t_vol_, $is_valid_) = GetFeatureSum ( $shortcode_, $t_date_, "VOL", [], $stime_, $etime_ );
    if ( $t_vol_ > 0 && $is_valid_ )
    {
      $date_to_vol_map_{$t_date_} = $t_vol_; 
    }
    else
    {
      push(@zero_vol_days_, $t_date_);
    }
  }
  if ( $#zero_vol_days_ + 1 >= max(10, 0.2*($#$all_dates_ + 1)) )
  {
    print STDERR "WARNING! Too many ZeroVol Days. Please Check for Sample Data : @zero_vol_days_\n";
  }

  my @vol_sorted_dates_ = sort { $date_to_vol_map_{$b} <=> $date_to_vol_map_{$a} } keys %date_to_vol_map_;
  my @selected_dates_ = ();
  my $num_days_to_sel_ = min ( $#vol_sorted_dates_+1, max(1, $hv_days_frac_ >= 1 ? $hv_days_frac_ : ($hv_days_frac_*($#$all_dates_+1))) );
  if ( $low_vol > 0 )
  {
    @selected_dates_ = @vol_sorted_dates_[-$num_days_to_sel_ .. -1]
  }
  else
  {
    @selected_dates_ = @vol_sorted_dates_[0 .. $num_days_to_sel_-1]
  }
  #returning dates in descending order, latest date first
  @selected_dates_ = sort { $b <=> $a } @selected_dates_;

  return \@selected_dates_;
}


sub GetAllDatesForEcoEvent
{
  my $eco_str_ = shift;
  $eco_str_ = quotemeta GetFullEcoStrFromTag($eco_str_);
  my $exec_cmd_ = "grep $eco_str_ $HOME_DIR/infracore_install/SysInfo/BloombergEcoReports/merged_eco_201?_processed.txt | awk '{print \$NF}' | awk -F'_' '{print \$1}' | uniq | sort -nrk1";
  my @dates_ = `$exec_cmd_ 2>/dev/null`; chomp(@dates_);
  return \@dates_;
}

sub GetEcoDates
{
  #reading args
  my $args_ = shift;

  my $eco_str_ = $$args_{'ECO_STR'};
  if ( !defined $eco_str_ ) 
  {  
    print STDERR "WARNING! GetEcoDates:: no input ECO_STR\n";
    return [];
  } 
  elsif ( $eco_str_ =~ /^\s*$/ )
  {
    print STDERR "WARNING! GetEcoDates:: invalid input ECO_STR: ".$$args_{'ECO_STR'}."[$eco_str_ After Processing] Reason: [ empty string ]\n";
  }

  my $end_date_ = $$args_{'ED'}; $end_date_ = 'TODAY' unless defined $end_date_; $end_date_ = GetIsoDateFromStrMin1($end_date_);
  if ( !ValidDate($end_date_) )
  {
    print STDERR "WARNING! GetEcoDates:: invalid input ED: ".$$args_{'ED'}."[$end_date_ After Processing] Reason: [ !ValidDate ]\n";
    return [];
  }

  my $start_date_ = -1;
  my $last_n_ = $$args_{'LAST_N_EVENTS'}; 
  if ( defined $last_n_ )
  {
    $last_n_ = int($last_n_);
    if ( $last_n_ <= 0 || $last_n_ > 1000 )
    {
      print STDERR "WARNING! GetEcoDates:: invalid input LAST_N_EVENTS: ".$$args_{'LAST_N_EVENTS'}."[$last_n_ After Processing] Reason: [ <=0 || >1000 ]\n";
      return [];
    }
  }
  else
  {
    #LAST_N_EVENTS not specified , using DAYS|SD
    $start_date_ = $$args_{'SD'};
    my $num_days_ = $$args_{'DAYS'}; 
    if ( defined $num_days_ )
    {
      #DAYS specified, using DAYS
      $start_date_ = CalcPrevWorkingDateMult($end_date_, $num_days_-1);
    }
    elsif ( defined $start_date_ )
    {
      #DAYS not specified, SD specified, using SD
      $start_date_ = GetIsoDateFromStrMin1($start_date_);
    }
    else
    {
      #none specified, using DAYS = 100
      $start_date_ = CalcPrevWorkingDateMult($end_date_,99);
    }
    $last_n_ = 1000; #just a sane number
  }
  #done reading args

  my $all_eco_dates_ = GetAllDatesForEcoEvent($eco_str_);
  my @selected_dates_ = ();
  foreach my $t_date_ ( @$all_eco_dates_ )
  {
    #assuming all_eco_dates is sorted with latest date first
    last if ( $t_date_ < $start_date_ || @selected_dates_ >= $last_n_ );
    next if ( $t_date_ > $end_date_ );
    push ( @selected_dates_, $t_date_ );
  }
  return \@selected_dates_;
}

sub GetASXExpiryDays
{
  #reading args
  my $args_ = shift;

  my $asx_flag_ = $$args_{'ASX_EXPIRY'};
  if ( !defined $asx_flag_ ) 
  {  
    print STDERR "WARNING! ASX flag not set properly. Should be ASX_EXPIRY:0 or ASX_EXPIRY:1\n";
    return [];
  } 
  elsif ( $asx_flag_ =~ /^\s*$/ )
  {
    print STDERR "WARNING! GetASXExpiryDays:: invalid input ASX_EXPIRY: ".$$args_{'ASX_EXPIRY'}."[$asx_flag_ After Processing] Reason: [ empty string ]\n";
  }
  my $shortcode_ = $$args_{'SHC'};
  if ( !defined $shortcode_ ) 
  {  
    print STDERR "WARNING! GetHVDatesForShc:: no input SHC\n";
    return [];
  } 

  my $end_date_ = $$args_{'ED'}; $end_date_ = 'TODAY' unless defined $end_date_; $end_date_ = GetIsoDateFromStrMin1($end_date_);
  if ( !ValidDate($end_date_) )
  {
    print STDERR "WARNING! GetASXExpiryDays:: invalid input ED: ".$$args_{'ED'}."[$end_date_ After Processing] Reason: [ !ValidDate ]\n";
    return [];
  }

  my $start_date_ = $$args_{'SD'};
  my $num_days_ = $$args_{'DAYS'}; 
  if ( defined $num_days_ )
  {
    #DAYS specified, using DAYS
    $start_date_ = CalcPrevWorkingDateMult($end_date_, $num_days_-1);
  }
  elsif ( defined $start_date_ )
  {
    #DAYS not specified, SD specified, using SD
    $start_date_ = GetIsoDateFromStrMin1($start_date_);
  }
  else
  {
    #none specified, using DAYS = 100
    $start_date_ = CalcPrevWorkingDateMult($end_date_,99);
  }
  #done reading args

  my @selected_dates_ = ();
  my $flag_ = int($asx_flag_);
  while ( $end_date_ >= $start_date_ ) 
  {
    my $date = int(substr($end_date_, 6, 2));
    if (($flag_ == 1 && $date >= 8 && $date <= 15) || ($flag_ == 0 && ($date < 8 || $date > 15))) {
      push ( @selected_dates_, $end_date_ );
    }

    $end_date_ = CalcPrevWorkingDateMult ( $end_date_ , 1 , $shortcode_);
  }
  return \@selected_dates_;
}

1

