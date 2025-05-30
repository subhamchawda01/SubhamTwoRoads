#!/usr/bin/perl

# \file scripts/get_bad_days.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
use strict;
use warnings;

my $USER     = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'};

my $REPO = "basetrade";

my $GENPERLLIB_DIR = $HOME_DIR . "/" . $REPO . "_install/GenPerlLib";

require "$GENPERLLIB_DIR/valid_date.pl";                     # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl";                # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl";                # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl";    # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/no_data_date.pl";                   # NoDataDateForShortcode
require "$GENPERLLIB_DIR/print_stacktrace.pl";               # PrintStacktrace , PrintStacktraceAndDie

my $USAGE =
  "$0 shortcode_1 shortcode_2 shortcode_3 ... shortcode_n output_files_dir days_to_look_behind bad_days_to_print";

if ( $#ARGV < 3 ) {
  printf "$USAGE\n";
  exit(0);
}

my @shortcode_list_ = ();

for ( my $arg_ = 0 ; $arg_ < $#ARGV - 2 ; $arg_++ ) {
  my $shortcode_ = $ARGV[$arg_];
  chomp($shortcode_);
  push( @shortcode_list_, $shortcode_ );
}

my $output_files_dir_ = $ARGV[ $#ARGV - 2 ];

my $yyyymmdd_ = `date +%Y%m%d`;
chomp($yyyymmdd_);
$yyyymmdd_ = CalcPrevWorkingDateMult( $yyyymmdd_, 1 );
my $days_to_look_behind_ = $ARGV[ $#ARGV - 1 ];
my $days_to_print_       = $ARGV[$#ARGV];

my $GLOBALRESULTSDBDIR = "/NAS1/ec2_globalresults";    # Changed for DVCTrader ... so that it does not clash with others

for ( my $shortcode_ = 0 ; $shortcode_ <= $#shortcode_list_ ; $shortcode_++ ) {
  my $current_yyyymmdd_ = $yyyymmdd_;
  my %date_to_pnl_      = ();

  for ( my $days_ = $days_to_look_behind_ ; $days_ != 0 ; $days_-- ) {
    if ( !ValidDate($current_yyyymmdd_) ) {            # Should not be here.
      printf("Invalid date : $current_yyyymmdd_\n");
      last;
    }

    if ( SkipWeirdDate($current_yyyymmdd_)
      || IsDateHoliday($current_yyyymmdd_)
      || NoDataDateForShortcode( $current_yyyymmdd_, $shortcode_list_[$shortcode_] )
      || IsProductHoliday( $current_yyyymmdd_, $shortcode_list_[$shortcode_] ) )
    {
      $current_yyyymmdd_ = CalcPrevWorkingDateMult( $current_yyyymmdd_, 1 );
      $days_++;
      next;
    }

    {    # Read in results from global results for this shortcode.
      my $result_count_ = 0;

      my @results_vec_vec_ = ();
      GetGlobalResultsForShortcodeDate( $shortcode_list_[$shortcode_],
        $current_yyyymmdd_, \@results_vec_vec_, $GLOBALRESULTSDBDIR );

      for ( my $result_line_idx_ = 0 ; $result_line_idx_ <= $#results_vec_vec_ ; $result_line_idx_++ ) {
        my $t_pnl_ = GetPnlFromGlobalResultVecRef( $results_vec_vec_[$result_line_idx_] );
        my $t_vol_ = GetVolFromGlobalResultVecRef( $results_vec_vec_[$result_line_idx_] );

        if ( $t_vol_ != 0 ) {
          $date_to_pnl_{$current_yyyymmdd_} += $t_pnl_;
          $result_count_++;
        }
      }

      if ( exists $date_to_pnl_{$current_yyyymmdd_} ) {
        $date_to_pnl_{$current_yyyymmdd_} /= $result_count_;
      }
    }

    $current_yyyymmdd_ = CalcPrevWorkingDateMult( $current_yyyymmdd_, 1 );
  }

  # Create the bad_days & very_bad_days files and dump out the dates.
  my $bad_day_file_name_ = $output_files_dir_ . "/" . "bad_days." . $shortcode_list_[$shortcode_];
  open BAD_DAYS_FILE_, ">", $bad_day_file_name_ or PrintStacktraceAndDie("Could not open file : $bad_day_file_name_\n");
  my $very_bad_day_file_name_ = $output_files_dir_ . "/" . "very_bad_days." . $shortcode_list_[$shortcode_];
  open VERY_BAD_DAYS_FILE_, ">", $very_bad_day_file_name_
    or PrintStacktraceAndDie("Could not open file : $very_bad_day_file_name_\n");

  my $days_printed_ = $days_to_print_;

  foreach my $date_ (
    sort { $date_to_pnl_{$a} <=> $date_to_pnl_{$b} }
    keys %date_to_pnl_
    )
  {
    print BAD_DAYS_FILE_ "$date_ $date_to_pnl_{$date_}\n";

    if ( $days_printed_ >= $days_to_print_ / 2 ) {
      print VERY_BAD_DAYS_FILE_ "$date_ $date_to_pnl_{$date_}\n";
    }

    $days_printed_--;
    if ( $days_printed_ == 0 ) {
      last;
    }
  }

  close(VERY_BAD_DAYS_FILE_);
  close(BAD_DAYS_FILE_);
}
