#!/usr/bin/perl
use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";

my $BASE_INFRACORE_BIN_DIR=$HOME_DIR."/infracore_install/bin";
my $L1_EXEC_BIN=$BASE_INFRACORE_BIN_DIR."/mds_log_l1_trade";
my $EXCHANGE_SYMBOL_EXEC=$BASE_INFRACORE_BIN_DIR."/get_exchange_symbol";

my $BASE_ORS_TRADES_DIR="/apps/logs/ORSTrades";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

my $USAGE="$0 shortcode days_to_look_behind";

if ($#ARGV < 1) { 
    printf "$USAGE\n"; 
    exit (0); 
}

my $yyyymmdd_=`date +%Y%m%d`; chomp ($yyyymmdd_);

my $shortcode_ = $ARGV[0]; chomp ($shortcode_);
my $days_to_look_behind_ = $ARGV[1];

my %date_to_sum_trade_count_ = ();
my %date_to_sum_trade_sizes_ = ();
my %date_to_exchange_symbol_ = ();

for (my $days_ = $days_to_look_behind_; $days_ != 0; $days_--) {
    if (!ValidDate ($yyyymmdd_)) { # Should not be here.
	printf ("Invalid date : $yyyymmdd_\n");
	exit (0);
    }
    
    if (SkipWeirdDate ($yyyymmdd_) ||
	IsDateHoliday ($yyyymmdd_)) {
	$yyyymmdd_ = CalcPrevWorkingDateMult ($yyyymmdd_, 1);
	$days_++;
	next;
    }

    # Use the get_exchange_symbol exec to get the shortcode->exch_symbol mapping for this date.
    my $exchange_symbol_ = `$EXCHANGE_SYMBOL_EXEC $shortcode_ $yyyymmdd_`; chomp ($exchange_symbol_);

    $date_to_exchange_symbol_{$yyyymmdd_} = $exchange_symbol_;

    # Using grep should strictly be avoided, but reduces the # of lines greatly.
    my @trade_lines_ = `$L1_EXEC_BIN $shortcode_ $yyyymmdd_ | grep TRADE`;

    for (my $i_ = 0; $i_ <= $#trade_lines_; $i_++) {
	# Output is of the form:
	# TRADE TIMESTAMP SIZE PRICE
	my @trade_line_words_ = split (' ', $trade_lines_[$i_]);

	my $trade_size_ = $trade_line_words_[2]; chomp ($trade_size_);

	$date_to_sum_trade_sizes_{$yyyymmdd_} += $trade_size_;
	$date_to_sum_trade_count_{$yyyymmdd_}++;
    }

    $yyyymmdd_ = CalcPrevWorkingDateMult ($yyyymmdd_, 1);
}

my $sum_trade_size_ = 0.0;
my $sum_trade_count_ = 0.0;

foreach my $date_ (sort keys %date_to_sum_trade_sizes_) {
    my $avg_trade_size_ = $date_to_sum_trade_sizes_{$date_} / $date_to_sum_trade_count_{$date_};

    $sum_trade_size_ += $date_to_sum_trade_sizes_{$date_};
    $sum_trade_count_ += $date_to_sum_trade_count_{$date_};

    printf ("$shortcode_ | $date_to_exchange_symbol_{$date_} | $date_ | sum_trade_size_ : %6d | trade_count_ : %6d | avg_trade_size_ : %4.2lf\n", $date_to_sum_trade_sizes_{$date_}, $date_to_sum_trade_count_{$date_}, $avg_trade_size_);
}

my $avg_trade_size_ = $sum_trade_size_ / $sum_trade_count_;

printf ("\n\t\t\t---- SUMMARY ----\n");
printf ("$shortcode_ | sum_trade_size_ : %6d | trade_count_ : %6d | avg_trade_size_ : %4.2lf\n\n", $sum_trade_size_, $sum_trade_count_, $avg_trade_size_);

exit (0);
