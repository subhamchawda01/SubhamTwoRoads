#!/usr/bin/perl
use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";
my $MDS_L1_EXEC=$HOME_DIR."/infracore_install/bin/mds_log_l1_trade";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1 # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD

my $USAGE="$0 SHORTCODE_1 SHORTCODE_2 SHORTCODE_3 ... SHORTCODE_N OUTPUT_FILES_DIR START_DATE END_DATE APPEND_TO_EXISTING_FILE(1/0)";

if ($#ARGV < 3) { 
    printf "$USAGE\n"; 
    exit (0); 
}

my @shortcode_list_ = ();

for (my $arg_ = 0; $arg_ < $#ARGV - 3; $arg_++) {
    my $shortcode_ = $ARGV[$arg_]; chomp ($shortcode_);
    push (@shortcode_list_, $shortcode_);
}

my $output_files_dir_ = $ARGV[$#ARGV - 3];

my $start_yyyymmdd_ = GetIsoDateFromStrMin1 ($ARGV[$#ARGV - 2]);
my $end_yyyymmdd_ = GetIsoDateFromStrMin1 ($ARGV[$#ARGV - 1]);
my $is_append_ = $ARGV[$#ARGV];

if ($start_yyyymmdd_ > $end_yyyymmdd_) {
    printf "Error start $start_yyyymmdd_ > end $end_yyyymmdd_\n";
    exit (0);
}

for (my $shortcode_ = 0; $shortcode_ <= $#shortcode_list_; $shortcode_++) {
    my %date_to_bid_px_ = ();
    my %date_to_bid_sz_ = ();
    my %date_to_bid_ord_ = ();
    my %date_to_ask_px_ = ();
    my %date_to_ask_sz_ = ();
    my %date_to_ask_ord_ = ();

    for (my $current_yyyymmdd_ = $end_yyyymmdd_; $current_yyyymmdd_ >= $start_yyyymmdd_; ) {
	if (!ValidDate ($current_yyyymmdd_)) { # Should not be here.
	    printf ("Invalid date : $current_yyyymmdd_\n");
	    exit (0);
	}
	
	if (SkipWeirdDate ($current_yyyymmdd_) ||
	    IsDateHoliday ($current_yyyymmdd_)) {
	    $current_yyyymmdd_ = CalcPrevWorkingDateMult ($current_yyyymmdd_, 1);
	    next;
	}

	my $mds_l1_exec_cmd_ = $MDS_L1_EXEC." $shortcode_list_[$shortcode_] $current_yyyymmdd_ | /bin/grep L1 | /usr/bin/tail -1";
	my $last_l1_line_ = `$mds_l1_exec_cmd_`; chomp ($last_l1_line_);

	# Format is :
	# L1 TimeStamp BS BO BP AP AO AS
	my @l1_line_words_ = split (' ', $last_l1_line_);

	if ($#l1_line_words_ == 7) {
	    $date_to_bid_sz_{$current_yyyymmdd_} = $l1_line_words_[2];
	    $date_to_bid_ord_{$current_yyyymmdd_} = $l1_line_words_[3];
	    $date_to_bid_px_{$current_yyyymmdd_} = $l1_line_words_[4];
	    $date_to_ask_px_{$current_yyyymmdd_} = $l1_line_words_[5];
	    $date_to_ask_ord_{$current_yyyymmdd_} = $l1_line_words_[6];
	    $date_to_ask_sz_{$current_yyyymmdd_} = $l1_line_words_[7];
	} else { # Error/MDS not available
	    $date_to_bid_sz_{$current_yyyymmdd_} = 0;
	    $date_to_bid_ord_{$current_yyyymmdd_} = 0;
	    $date_to_bid_px_{$current_yyyymmdd_} = 0;
	    $date_to_ask_px_{$current_yyyymmdd_} = 0;
	    $date_to_ask_ord_{$current_yyyymmdd_} = 0;
	    $date_to_ask_sz_{$current_yyyymmdd_} = 0;
	}

	$current_yyyymmdd_ = CalcPrevWorkingDateMult ($current_yyyymmdd_, 1);
    }

    # Create the last prices file and dump out prices there.
    my $output_file_name_ = $output_files_dir_."/"."last_prices.".$shortcode_list_[$shortcode_];
    if ($is_append_ == 1) {
	open LAST_PX_FILE_, ">>", $output_file_name_ or die "Could not open file : $output_file_name_\n";
    } else {
	open LAST_PX_FILE_, ">", $output_file_name_ or die "Could not open file : $output_file_name_\n";
    }

    foreach my $date_ (sort {$a <=> $b} keys %date_to_bid_px_)
    {
	print LAST_PX_FILE_ "$date_ $date_to_bid_sz_{$date_} $date_to_bid_ord_{$date_} $date_to_bid_px_{$date_} $date_to_ask_px_{$date_} $date_to_ask_ord_{$date_} $date_to_ask_sz_{$date_}\n";
    }

    close (LAST_PX_FILE_);
}
