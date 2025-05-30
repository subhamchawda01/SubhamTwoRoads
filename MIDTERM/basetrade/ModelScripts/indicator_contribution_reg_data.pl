#!/usr/bin/perl

# \file ModelScripts/indicator_contribution.pl
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
use feature "switch";          # for given, when
use File::Basename;            # for basename and dirname
use File::Copy;                # for copy
use List::Util qw/max min/;    # for max
use FileHandle;

my $USER       = $ENV{'USER'};
my $HOME_DIR   = $ENV{'HOME'};
my $SPARE_HOME = "/spare/local/" . $USER . "/";

my $TRADELOG_DIR   = "/spare/local/logs/tradelogs/";
my $DATAGEN_LOGDIR = "/spare/local/logs/datalogs/";

my $GENRSMWORKDIR = $SPARE_HOME . "RSM/";

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR . "/" . $REPO . "_install/ModelScripts";
my $GENPERLLIB_DIR   = $HOME_DIR . "/" . $REPO . "_install/GenPerlLib";

#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR = $HOME_DIR . "/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl";    # PrintStacktrace , PrintStacktraceAndDie

if ( $USER eq "rkumar" ) {
  $LIVE_BIN_DIR = $HOME_DIR . "/" . $REPO . "_install/bin";
}
if ( $USER eq "sghosh" || $USER eq "ravi" || $USER eq "ankit" || $USER eq "anshul" ) {
  $LIVE_BIN_DIR = $HOME_DIR . "/" . $REPO . "_install/bin";
}

require "$GENPERLLIB_DIR/is_date_holiday.pl";                # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl";                # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl";                   # NoDataDate
require "$GENPERLLIB_DIR/valid_date.pl";                     # ValidDate
require "$GENPERLLIB_DIR/calc_next_date.pl";                 # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl";                 # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl";            # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl";    # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl";     # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/check_ilist_data.pl";               #CheckIndicatorData
require "$GENPERLLIB_DIR/get_pred_counters_for_this_pred_algo.pl"; # GetPredCountersForThisPredAlgo

# start
my $USAGE = "$0 modelfile startdate enddate starthhmm endhhmm pred_duration normalizing_algo";

