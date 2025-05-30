#!/usr/bin/perl

# \file ModelScripts/get_stdev_model.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
# modelfile startdate enddate start_hhmm end_hhmm

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $GENRSMWORKDIR=$SPARE_HOME."RSM/";
`mkdir -p $GENRSMWORKDIR`;

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/strat_utils.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( $USER eq "rkumar" ) 
{ 
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}
if ( $USER eq "sghosh" || $USER eq "ravi" || $USER eq "ankit" || $USER eq "anshul" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/check_ilist_data.pl"; #CheckIndicatorData
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; #GetDatesFromStartDate
# start 
my $USAGE="$0 modelfile startdate/dateslist_file enddate/-1 starthhmm endhhmm";

if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }
my $model_filename_ = $ARGV[0];
my $datagen_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[1] );
my $datagen_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[2] );
my $datagen_start_hhmm_ = $ARGV[3];
my $datagen_end_hhmm_ = $ARGV[4];

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

my $datagen_msecs_timeout_ = 5000;
my $datagen_l1events_timeout_ = 0;
my $datagen_num_trades_timeout_ = 0;
my $to_print_on_economic_times_ = 0;

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;

my @model_coeff_vec_ = ();
my @model_coeff_alpha_vec_ = ();
my @model_coeff_beta_vec_ = ();

my $exec_cmd_ = "cat $model_filename_ | grep MODELMATH | awk '{print \$2}'";
my $model_math_line_ = `$exec_cmd_`; chomp ( $model_math_line_);
my $model_type_ = $model_math_line_;

if ( ! ( $model_type_ eq "LINEAR" || $model_type_ eq "SIGLR" || $model_type_ eq "SELECTIVENEW" || $model_type_ eq "SELECTIVESIGLR" ) ) {
  printf "-1 -1\nMODELTYPE:$model_type_ not supported\n";
  exit(0);
}
my $is_siglr_ = 0 + ($model_type_ eq "SIGLR" || $model_type_ eq "SELECTIVESIGLR");

my $shortcode_ = "NA";
$exec_cmd_ = "cat $model_filename_ | grep MODELINIT | awk '{print \$3}'";
$shortcode_ = `$exec_cmd_`; chomp ( $shortcode_ );

my @datagen_days_vec_ = ( );

if ( ! IsValidStartEndDate($datagen_start_yyyymmdd_, $datagen_end_yyyymmdd_) || $datagen_start_yyyymmdd_ > $datagen_end_yyyymmdd_ ) {
  if ( -f $datagen_start_yyyymmdd_ ) {
    open DATESHANDLE, "< $datagen_start_yyyymmdd_" or PrintStacktraceAndDie ( "Could not open file $datagen_start_yyyymmdd_ for reading" );
    @datagen_days_vec_ = <DATESHANDLE>;
    chomp @datagen_days_vec_;
    close DATESHANDLE;
  }
  else {
    print STDERR "Either Start/End Dates are invalid or start date is after end-date\n";
    exit(0);
  }
}
else {
  @datagen_days_vec_ = GetDatesFromStartDate( $shortcode_, $datagen_start_yyyymmdd_, $datagen_end_yyyymmdd_, "INVALIDFILE", 400 );
}
$datagen_end_yyyymmdd_ = max ( @datagen_days_vec_ );

my $min_price_increment_ = 1 ;
if ( $shortcode_ ne "NA" ) {
    $exec_cmd_ = "$LIVE_BIN_DIR/get_min_price_increment $shortcode_ $datagen_end_yyyymmdd_";
    $min_price_increment_ = `$LIVE_BIN_DIR/get_min_price_increment $shortcode_ $datagen_end_yyyymmdd_` ; chomp ( $min_price_increment_ ); # using single date in hope that min_inc doesnt change much
}

my $temp_model_filename_ = $GENRSMWORKDIR."tmp_stdev_model_".$unique_gsm_id_;

my $curr_index_ = -1;
my $curr_uniq_idx_ = -1;
my @last_ind_idx_vec_ = (-1);
my $reg_ind_str_ = "";
my $to_print_ = 1;
my $is_regime_ = 0;
my %ind_str_to_uniq_idx_map_ = ();
my %wt_idx_to_uniq_idx_map_ = ();

open MFILE, "< $model_filename_" or PrintStacktraceAndDie ( "Could not open model_filename_ $model_filename_ for reading\n" );
open TMFILE, "> $temp_model_filename_" or PrintStacktraceAndDie ( "Could not open model_filename_ $temp_model_filename_ for writing\n" );

