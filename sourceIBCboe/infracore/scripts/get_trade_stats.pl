#!/usr/bin/perl
use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";

my $BASE_INFRACORE_BIN_DIR=$HOME_DIR."/infracore_install/bin";
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

# These values to be set in setExchangeProfiles.
my $exchange_ = "";
my @exchange_profile_list_ = ();

setExchangeProfiles ();

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

    for (my $profile_no_ = 0; $profile_no_ <= $#exchange_profile_list_; $profile_no_++) {
	# ORS Trades file name:
	# /apps/logs/ORSTrades/CME/HC0/2012/01/09/trades.20120109
	my $ors_trades_file_name_ = $BASE_ORS_TRADES_DIR."/".$exchange_."/".$exchange_profile_list_[$profile_no_]."/".substr ($yyyymmdd_, 0, 4)."/".substr ($yyyymmdd_, 4, 2)."/".substr ($yyyymmdd_, 6, 2)."/trades.".$yyyymmdd_;

	# Open the corresponding trade file and get the trade sizes & count.
	open ORS_TRADES_FILE_HANDLE, "< $ors_trades_file_name_" or next;
	my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;
	close ORS_TRADES_FILE_HANDLE;

	for (my $i = 0; $i <= $#ors_trades_file_lines_; $i ++) {
	    my @words_ = split ('', $ors_trades_file_lines_[$i]);
	    if ($#words_ >= 4)
	    {
		my $symbol_ = $words_[0];
		my $tsize_ = $words_[2];

		if ($symbol_ ne $exchange_symbol_) {
		    next;
		}

		$date_to_sum_trade_sizes_{$yyyymmdd_} += $tsize_;
		$date_to_sum_trade_count_{$yyyymmdd_}++;
	    }
	}
    }

    $yyyymmdd_ = CalcPrevWorkingDateMult ($yyyymmdd_, 1);
}

my $sum_trade_size_ = 0.0;
my $sum_trade_count_ = 0.0;

foreach my $date_ (sort keys %date_to_sum_trade_sizes_) {
    my $avg_trade_size_ = $date_to_sum_trade_sizes_{$date_} / $date_to_sum_trade_count_{$date_};

    $sum_trade_size_ += $date_to_sum_trade_sizes_{$date_};
    $sum_trade_count_ += $date_to_sum_trade_count_{$date_};

    printf ("$shortcode_ | $date_to_exchange_symbol_{$date_} | $date_ | sum_trade_size_ : %5d | trade_count_ : %4d | avg_trade_size_ : %0.2lf\n", $date_to_sum_trade_sizes_{$date_}, $date_to_sum_trade_count_{$date_}, $avg_trade_size_);
}

my $avg_trade_size_ = $sum_trade_size_ / $sum_trade_count_;

printf ("\n\t\t\t---- SUMMARY ----\n");
printf ("$shortcode_ | sum_trade_size_ : %5d | trade_count_ : %4d | avg_trade_size_ : %0.2lf\n\n", $sum_trade_size_, $sum_trade_count_, $avg_trade_size_);

exit (0);

# These values to be set in setExchangeProfiles.
# my $exchange_ = "";
# my @exchange_profile_list_ = ();
sub setExchangeProfiles
{
    if (index ($shortcode_, "ES") == 0 ||
	index ($shortcode_, "UB") == 0 ||
	index ($shortcode_, "ZB") == 0 ||
	index ($shortcode_, "ZN") == 0 ||
	index ($shortcode_, "ZF") == 0 ||
	index ($shortcode_, "ZT") == 0) {
	$exchange_ = "CME";
	push (@exchange_profile_list_, "HC0");
	push (@exchange_profile_list_, "G52");
	push (@exchange_profile_list_, "VD4");
	push (@exchange_profile_list_, "J55");
    } elsif (index ($shortcode_, "FGBS") == 0 ||
	     index ($shortcode_, "FGBM") == 0 ||
	     index ($shortcode_, "FGBL") == 0 ||
	     index ($shortcode_, "FESX") == 0 ||
	     index ($shortcode_, "FGBX") == 0 ||
	     index ($shortcode_, "FOAT") == 0 ||
	     index ($shortcode_, "FDAX") == 0) {
	$exchange_ = "EUREX";
	push (@exchange_profile_list_, "NTAPROD3");
	push (@exchange_profile_list_, "NTAPROD2");
    } elsif (index ($shortcode_, "CGB") == 0 ||
	     index ($shortcode_, "SXF") == 0 ||
	     index ($shortcode_, "BAX") == 0) {
	$exchange_ = "TMX";
	push (@exchange_profile_list_, "BDMA");
	push (@exchange_profile_list_, "BDMB");
    } elsif (index ($shortcode_, "BR_WIN") == 0 ||
	     index ($shortcode_, "BR_DOL") == 0 ||
	     index ($shortcode_, "BR_IND") == 0 ||
	     index ($shortcode_, "BR_WDO") == 0) {
	$exchange_ = "BMFEP";
	push (@exchange_profile_list_, "XLIN66");
	push (@exchange_profile_list_, "XLIN67");
	push (@exchange_profile_list_, "XLIN68");
    } else {
	printf ("No exchange/profile found for shortcode : $shortcode_ \n");
	exit (0);
    }
}