if ( $#ARGV < 4 ) { print $USAGE. "\n"; exit(0); }
my $model_filename_         = $ARGV[0];
my $datagen_start_yyyymmdd_ = GetIsoDateFromStrMin1( $ARGV[1] );
my $datagen_end_yyyymmdd_   = GetIsoDateFromStrMin1( $ARGV[2] );
my $datagen_start_hhmm_     = $ARGV[3];
my $datagen_end_hhmm_       = $ARGV[4];
my $this_pred_duration_     = $ARGV[5];
my $normalizing_algo_       = $ARGV[6];

my $yyyymmdd_ = `date +%Y%m%d`;
chomp($yyyymmdd_);
my $hhmmss_ = `date +%H%M%S`;
chomp($hhmmss_);

my $datagen_msecs_timeout_      = 1000;
my $datagen_l1events_timeout_   = 6;
my $datagen_num_trades_timeout_ = 0;
my $to_print_on_economic_times_ = 0;

my $unique_gsm_id_ = `date +%N`;
chomp($unique_gsm_id_);
$unique_gsm_id_ = int($unique_gsm_id_) + 0;

#my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );

my @model_coeff_vec_       = ();
my @model_coeff_alpha_vec_ = ();
my @model_coeff_beta_vec_  = ();

my $exec_cmd_        = "cat $model_filename_ | grep MODELMATH | awk '{print \$2}'";
my $model_math_line_ = `$exec_cmd_`;
chomp($model_math_line_);
my $model_type_          = $model_math_line_;
my $min_price_increment_ = 1;
my $shortcode_           = "NA";
$exec_cmd_  = "cat $model_filename_ | grep MODELINIT | awk '{print \$3}'";
$shortcode_ = `$exec_cmd_`;
chomp($shortcode_);

if ( $shortcode_ ne "NA" ) {
  $exec_cmd_            = "$LIVE_BIN_DIR/get_min_price_increment $shortcode_ $datagen_end_yyyymmdd_";
  $min_price_increment_ = `$exec_cmd_`;
  chomp($min_price_increment_);    # using single date in hope that min_inc doesnt change much
}

my $temp_model_filename_ = $model_filename_ . "_tmp_stdev_model";
`cp $model_filename_ $temp_model_filename_`;

if ( $model_type_ eq "SIGLR" ) {
  `sed -i 's/SIGLR/LINEAR/g' $temp_model_filename_`;
}

my @contri_sum_ind_ = ();
my @l1sum_ind_      = ();
my @xysum_ind_      = ();
my @l2sum_ind_      = ();
my @absl1sum_ind_   = ();

open MFILE, "< $model_filename_" or PrintStacktraceAndDie("Could not open model_filename_ $model_filename_ for reading\n");
while ( my $mline_ = <MFILE> ) {
  chomp($mline_);
  if ( $mline_ =~ /INDICATOR / ) {
    my @mwords_ = split( ' ', $mline_ );
    if ( $#mwords_ >= 2 ) {
      if ( $model_type_ eq "SIGLR" ) {
        my @t_model_coeffs_ = split( ':', $mwords_[1] );
        push( @model_coeff_alpha_vec_, $t_model_coeffs_[0] );
        push( @model_coeff_beta_vec_,  $t_model_coeffs_[1] );
      }
      else {
        push( @model_coeff_vec_, $mwords_[1] );
      }
      push( @contri_sum_ind_, 0 );
      push( @l1sum_ind_,      0 );
      push( @xysum_ind_,      0 );
      push( @l2sum_ind_,      0 );
      push( @absl1sum_ind_,   0 );
    }
  }
}
close MFILE;

my $abs_sum_l1    = 0;
my $sum_l1        = 0;
my $sum_l2        = 0;
my $count         = 0;
my $filtered_pts_ = 0;
my $low_filtered_pts_ = 0;

my $dep_and_model_file_ = $GENRSMWORKDIR."del_y_and_model_file".$datagen_start_yyyymmdd_."_".$datagen_end_yyyymmdd_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_". $unique_gsm_id_ . "_". $datagen_msecs_timeout_ . "_". $datagen_l1events_timeout_ . "_". $datagen_num_trades_timeout_;
open DEP_AND_MODEL_FILE, " > $dep_and_model_file_" or PrintStacktraceAndDie("Could not open y_model_out file $dep_and_model_file_ for writing\n" );

my $tradingdate_        = $datagen_start_yyyymmdd_;
my $max_days_at_a_time_ = 2000;
for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_++ ) {
  if ( SkipWeirdDate($tradingdate_)
    || NoDataDate($tradingdate_)
    || IsDateHoliday($tradingdate_)
    || CheckIndicatorData( $tradingdate_, $temp_model_filename_ ) )
  {
    $tradingdate_ = CalcNextWorkingDateMult( $tradingdate_, 1 );
    next;
  }

  if ( ( !ValidDate($tradingdate_) )
    || ( $tradingdate_ > $datagen_end_yyyymmdd_ ) )
  {
    last;
  }
  else {

    my $this_day_timed_data_filename_ = $GENRSMWORKDIR. "timed_data". $tradingdate_ . "_". $datagen_start_hhmm_ . "_". $datagen_end_hhmm_ . "_". $unique_gsm_id_ . "_". $datagen_msecs_timeout_ . "_". $datagen_l1events_timeout_ . "_". $datagen_num_trades_timeout_;
    $exec_cmd_ = "$LIVE_BIN_DIR/datagen $temp_model_filename_ $tradingdate_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";

    #	print $main_log_file_handle_ "$exec_cmd\n";
    `$exec_cmd_`;

    my $msecs_to_predict_ = GetPredCountersForThisPredAlgo( $shortcode_, $this_pred_duration_, $normalizing_algo_, $this_day_timed_data_filename_ ) ;
    my $this_date_reg_data_filename_ = $GENRSMWORKDIR. "reg_data". $tradingdate_ . "_" . $datagen_start_hhmm_ . "_". $datagen_end_hhmm_ . "_". $unique_gsm_id_ . "_". $datagen_msecs_timeout_ . "_". $datagen_l1events_timeout_ . "_". $datagen_num_trades_timeout_;
      
    $exec_cmd_ = "$LIVE_BIN_DIR/timed_data_to_reg_data $temp_model_filename_ $this_day_timed_data_filename_ $msecs_to_predict_ $normalizing_algo_ $this_date_reg_data_filename_";
    `$exec_cmd_`;

    my $datagen_logfile_ = $DATAGEN_LOGDIR . "log." . $yyyymmdd_ . "." . $unique_gsm_id_;
    `rm -f $datagen_logfile_`;

    if ( -e $this_date_reg_data_filename_ ) {
      open DFILE, "< $this_date_reg_data_filename_"
        or PrintStacktraceAndDie(
        "Could not open this_day_timed_data_filename_ $this_day_timed_data_filename_ for reading\n");

      #print $this_day_timed_data_filename_."\n";
      while ( my $data_line_ = <DFILE> ) {
        chomp($data_line_);
        my @dwords_ = split( ' ', $data_line_ );

        if ( $#dwords_ >= $#model_coeff_vec_ + 1 ) {
          my $huge_vals_        = 0;
          my $this_model_value_ = 0;
          my @t_ind_val_vec_    = ();

          if ( $model_type_ eq "LINEAR" ) {
            for ( my $j = 0 ; $j <= $#model_coeff_vec_ ; $j++ ) {
              my $t_ind_val_ = $dwords_[ $j + 1 ];
              $t_ind_val_ = $t_ind_val_ * $model_coeff_vec_[$j];
              if ( abs($t_ind_val_) > 10 * $min_price_increment_ ) { $huge_vals_ = 1; last; }
              push( @t_ind_val_vec_, $t_ind_val_ );
              $this_model_value_ += $t_ind_val_;
            }
          }

          if ( $model_type_ eq "SIGLR" ) {
            for ( my $j = 0 ; $j <= $#model_coeff_alpha_vec_ ; $j++ ) {
              my $t_ind_val_ = $dwords_[ $j + 1 ];
              $t_ind_val_ =
                ( 1 / ( 1 + exp( -$t_ind_val_ * $model_coeff_alpha_vec_[$j] ) ) - 0.5 ) * $model_coeff_beta_vec_[$j];
              if ( abs($t_ind_val_) > 10 * $min_price_increment_ ) { $huge_vals_ = 1; last; }
              push( @t_ind_val_vec_, $t_ind_val_ );
              $this_model_value_ += $t_ind_val_;
            }
          }

          if ( abs($this_model_value_) > 10 * $min_price_increment_
            || abs($this_model_value_) < 0.001 * $min_price_increment_ )
          {
          	# adding counter for points filtered because of being low in value
          	if ( abs($this_model_value_) < 0.001 * $min_price_increment_){
          		$low_filtered_pts_++;
          	}
          	
            $huge_vals_ = 1;
          }

          if ( $huge_vals_ == 0 ) {
            $abs_sum_l1 += abs($this_model_value_);
            $sum_l1     += $this_model_value_;
            $sum_l2     += $this_model_value_ * $this_model_value_;
            for ( my $j = 0 ; $j <= $#l1sum_ind_ ; $j++ ) {
              $contri_sum_ind_[$j] += max( -1, min( 1, $t_ind_val_vec_[$j] / $this_model_value_ ) );
              $l1sum_ind_[$j] += $t_ind_val_vec_[$j];
              $l2sum_ind_[$j]    += $t_ind_val_vec_[$j] * $t_ind_val_vec_[$j];
              $xysum_ind_[$j]    += $t_ind_val_vec_[$j] * $this_model_value_;
              $absl1sum_ind_[$j] += abs( $t_ind_val_vec_[$j] );
            }
            $count++;
          }
          else {
            $filtered_pts_++;
          }
          print DEP_AND_MODEL_FILE "$dwords_[0] $this_model_value_\n";
        }
      }
      close DFILE;
      `rm -f $this_day_timed_data_filename_`;
    }

    $tradingdate_ = CalcNextWorkingDateMult( $tradingdate_, 1 );
  }

}