while ( my $mline_ = <MFILE> ) {    
  $to_print_ = 1;
  chomp ( $mline_ );
  my @mwords_ = split ( ' ', $mline_ );

  if ( $#mwords_ >= 2 && $mwords_[0] eq "INDICATOR" ) {
    $curr_index_++;

    if ( $is_siglr_ ) {
      my @t_model_coeffs_ = split ( ':', $mwords_[1] );
      if ( $#t_model_coeffs_ >= 1) {
        push ( @model_coeff_alpha_vec_, $t_model_coeffs_[0] );
        push ( @model_coeff_beta_vec_, $t_model_coeffs_[1] );
      }
      else {
        printf "-1 -1\ninvalid weights for siglr model\n";
        exit(0);
      }	
    }
    else {
      push ( @model_coeff_vec_, $mwords_[1] );
    }

    my $ind_str_ = GetCleanIndicatorString( $mline_ );
    if ( not exists ( $ind_str_to_uniq_idx_map_{ $ind_str_ } ) ) {
      $curr_uniq_idx_++;
      $ind_str_to_uniq_idx_map_{ $ind_str_ } = $curr_uniq_idx_;
    }  
    else {
      $to_print_ = 0;
    }
    $wt_idx_to_uniq_idx_map_{ $curr_index_ } = $ind_str_to_uniq_idx_map_{ $ind_str_ };

  }
  elsif ( $#mwords_ >= 2 && $mwords_[0] eq "REGIMEINDICATOR" ) {
    $reg_ind_str_ = $mline_ ;
    $reg_ind_str_ =~ s/REGIMEINDICATOR/INDICATOR/;
    $to_print_ = 0;
  }
  elsif ( $#mwords_ >= 0 && $mwords_[0] eq "INDICATORINTERMEDIATE" ) {
    push ( @last_ind_idx_vec_, $curr_index_ );
    $to_print_ = 0;
  }
  elsif ( $#mwords_ >= 0 && $mwords_[0] eq "INDICATOREND" ) {
    push ( @last_ind_idx_vec_, $curr_index_ );
  }
  elsif ( $#mwords_ >= 0 && ( $mwords_[0] eq "INDICATORSTART" ) ) {
    if ( $reg_ind_str_ ) {
      $is_regime_ = 1;
      $mline_ = $mline_."\n$reg_ind_str_";
    }
  }
  elsif ( $#mwords_ >= 0 && ( $mwords_[0] eq "MODELMATH" ) ) {
    print TMFILE "MODELMATH LINEAR CHANGE\n"; #changing to linear aggregator for input to datagen
      $to_print_ = 0;
  }

  if ( $to_print_ ) {
    print TMFILE "$mline_\n";
  }
}
close MFILE;
close TMFILE;

my $invalid_reg_lines_ = 0;
my $abs_sum_l1 = 0;
my $sum_l1 = 0;
my $sum_l2 = 0;
my $count = 0; 	    
my $fil_lines_ = 0;

my @abs_sum_l1_vec_ = (0) x $#last_ind_idx_vec_;
my @sum_l1_vec_ = (0) x $#last_ind_idx_vec_;
my @sum_l2_vec_ = (0) x $#last_ind_idx_vec_;
my @count_vec_ = (0) x $#last_ind_idx_vec_; 	    
my @fil_lines_vec_ = (0) x $#last_ind_idx_vec_;
                    
my $offset_ = 4;
$offset_++ if $is_regime_;

