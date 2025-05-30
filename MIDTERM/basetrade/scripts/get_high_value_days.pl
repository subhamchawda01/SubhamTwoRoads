#!/usr/bin/perl
# \file scripts/get_high_value_days.pl
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

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

my $ALL_MDS_ON_DAY_EXEC=$BIN_DIR."/get_all_mds_stats_for_day";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/no_data_date.pl"; #NoDataDateForShortcode
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE="$0 shortcode_1 shortcode_2 shortcode_3 ... shortcode_n output_files_dir days_to_look_behind high_value_days_to_print";

if ($#ARGV < 3) { 
    printf "$USAGE\n"; 
    exit (0); 
}

my @shortcode_list_ = ();

for (my $arg_ = 0; $arg_ < $#ARGV - 2; $arg_++) {
    my $shortcode_ = $ARGV[$arg_]; chomp ($shortcode_);
    push (@shortcode_list_, $shortcode_);
}

my $output_files_dir_ = $ARGV[$#ARGV - 2];

my $yyyymmdd_=`date +%Y%m%d`; chomp ($yyyymmdd_);
$yyyymmdd_=CalcPrevWorkingDateMult($yyyymmdd_,1);
my $days_to_look_behind_ = $ARGV[$#ARGV - 1];
my $days_to_print_ = $ARGV[$#ARGV];

for (my $shortcode_ = 0; $shortcode_ <= $#shortcode_list_; $shortcode_++) {
    my $current_yyyymmdd_ = $yyyymmdd_;
    my %date_to_info_ = ();

    for (my $days_ = $days_to_look_behind_; $days_ != 0; $days_--) {
	if (!ValidDate ($current_yyyymmdd_)) { # Should not be here.
	    printf ("Invalid date : $current_yyyymmdd_\n");
	    last;
	}
	
	if (SkipWeirdDate ($current_yyyymmdd_) ||
	    IsDateHoliday ($current_yyyymmdd_) ||
            NoDataDateForShortcode ($current_yyyymmdd_, $shortcode_list_[$shortcode_]) ||
            IsProductHoliday ($current_yyyymmdd_, $shortcode_list_[$shortcode_])) {
	    $current_yyyymmdd_ = CalcPrevWorkingDateMult ($current_yyyymmdd_, 1);
	    $days_++;
	    next;
	}
	
	my $exec_cmd_ = $ALL_MDS_ON_DAY_EXEC." $shortcode_list_[$shortcode_] $current_yyyymmdd_";
	my @output_ = `$exec_cmd_`; #chomp (@output_);
	if (  $#output_ < 8 )
	{
	    $date_to_info_{$current_yyyymmdd_} = "-1";
	    $current_yyyymmdd_ = CalcPrevWorkingDateMult ($current_yyyymmdd_, 1);
	    next;
	}
	my @avg_tr_px_ = split (' ', $output_[8]);
	if ( $#avg_tr_px_ < 1 )
	{
	    $date_to_info_{$current_yyyymmdd_} = "-1";
	    $current_yyyymmdd_ = CalcPrevWorkingDateMult ($current_yyyymmdd_, 1);
	    next;
	}
	if ($avg_tr_px_[1] > 0) {
	    my $atp_ = $avg_tr_px_[1]; chomp ($atp_);
	    $date_to_info_{$current_yyyymmdd_} = $atp_;
	}

	$current_yyyymmdd_ = CalcPrevWorkingDateMult ($current_yyyymmdd_, 1);
    }

    my $output_file_name_ = $output_files_dir_."/"."high_value_days.".$shortcode_list_[$shortcode_];
    open HIGH_VAL_FILE_, ">", $output_file_name_ or PrintStacktraceAndDie ( "Could not open file : $output_file_name_\n" );
    my $days_printed_ = $days_to_print_;

    foreach my $date_ (sort {$date_to_info_{$b} <=> $date_to_info_{$a}}
		      keys %date_to_info_)
    {
	print HIGH_VAL_FILE_ "$date_ $date_to_info_{$date_}\n";

	$days_printed_--;
	if ($days_printed_ == 0) {
	    last;
	}
    }

    close (HIGH_VAL_FILE_);
}
