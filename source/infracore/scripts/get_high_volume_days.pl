#!/usr/bin/perl
use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";
my $VOLUME_ON_DAY_EXEC=$HOME_DIR."/infracore_install/bin/get_volume_on_day";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

my $USAGE="$0 shortcode_1 shortcode_2 shortcode_3 ... shortcode_n output_files_dir days_to_look_behind high_volume_days_to_print";

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
my $days_to_look_behind_ = $ARGV[$#ARGV - 1];
my $days_to_print_ = $ARGV[$#ARGV];

for (my $shortcode_ = 0; $shortcode_ <= $#shortcode_list_; $shortcode_++) {
    my $current_yyyymmdd_ = $yyyymmdd_;
    my %date_to_volumes_ = ();

    for (my $days_ = $days_to_look_behind_; $days_ != 0; $days_--) {
	if (!ValidDate ($current_yyyymmdd_)) { # Should not be here.
	    printf ("Invalid date : $yyyymmdd_\n");
	    exit (0);
	}
	
	if (SkipWeirdDate ($current_yyyymmdd_) ||
	    IsDateHoliday ($current_yyyymmdd_)) {
	    $current_yyyymmdd_ = CalcPrevWorkingDateMult ($current_yyyymmdd_, 1);
	    $days_++;
	    next;
	}
	
	my $volume_exec_cmd_ = $VOLUME_ON_DAY_EXEC." $shortcode_list_[$shortcode_] $current_yyyymmdd_";

	my $volume_string_ = `$volume_exec_cmd_`; chomp ($volume_string_);
	
	# Exec returns output : "shortcode_ exchange_symbol_ traded_volume_"
	my @volume_string_output_ = split (' ', $volume_string_);

	if ($volume_string_output_ [ $#volume_string_output_ ] != -1) {
	    my $volume_ = $volume_string_output_ [ $#volume_string_output_ ]; chomp ($volume_);
	    $date_to_volumes_{$current_yyyymmdd_} = $volume_;
	}

	$current_yyyymmdd_ = CalcPrevWorkingDateMult ($current_yyyymmdd_, 1);
    }

    # Create the high_volume file and dump out top volume days.
    my $output_file_name_ = $output_files_dir_."/"."high_volume_days.".$shortcode_list_[$shortcode_];
    open HIGH_VOL_FILE_, ">", $output_file_name_ or die "Could not open file : $output_file_name_\n";

    my $days_printed_ = $days_to_print_;

    foreach my $date_ (sort {$date_to_volumes_{$b} <=> $date_to_volumes_{$a}}
		      keys %date_to_volumes_)
    {
	print HIGH_VOL_FILE_ "$date_ $date_to_volumes_{$date_}\n";

	$days_printed_--;
	if ($days_printed_ == 0) {
	    last;
	}
    }

    close (HIGH_VOL_FILE_);
}
