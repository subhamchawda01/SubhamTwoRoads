#!/usr/bin/perl

use strict;
use warnings;

my $USER=$ENV{"USER"};
my $HOME=$ENV{"HOME"};

my $REPO = "infracore";

my $GENPERLLIB_DIR = "$HOME/$REPO/GenPerlLib";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/clear_temporary_files.pl"; # ClearTemporaryFiles
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktraceAndDie

my $USAGE="$0 START_DATE NUM_OF_PAST_DAYS";
if( $#ARGV < 1 )
{
    print "USAGE:$USAGE\n";
    exit( 0 );
}

my $end_date_ = $ARGV[ 0 ];
if( $end_date_ eq "TODAY" ) { $end_date_ = `date +%Y%m%d`; }
my $num_of_days_ = $ARGV[ 1 ];

my $EUR_TO_DOL = 1.30; # Should have a way of looking this up from the currency info file.
my $CD_TO_DOL =  0.97; # canadian dollar
my $BR_TO_DOL = 0.51; # Brazil real
my $GBP_TO_DOL = 1.57; # for liffe products

my @temporary_files_ = ();

#================================= fetching conversion rates from currency info file if available ====================#

my $curr_date_ = $end_date_ ;

my $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$curr_date_.'.txt' ;
if ( ( ! -e $curr_filename_ ) && ( -e $curr_filename_."_N" ) )
{ $curr_filename_ = $curr_filename_."_N"; }

open CURR_FILE_HANDLE, "< $curr_filename_" ;

my @curr_file_lines_ = <CURR_FILE_HANDLE>;
close CURR_FILE_HANDLE;

for ( my $itr = 0 ; $itr <= $#curr_file_lines_; $itr ++ )
{

    my @cwords_ = split ( ' ', $curr_file_lines_[$itr] );
    if ( $#cwords_ >= 1 )
      {
        my $currency_ = $cwords_[0] ;
        if ( index ( $currency_, "EURUSD" ) == 0 ){
          $EUR_TO_DOL = sprintf( "%.4f", $cwords_[1] );
        }

        if( index ( $currency_, "USDCAD" ) == 0 ){
          $CD_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
        }

        if( index ( $currency_, "USDBRL" ) == 0 ){
          $BR_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
        }
        if ( index ( $currency_, "GBPUSD" ) == 0 ){
          $GBP_TO_DOL = sprintf( "%.4f", $cwords_[1] );
        }

      }
}

#======================================================================================================================#

my %shortcode_to_margin_per_contract_ = ();

$shortcode_to_margin_per_contract_{"ZF_0"} = 990;
$shortcode_to_margin_per_contract_{"ZN_0"} = 1623;
$shortcode_to_margin_per_contract_{"ZB_0"} = 2530;
$shortcode_to_margin_per_contract_{"UB_0"} = 3960;

$shortcode_to_margin_per_contract_{"FDAX_0"} = $EUR_TO_DOL * 17857;
$shortcode_to_margin_per_contract_{"FESX_0"} = $EUR_TO_DOL * 2342;
$shortcode_to_margin_per_contract_{"FGBL_0"} = $EUR_TO_DOL * 2160;
$shortcode_to_margin_per_contract_{"FGBM_0"} = $EUR_TO_DOL * 1065;
$shortcode_to_margin_per_contract_{"FGBS_0"} = $EUR_TO_DOL * 390;
$shortcode_to_margin_per_contract_{"FOAT_0"} = $EUR_TO_DOL * 1880;
$shortcode_to_margin_per_contract_{"FBTP_0"} = $EUR_TO_DOL * 4840;
$shortcode_to_margin_per_contract_{"FBTS_0"} = $EUR_TO_DOL * 2000;
$shortcode_to_margin_per_contract_{"FGBX_0"} = $EUR_TO_DOL * 4620;

$shortcode_to_margin_per_contract_{"JFFCE_0"} = $EUR_TO_DOL * 2031;
$shortcode_to_margin_per_contract_{"KFFTI_0"} = $EUR_TO_DOL * 4265;
$shortcode_to_margin_per_contract_{"LFR_0"} = $GBP_TO_DOL * 2270;
$shortcode_to_margin_per_contract_{"LFZ_0"} = $GBP_TO_DOL * 3000;
$shortcode_to_margin_per_contract_{"LFI_0"} = $EUR_TO_DOL * 597;
$shortcode_to_margin_per_contract_{"LFI_1"} = $EUR_TO_DOL * 597;
$shortcode_to_margin_per_contract_{"LFI_2"} = $EUR_TO_DOL * 597;
$shortcode_to_margin_per_contract_{"LFI_3"} = $EUR_TO_DOL * 597;
$shortcode_to_margin_per_contract_{"LFI_4"} = $EUR_TO_DOL * 597;
$shortcode_to_margin_per_contract_{"LFI_5"} = $EUR_TO_DOL * 597;
$shortcode_to_margin_per_contract_{"LFI_6"} = $EUR_TO_DOL * 597;
$shortcode_to_margin_per_contract_{"LFL_0"} = $GBP_TO_DOL * 436;
$shortcode_to_margin_per_contract_{"LFL_1"} = $GBP_TO_DOL * 436;
$shortcode_to_margin_per_contract_{"LFL_2"} = $GBP_TO_DOL * 436;
$shortcode_to_margin_per_contract_{"LFL_3"} = $GBP_TO_DOL * 436;
$shortcode_to_margin_per_contract_{"LFL_4"} = $GBP_TO_DOL * 436;
$shortcode_to_margin_per_contract_{"LFL_5"} = $GBP_TO_DOL * 436;
$shortcode_to_margin_per_contract_{"LFL_6"} = $GBP_TO_DOL * 436;

$shortcode_to_margin_per_contract_{"CGB_0"} = 1850;
$shortcode_to_margin_per_contract_{"SXF_0"} = 5300;

my %symbol_to_shortcode_ = ();
my %shortcode_to_margin_ = ();
my %shortcode_to_pos_ = ();

my $max_margin_ = 0;
my $max_margin_date_ = $end_date_;
my %max_margin_pos_ = ();
my %max_margin_margin_ = ();

foreach my $t_shortcode_ ( keys %shortcode_to_margin_per_contract_ )
{
    $shortcode_to_margin_{ $t_shortcode_ } = 0;
    $shortcode_to_pos_{ $t_shortcode_ } = 0;
}

my $temp_all_trades_file_base_ = "$HOME/margin_calc_all_trades";

my $t_date_ = $end_date_;
for ( my $iter_ = 0; $iter_ < $num_of_days_; $iter_++ )
{
    while ( (!ValidDate($t_date_)) || SkipWeirdDate($t_date_) || IsDateHoliday($t_date_) )
    {
        $t_date_ = CalcPrevWorkingDateMult( $t_date_, 1 );
    }
    my $temp_all_trades_file_ = $temp_all_trades_file_base_."_$t_date_";
    if ( ExistsWithSize ( $temp_all_trades_file_ ) )
    {
	print "removing : $temp_all_trades_file_\n";
	`rm $temp_all_trades_file_`;
    }
    push ( @temporary_files_, $temp_all_trades_file_ );

    foreach my $t_shortcode_ ( keys %shortcode_to_margin_per_contract_ )
    {
	$shortcode_to_margin_{ $t_shortcode_ } = 0;
	$shortcode_to_pos_{ $t_shortcode_ } = 0;
	my $t_symbol_ = `~/infracore_install/bin/get_exchange_symbol $t_shortcode_ $t_date_ | tr ' ' '~'`; chomp( $t_symbol_ );
	$symbol_to_shortcode_{ $t_symbol_ } = $t_shortcode_;
	if ( substr($t_shortcode_, 0, 3) eq "LFI" || substr($t_shortcode_, 0, 3) eq "LFR" || substr($t_shortcode_, 0, 3) eq "LFZ" || substr($t_shortcode_, 0, 3) eq "LFL" )
	{
	    `~/infracore_install/bin/ors_binary_reader $t_shortcode_ $t_date_ \| awk '{print \$3 \"~~\" \$4, \$13, \$28}' >> $temp_all_trades_file_`;
	}
	else
	{
	    `~/infracore_install/bin/ors_binary_reader $t_shortcode_ $t_date_ \| awk '{print \$3, \$12, \$27}' >> $temp_all_trades_file_`;
	}
    }
    my $temp_sorted_file_ = $temp_all_trades_file_."_sorted";
    `cat $temp_all_trades_file_ | sort -nk2 > $temp_sorted_file_`;
    push ( @temporary_files_, $temp_sorted_file_ );

    open ( TRADE_FILE, "<", $temp_sorted_file_ ) or PrintStacktraceAndDie ( "Could Not Open $temp_sorted_file_\n" );
    my @all_trade_lines_ = <TRADE_FILE>;
    close( TRADE_FILE );
    chomp( @all_trade_lines_ );
    foreach my $trade_line_ ( @all_trade_lines_ )
    {
	my @trade_line_words_ = split(' ', $trade_line_ );
	my $t_symbol_ = $trade_line_words_[ 0 ];
	my $t_global_pos_ = $trade_line_words_[ 2 ];
	my $t_shortcode_ = $symbol_to_shortcode_{ $t_symbol_ };
	if( $t_shortcode_ eq "" )
	{
	    print "for $t_symbol_ shortcode not found : $t_shortcode_, Date : $t_date_ \n" ;
	    exit( 0 );
	}
	if( abs( $t_global_pos_ ) > abs( $shortcode_to_pos_{ $t_shortcode_ } ) )
	{
	    $shortcode_to_pos_{ $t_shortcode_ } = $t_global_pos_;
	    $shortcode_to_margin_{ $t_shortcode_ } = abs( $t_global_pos_ ) * $shortcode_to_margin_per_contract_{ $t_shortcode_ };
	    CheckMaxTotalMargin();
	}
	else
	{
	    $shortcode_to_pos_{ $t_shortcode_ } = $t_global_pos_;
	    $shortcode_to_margin_{ $t_shortcode_ } = abs( $t_global_pos_ ) * $shortcode_to_margin_per_contract_{ $t_shortcode_ };
	}
    }
    `rm -f $temp_all_trades_file_ $temp_sorted_file_`;

    $t_date_ = CalcPrevWorkingDateMult ( $t_date_, 1 );
}

print "\n\nMAX MARGIN(USD) = $max_margin_ reached on date $max_margin_date_\n";
foreach my $t_shortcode_ ( keys %max_margin_pos_ )
{
    print "shortcode : $t_shortcode_,\tmax_margin position: $max_margin_pos_{ $t_shortcode_ },  \tmargin(USD): $max_margin_margin_{ $t_shortcode_ }\n";
}

ClearTemporaryFiles ( @temporary_files_ );

exit (0);

sub CheckMaxTotalMargin
{
    my $curr_margin_ = 0;
    foreach my $t_shortcode_ ( keys %shortcode_to_margin_per_contract_ )
    {
	$curr_margin_ += $shortcode_to_margin_{ $t_shortcode_ };
    }
    if( $curr_margin_ > $max_margin_ )
    {
	$max_margin_ = $curr_margin_;
	$max_margin_date_ = $t_date_;
	foreach my $t_shortcode_ ( keys %shortcode_to_margin_per_contract_ )
	{
	    $max_margin_pos_{ $t_shortcode_ } = $shortcode_to_pos_{ $t_shortcode_ };
	    $max_margin_margin_{ $t_shortcode_ } = $shortcode_to_margin_{ $t_shortcode_ };
	}
    }
}
