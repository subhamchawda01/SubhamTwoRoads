# \file GenPerlLib/get_market_model_for_shortcode.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
use strict;
use warnings;
use feature "switch";
use List::Util qw[min max]; # max , min

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";

require "$GENPERLLIB_DIR/parse_utils.pl";
require "$GENPERLLIB_DIR/print_stacktrace.pl";
require "$GENPERLLIB_DIR/calc_prev_date.pl";
require "$GENPERLLIB_DIR/calc_next_date.pl";
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/search_script.pl"; # SearchScript


my $tz_mapping_file_ = "/spare/local/tradeinfo/tz_mapping.txt";
my %dvc_tz_code_tz_name_ = ();
LoadTZMap();

sub LoadTZMap 
{
  if ( -s $tz_mapping_file_ )
  {
    my $lines_vec_ref_ = ParseConfigLines($tz_mapping_file_);
    foreach my $line_ ( @$lines_vec_ref_ )
    {
      if ( @$line_ >= 2 )
      {
        $dvc_tz_code_tz_name_{$$line_[0]} = $$line_[1];
      }
    }
  }
}

sub GetTPFromTime
{
  my $time_ = shift;

  if ( GetUTCTime($time_) < GetUTCTime("CET_600") ) {return "AS";}  #this covers PRE_UTC_0 start_times gracefully as GetUTCTime will return -ve numbers for them
  elsif ( GetUTCTime($time_) < GetUTCTime("EST_600") ) {return "EU";}
  else {return "US";}
}

#return value can be +ve as well as -ve
sub GetUTCTime
{
  my $time_ = shift;
  my $date_ = @_ ? shift : `date +%Y%m%d`; chomp($date_);

  my $secs_from_midnight_ = GetTimestampFromTZ_HHMM_DATE($time_, $date_) - GetTimestampFromTZ_HHMM_DATE(0, $date_);
  if ( $secs_from_midnight_ == 0 ) { return 0; }
  else
  {
    my $sign_ = $secs_from_midnight_ > 0 ? 1 : -1;
    my $abs_mins_ = int(abs($secs_from_midnight_)/60);
    my $utc_time_ = $sign_*((int($abs_mins_/60))*100 + $abs_mins_%60);
    return $utc_time_;
  }
}

sub AddTime
{
    my $orig_time_ = shift;
    my $pre_str_ = "";
    if ( not ($orig_time_ =~ /^[0-9]/) )
    {
      $pre_str_ = substr($orig_time_, 0 ,4);
      $orig_time_ = substr($orig_time_, 4 );
    }
    my $diff = shift;
    my $new_min = (int($orig_time_/100))*60 + ($orig_time_%100) + $diff;
    my $new_time_ = (int($new_min/60))*100 + $new_min%60;
    return $pre_str_.$new_time_;
}

sub GetMinsFromHHMM
{
  my $hhmm_ = shift;
  return (int($hhmm_/100))*60 + ($hhmm_%100) ;
}

sub GetOverlapFraction
{
  my $st_1_ = shift;
  my $et_1_ = shift;
  my $st_2_ = shift;
  my $et_2_ = shift;
  my $date_ = @_ ? shift : `date +%Y%m%d`; chomp($date_);

  my $st_1_epoch_secs_ = GetTimestampFromTZ_HHMM_DATE($st_1_, $date_);
  my $et_1_epoch_secs_ = GetTimestampFromTZ_HHMM_DATE($et_1_, $date_);
  my $st_2_epoch_secs_ = GetTimestampFromTZ_HHMM_DATE($st_2_, $date_);
  my $et_2_epoch_secs_ = GetTimestampFromTZ_HHMM_DATE($et_2_, $date_);

  my $dur_in_secs_1_ = $et_1_epoch_secs_ - $st_1_epoch_secs_;
  my $overlap_secs_ = min($et_1_epoch_secs_, $et_2_epoch_secs_) - max($st_1_epoch_secs_, $st_2_epoch_secs_) ;

  if ( $dur_in_secs_1_ <= 0 || $overlap_secs_ <= 0 ) { return 0.0; }
  else { return $overlap_secs_/$dur_in_secs_1_; }
}