foreach my $tradingdate_ ( @datagen_days_vec_ ) {
  next if CheckIndicatorData($tradingdate_, $model_filename_);

  my $this_day_timed_data_filename_ = $GENRSMWORKDIR."timed_data".$tradingdate_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$unique_gsm_id_."_".$datagen_msecs_timeout_."_".$datagen_l1events_timeout_."_".$datagen_num_trades_timeout_;
  $exec_cmd_="$LIVE_BIN_DIR/datagen $temp_model_filename_ $tradingdate_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";
#print "$exec_cmd_\n";
  `$exec_cmd_`;

  my $datagen_logfile_ = $DATAGEN_LOGDIR."log.".$yyyymmdd_.".".$unique_gsm_id_;
  `rm -f $datagen_logfile_`;

  if ( -e $this_day_timed_data_filename_ ) {
    open DFILE, "< $this_day_timed_data_filename_" or PrintStacktraceAndDie ( "Could not open this_day_timed_data_filename_ $this_day_timed_data_filename_ for reading\n" );

    while ( my $data_line_ = <DFILE> ) {
      chomp ( $data_line_ );
      my @dwords_ = split ( ' ', $data_line_ );
      if ( $#dwords_ >= $curr_uniq_idx_ + $offset_) {
        my $t_reg_model_num_ = 1;

#first indicator is regime indicator
        $t_reg_model_num_ = int ( $dwords_[4] ) if $is_regime_;

        if ( $t_reg_model_num_ > $#last_ind_idx_vec_ ) {
          $invalid_reg_lines_++;
          next;
        }

        my $t_start_ind_idx_ = $last_ind_idx_vec_[$t_reg_model_num_ - 1] + 1;
        my $t_end_ind_idx_ = $last_ind_idx_vec_[$t_reg_model_num_];

        my $this_model_value_ = 0;              
        if ( not $is_siglr_ ) {
          for ( my $j = $t_start_ind_idx_; $j <= $t_end_ind_idx_; $j ++ ) {
            $this_model_value_ += ( $dwords_[ $wt_idx_to_uniq_idx_map_{$j} + $offset_ ] * $model_coeff_vec_[$j] );
          }
        }
        else {
          for ( my $j = $t_start_ind_idx_; $j <= $t_end_ind_idx_; $j ++ ) {
            $this_model_value_ += ( 1 / ( 1 + exp ( - $dwords_[ $wt_idx_to_uniq_idx_map_{$j} + $offset_ ] * $model_coeff_alpha_vec_[$j] ) ) - 0.5 ) * $model_coeff_beta_vec_[$j];
          }		
        }

        if ( abs ( $this_model_value_ ) < 1000000 * $min_price_increment_ ) {
          $abs_sum_l1 += abs ($this_model_value_);
          $sum_l1 +=  $this_model_value_ ;
          $sum_l2 += $this_model_value_ * $this_model_value_ ;
          $count ++;

          $abs_sum_l1_vec_[$t_reg_model_num_ - 1] += abs ($this_model_value_);
          $sum_l1_vec_[$t_reg_model_num_ - 1] +=  $this_model_value_ ;
          $sum_l2_vec_[$t_reg_model_num_ - 1] += $this_model_value_ * $this_model_value_ ;
          $count_vec_[$t_reg_model_num_ - 1] += 1;
        }
        else {
          $fil_lines_++;
          $fil_lines_vec_[$t_reg_model_num_] += 1;
        }
      }
    }
    close DFILE;
    `rm -f $this_day_timed_data_filename_`;
  }
}

my $current_model_stdev_ = -1;
my $current_model_l1norm_ = -1;
my $filter_per_ = -1;
if ( $count > 1 ) {
    $current_model_stdev_ = sqrt ( ( $sum_l2 - ( $sum_l1 * $sum_l1 / $count ) ) / ($count -1) );
    $current_model_l1norm_ = $abs_sum_l1 / $count ;
    $filter_per_ = 100 * $fil_lines_ / ( $count + $fil_lines_ );
}
printf "%.8f %.8f %f\n",$current_model_stdev_,$current_model_l1norm_, $filter_per_;

if ( $#count_vec_ > 0 ) {
  print "RegimeNo Stdev L1Norm FilteredFraction RegimeFraction\n";

  for ( my $i=0; $i<=$#count_vec_; $i++) {
    my $current_model_stdev_ = -1;
    my $current_model_l1norm_ = -1;
    my $filter_per_ = -1;
    my $contri_ = -1;
    if ( $count_vec_[$i] > 1 ) {
        $current_model_stdev_ = sqrt ( ( $sum_l2_vec_[$i] - ( $sum_l1_vec_[$i] * $sum_l1_vec_[$i] / $count_vec_[$i] ) ) / ($count_vec_[$i] -1) );
        $current_model_l1norm_ = $abs_sum_l1_vec_[$i] / $count_vec_[$i] ;
        $filter_per_ = 100 * $fil_lines_vec_[$i] / ( $count_vec_[$i] + $fil_lines_vec_[$i] );
        $contri_ = $count_vec_[$i]/$count;
    }
    printf "REGIME %d: %.8f %.8f %f %f\n",$i + 1, $current_model_stdev_,$current_model_l1norm_, $filter_per_, $contri_;
  }
}
`rm -f $temp_model_filename_`;
