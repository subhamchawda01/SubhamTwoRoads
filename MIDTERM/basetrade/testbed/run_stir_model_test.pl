#!/usr/bin/perl

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

my $USAGE="$0 trade_date";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $end_date = $ARGV[0];

my $no_of_days_till_expiry_F_0 = 0;
my $no_of_days_till_expiry_F_1 = 0;
my $no_of_days_till_expiry_N_1 = 0;

my $expiry_date_F_0 = 20140101;
my $expiry_date_F_1 = 20150101;
my $expiry_date_N_1 = 20140701;

$no_of_days_till_expiry_F_0 = CalculateNumberOfDays( "DI1F15" , $expiry_date_F_0);
$no_of_days_till_expiry_F_1 = CalculateNumberOfDays( "DI1F16" , $expiry_date_F_1);
$no_of_days_till_expiry_N_1 = CalculateNumberOfDays( "DI1N15", $expiry_date_N_1);

`~/basetrade_install/bin/mult_price_printer $end_date 1200 2000 DI1N15 DI1F15 DI1F16 > ~/di_price.n1.0.1`;
my @result_lines = `~/basetrade/testbed/stir_model_test.R ~/di_price.n1.0.1 $no_of_days_till_expiry_F_0 $no_of_days_till_expiry_F_1 $no_of_days_till_expiry_N_1`;
print @result_lines;
exit ( 0 );

sub CalculateNumberOfDays
{
    my $shc_ = shift;
    my $expiry_date = shift;
    my $datagen_date_ = $expiry_date;
    my $max_days_at_a_time_ = 2000;
    my $no_of_days_till_expiry = 0;

    for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_ ++ )
    {
        if ( $datagen_date_ < $end_date ) { last ; }

        #print $main_log_file_handle_ "Considering datagen day $datagen_date_ \n";
        if ( ! ValidDate ( $datagen_date_ ) )
        {
            #last;
        }

        if ( SkipWeirdDate ( $datagen_date_ ) ||
             NoDataDateForShortcode ( $datagen_date_ , $shc_ ) ||
             ( IsDateHoliday ( $datagen_date_ ) || ( ( $shc_ ) && ( IsProductHoliday ( $datagen_date_, $shc_ ) ) ) ) )
        {
            $datagen_date_ = CalcPrevWorkingDateMult ( $datagen_date_, 1 );
            next;
        }

        $datagen_date_ = CalcPrevWorkingDateMult ( $datagen_date_, 1 );
        $no_of_days_till_expiry ++ ;
    }
print "$no_of_days_till_expiry\n";
return $no_of_days_till_expiry ;
}