close DEP_AND_MODEL_FILE;

my $exec_cmd = "$LIVE_BIN_DIR/get_dep_corr  $dep_and_model_file_";
my $current_model_correlation_ = `$exec_cmd`; chomp ( $current_model_correlation_ ) ;

`rm -rf $dep_and_model_file_`;

my $current_model_stdev_  = -1;
my $current_model_l1norm_ = -1;

#print "$count \n";

if ( $count <= 1 ) {
  $current_model_stdev_ = -1;
}
else {
  $current_model_stdev_ = sqrt( ( $sum_l2 - ( $sum_l1 * $sum_l1 / $count ) ) / ( $count - 1 ) );
  $current_model_l1norm_ = $abs_sum_l1 / $count;

  open MFILE, "< $model_filename_"
    or PrintStacktraceAndDie("Could not open model_filename_ $model_filename_ for reading\n");
  my $ind_idx_ = 0;
  while ( my $mline_ = <MFILE> ) {
    chomp($mline_);
    if ( $mline_ =~ /INDICATOR / ) {
      my @mwords_ = split( ' ', $mline_ );
      if ( $#mwords_ >= 2 ) {
        my $end_idx_ = index( $mline_, "#" );
        if ( $end_idx_ == -1 ) { $end_idx_ = length($mline_); }
        my $stdev_ = sqrt(( $l2sum_ind_[$ind_idx_] - $l1sum_ind_[$ind_idx_] * $l1sum_ind_[$ind_idx_] / $count ) / ( $count - 1 ) );
        my $corr_ = 0.0;
        if ( $stdev_ > 0 ) {
          $corr_ = ( $count * $xysum_ind_[$ind_idx_] - $l1sum_ind_[$ind_idx_] * $sum_l1 ) / sqrt( ( $count * $l2sum_ind_[$ind_idx_] - $l1sum_ind_[$ind_idx_] * $l1sum_ind_[$ind_idx_] ) * ( $count * $sum_l2 - $sum_l1 * $sum_l1 ) );
        }
        printf "%-100s # CONTRI: %2.4f\tCORR: %f\tMEAN: %f\tSTDEV: %f\tL1NORM: %f\n", substr( $mline_, 0, $end_idx_ ), 100 * ( $contri_sum_ind_[$ind_idx_] / $count ), $corr_, $l1sum_ind_[$ind_idx_] / $count, $stdev_, $absl1sum_ind_[$ind_idx_] / $count;
      }
      $ind_idx_++;
    }
  }
  close MFILE;
  # get it in percentage
  $low_filtered_pts_ = 100 * ( $low_filtered_pts_/( $filtered_pts_ + $count));
  
  $filtered_pts_ = 100 * ( $filtered_pts_ / ( $filtered_pts_ + $count ) );
}
printf "MODEL CORR: %f STDEV: %f L1NORM: %f FILTERPER: %f LOWFILTERPER: %f \n", $current_model_correlation_, $current_model_stdev_, $current_model_l1norm_, $filtered_pts_, $low_filtered_pts_;
`rm -f $temp_model_filename_`;
