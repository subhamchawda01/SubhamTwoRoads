#!/usr/bin/perl

# \file scripts/update_spot_future_indicator_info.pl
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
use POSIX;
use List::Util qw[min max]; # max , min

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $dep_shortcode_ = "";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl";
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE =  "$0 date";

if ( $#ARGV < 0 ) 
{ 
    printf "$USAGE\n"; 
    exit ( 0 ); 
}

my $date_ = $ARGV[0];
my $start_time_ = "EST_800";
my $end_time_ = "EST_1300";


if ( SkipWeirdDate ( $date_ ) || IsDateHoliday ( $date_ ) )
{
    exit(0);
}


my $indicator_list_ = "/spare/local/$USER/tmp_ilist";

#building empty ilist for stdev calculation of dependent
my $cmd_ = "echo \"MODELINIT DEPBASE $dep_shortcode_ MktSizeWPrice MktSizeWPrice\" >> $indicator_list_";
`$cmd_`;
$cmd_ = "echo \"MODELMATH LINEAR CHANGE\" >> $indicator_list_";
`$cmd_`;
$cmd_ = "echo \"INDICATORSTART\" >> $indicator_list_";
`$cmd_`;
$cmd_ = "echo \"INDICATOR SimpleTrend USD000UTSTOM 30.0\" >> $indicator_list_";
`$cmd_`;
$cmd_ = "echo \"INDICATOR SimpleTrend Si_0 30.0\" >> $indicator_list_";
`$cmd_`;
$cmd_ = "echo \"INDICATOREND\" >> $indicator_list_";
`$cmd_`;

my $tmp_dgen_filename_ = "/spare/local/".$USER."/tmp_dgen";

my $tmp_diff_rates_filename_ = "/spare/local/".$USER."/tmp_prices";

my $first_shortcode_ = "USD000UTSTOM";
my $second_shortcode_ = "Si_0";

$cmd_ = "~/basetrade_install/bin/get_exchange_symbol $second_shortcode_ $date_";
my $exchange_symbol_ = `$cmd_`;
chomp ( $exchange_symbol_ );

my $expiry_date_ = 20110101; # dummy date

if ( substr ( $exchange_symbol_, 2, 2 ) eq "Z3" )
{
   $expiry_date_ = "20131216";
}
elsif ( substr ( $exchange_symbol_, 2, 2 ) eq "Z4" )
{
   $expiry_date_ = "20141215";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "X3" )
{
   $expiry_date_ = "20131115";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "X4" )
{
   $expiry_date_ = "20141117";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "V3" )
{
   $expiry_date_ = "20131015";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "V4" )
{
   $expiry_date_ = "20141015";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "U3" )
{
   $expiry_date_ = "20130916";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "U4" )
{ 
   $expiry_date_ = "20140915";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "Q3" )
{
   $expiry_date_ = "20130815";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "Q4" )
{
   $expiry_date_ = "20140815";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "N3" )
{
   $expiry_date_ = "20130715";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "N4" )
{
   $expiry_date_ = "20140715";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "M3" )
{
   $expiry_date_ = "20130617";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "M4" )
{
   $expiry_date_ = "20140615";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "K4" )
{
   $expiry_date_ = "20140515";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "J4" )
{
   $expiry_date_ = "20140415";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "H4" )
{
   $expiry_date_ = "20140317";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "G4" )
{
   $expiry_date_ = "20140217";
}
elsif ( substr ( $exchange_symbol_, 2 , 2 ) eq "F4" )
{
   $expiry_date_ = "20140115";
}
else
{
  # TODO exit saying the reason
}

my $current_date_ = $expiry_date_;

my $time_to_expiry_ = 0;

while ( $current_date_ ne $date_ )
{
   $cmd_ = "~/basetrade_install/bin/calc_prev_day $current_date_";
   $time_to_expiry_ ++;   
   $current_date_ = `$cmd_`;
   chomp ( $current_date_);
}

$cmd_ = "~/basetrade_install/bin/mult_price_printer $date_ $start_time_ $end_time_ Midprice $first_shortcode_ $second_shortcode_ | awk 'BEGIN{sum=0;}{ if(\$2 > 0 && \$3 > 0 ) { sum+=(\$2*1000 - \$3)/(\$3*$time_to_expiry_) }  }END{print sum/NR}'";

my $diff_daily_interest_rates_ = `$cmd_`;
chomp($diff_daily_interest_rates_);

$diff_daily_interest_rates_ = $diff_daily_interest_rates_ + 0.0;

my $spot_future_indicator_info_filename_ = "/spare/local/tradeinfo/SpotFutureIndicatorInfo/spot_future_indicator_offline_info.txt"; 

$cmd_ = "cat $spot_future_indicator_info_filename_ | head -n1 | awk '{print \$4}'";
my $earlier_diff_daily_interest_rates_ = `$cmd_`;
chomp ( $earlier_diff_daily_interest_rates_ );

if ( abs ( $diff_daily_interest_rates_ - $earlier_diff_daily_interest_rates_ )*365 > 0.01  )
{
    $diff_daily_interest_rates_ = $earlier_diff_daily_interest_rates_;     
}

my $mail_body_ = "For Date $date_ \nDefault Interest Rate Diff : $earlier_diff_daily_interest_rates_\nCurrent Interest Rate Diff : $diff_daily_interest_rates_\n";

open(MAIL, "|/usr/sbin/sendmail -t");

my $hostname_=`hostname`;

my $mail_address_ = "nseall@tworoads.co.in";

print MAIL "To: $mail_address_\n";
print MAIL "From: $mail_address_\n";
print MAIL "Subject: Spot Future Indicator Info Alert\n";
print MAIL $mail_body_ ;


$cmd_ = "sed -i '/$date_/d' $spot_future_indicator_info_filename_";
`$cmd_`;

my $next_working_date_ = CalcNextWorkingDateMult ( $date_, 1 ); 
print $next_working_date_."\n";

$cmd_ = "sed -i '1 s/^.*\$/DEFAULT $first_shortcode_ $second_shortcode_ $diff_daily_interest_rates_ /g' $spot_future_indicator_info_filename_";
`$cmd_`;
$cmd_ = "echo \"$next_working_date_ $first_shortcode_ $second_shortcode_ $diff_daily_interest_rates_ \" >> $spot_future_indicator_info_filename_";
`$cmd_`;
$diff_daily_interest_rates_ = -$diff_daily_interest_rates_;
$cmd_ = "sed -i '2 s/^.*\$/DEFAULT $second_shortcode_ $first_shortcode_ $diff_daily_interest_rates_ /g' $spot_future_indicator_info_filename_";
`$cmd_`;
$cmd_ = "echo \"$next_working_date_ $second_shortcode_ $first_shortcode_ $diff_daily_interest_rates_ \" >> $spot_future_indicator_info_filename_";
`$cmd_`;