sub GetTimestampFromTZ_HHMM_DATE
{
  my $time_str_ = shift;
  my $date_ = @_ ? shift : `date +%Y%m%d`; chomp($date_);
  if ( index( $time_str_, "PREV_") == 0 )
  {
    $date_ = CalcPrevDate($date_);
    $time_str_ = substr($time_str_, 5); 
  }
  elsif ( index( $time_str_, "NEXT_") == 0 )
  {
    $date_ = CalcNextDate($date_);
    $time_str_ = substr($time_str_, 5); 
  }
  elsif ( index( $time_str_, "_" ) == -1 ) 
  {
    $time_str_ = int($time_str_) + 0;
    if ( $time_str_ < 0 ) { $date_ = CalcPrevDate($date_); }
    elsif ( $time_str_ > 2359 ) { $date_ = CalcNextDate($date_); }
    $time_str_ = abs($time_str_)%2400;
  }

  my $tz_code_ = "UTC";
  my $hhmm_str_ = $time_str_;
  if ( index( $time_str_, "_" ) >= 0 )
  {
    ( $tz_code_, $hhmm_str_ ) = split('_', $time_str_);
  }
  my $tz_str_ = GetTZStrFromDVCTZCode($tz_code_);
  my $cmd = "TZ=$tz_str_ date +%s -d\"$date_ $hhmm_str_\"";
  my $epoch_secs_ = `$cmd`; chomp($epoch_secs_);
  return (int($epoch_secs_) + 0);
}

sub GetEndTimeFromTP
{
  my $tp_ = shift;
  my @times_ = split( '-' , $tp_ );
  if ( $#times_>=1 )
  {
    return $times_[1];
  }
  return "";
}

sub GetTZStrFromDVCTZCode 
{
  my $dvc_code_ = shift;
  if ( exists($dvc_tz_code_tz_name_{$dvc_code_}) ) { return $dvc_tz_code_tz_name_{$dvc_code_}; }
  else 
  {
    PrintStacktrace("WARNING! GetTZStrFromDVCTZCode: Invalid TZ_CODE: $dvc_code_, returning UTC\n"); 
    return "UTC"; 
  } 
}

sub GetTrainingDatesFromDB
{
  my @trainingdates = ();
  my ($start_date_, $end_date_, $num_days_, $skip_dates_file_) = @_;
  my $generate_date_script = SearchScript ( "generate_dates.py", () );
  my $get_date_cmd;

  if ($end_date_ eq ""){
    print "end_date_ cannot be empty string.";
    exit(0);
  }

  if ($start_date_ ne "" and $num_days_ ne ""){
    $get_date_cmd = "python $generate_date_script -m 2 -sd $start_date_ -ed $end_date_ -n $num_days_";
  } elsif ($start_date_ eq "" and $num_days_ ne ""){
    $get_date_cmd = "python $generate_date_script -m 2 -ed $end_date_ -n $num_days_";
  } elsif ($start_date_ ne "" and $num_days_ eq "") {
    $get_date_cmd = "python $generate_date_script -m 2 -sd $start_date_ -ed $end_date_";
  } else{
    print "either start_date_ or num_days_ should be mentioned";
    exit(0);
  }

  my $ret = `$get_date_cmd`;
  
  my @dates = split /\s+/,$ret ;

  my @skip_dates_list = ();
  if (open(my $skip_fp, '<:encoding(UTF-8)', $skip_dates_file_)){
    while (my $skip_date = <$skip_fp>){
      chomp $skip_date; 
      push(@skip_dates_list, $skip_date);
    }
  }
  
  foreach my $onedate (@dates){
    if (!FindItemFromVec ( $onedate, @skip_dates_list )){
      push(@trainingdates, $onedate);
    }
  }
  return @trainingdates;
}

1
