#!/usr/bin/perl

# \file scripts/generate_trades_file_diffs.pl
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
use List::Util qw/max min/; # max , min

my $USER = $ENV { 'USER' } ;
my $HOME_DIR = $ENV { 'HOME' } ;
my $REPO = "basetrade";

my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";

my $USAGE = "$0 trades_file1 trades_file2 output_file_name";

if ( $#ARGV < 2 ) 
{ 
    printf "$USAGE\n" ;
    exit ( 0 ) ; 
}

my $trades_file_1_ = $ARGV [ 0 ] ; chomp ( $trades_file_1_ );
my $trades_file_2_ = $ARGV [ 1 ] ; chomp ( $trades_file_2_ );
my $output_file_name_ = $ARGV [ 2 ]; chomp ( $output_file_name_ );

my %time_to_pnl_1_ = ( ) ;
my %time_to_pnl_2_ = ( ) ;
my %time_to_pos_1_ = ( ) ;
my %time_to_pos_2_ = ( ) ;
my %time_to_vol_1_ = ( ) ;
my %time_to_vol_2_ = ( ) ;

my @all_times_1_ = ( );
my @all_times_2_ = ( );
my @all_times_ = ( ) ;

# Matching should start after max ( $start_time_1_ , $start_time_2_ )
my $start_time_1_ = 0;
my $start_time_2_ = 0;

# Matching should stop after min ( $last_time_1_ , $last_time_2_ )
my $last_time_1_ = 0;
my $last_time_2_ = 0;

{
    open TRADES_FILE , "< $trades_file_1_" or PrintStacktraceAndDie ( "Could not open $trades_file_1_\n" );
    my @trades_1_lines_ = <TRADES_FILE> ;
    close ( TRADES_FILE ) ;

    my $t_vol_ = 0;

    for ( my $i = 0 ; $i <= $#trades_1_lines_ ; $i ++ )
    {
	my @trade_words_ = split ( ' ', $trades_1_lines_ [ $i ] );
        if ( index ( $trades_1_lines_[$i] , "PNLSAMPLES" ) >= 0 )  { next ; } 
	if ( $#trade_words_ >= 8 )
	{
	my $t_time_ = $trade_words_ [ 0 ];
	my $t_pos_ = $trade_words_ [ 6 ] ;
	my $t_pnl_ = $trade_words_ [ 8 ] ;

	my $t_size_ = $trade_words_ [ 4 ] ;
	$t_vol_ += $t_size_ ;

	$time_to_pnl_1_ { $t_time_ } = $t_pnl_ ;
	$time_to_pos_1_ { $t_time_ } = $t_pos_ ;
	$time_to_vol_1_ { $t_time_ } = $t_vol_ ;

	push ( @all_times_ , $t_time_ ) ;

	if ( $i == 0 ) { $start_time_1_ = $t_time_; }
	$last_time_1_ = $t_time_; 
	}
    }
}

{
    open TRADES_FILE , "< $trades_file_2_" or PrintStacktraceAndDie ( "Could not open $trades_file_2_\n" );
    my @trades_2_lines_ = <TRADES_FILE> ;
    close ( TRADES_FILE ) ;

    my $t_vol_ = 0;

    for ( my $i = 0 ; $i <= $#trades_2_lines_ ; $i ++ )
    {
	my @trade_words_ = split ( ' ', $trades_2_lines_ [ $i ] );
        if ( index ( $trades_2_lines_[$i] , "PNLSAMPLES" ) >= 0 )  { next ; } 
	if ( $#trade_words_ >= 8 )
	{
	my $t_time_ = $trade_words_ [ 0 ];
	my $t_pos_ = $trade_words_ [ 6 ] ;
	my $t_pnl_ = $trade_words_ [ 8 ];

	my $t_size_ = $trade_words_ [ 4 ] ;
	$t_vol_ += $t_size_ ;

	$time_to_pnl_2_ { $t_time_ } = $t_pnl_ ;
	$time_to_pos_2_ { $t_time_ } = $t_pos_ ;
	$time_to_vol_2_ { $t_time_ } = $t_vol_ ;

	push ( @all_times_ , $t_time_ ) ;


	if ( $i == 0 ) { $start_time_2_ = $t_time_; }
	 $last_time_2_ = $t_time_; 
	}
    }
}

open DIFFTRADEFILE, "> $output_file_name_" or PrintStacktraceAndDie ( "Could not open trade diff out file $output_file_name_ for writing \n" );

my $time_to_start_ = max ( $start_time_1_ , $start_time_2_ );
my $time_to_stop_ = min ( $last_time_1_ , $last_time_2_ );

@all_times_ = sort ( @all_times_ ) ;

my %time_to_abs_pnl_diff_ = ( ) ;
my %time_to_abs_pos_diff_ = ( ) ;
my %time_to_abs_vol_diff_ = ( ) ;

my $last_t1_pnl_ = 0 ;
my $last_t2_pnl_ = 0 ;
my $last_t1_pos_ = 0 ;
my $last_t2_pos_ = 0 ;
my $last_t1_vol_ = 0 ;
my $last_t2_vol_ = 0 ;


for ( my $i = 0 ; $i <= $#all_times_ ; $i ++ )
{
    my $time_ = $all_times_ [ $i ] ;

    if ( exists $time_to_pnl_1_ { $time_ } )
    {
	$last_t1_pnl_ = $time_to_pnl_1_ { $time_ } ;
	$last_t1_pos_ = $time_to_pos_1_ { $time_ } ;
	$last_t1_vol_ = $time_to_vol_1_ { $time_ } ;
    }

    if ( exists $time_to_pnl_2_ { $time_ } )
    {
	$last_t2_pnl_ = $time_to_pnl_2_ { $time_ } ;
	$last_t2_pos_ = $time_to_pos_2_ { $time_ } ;
	$last_t2_vol_ = $time_to_vol_2_ { $time_ } ;
    }

    my $abs_pnl_diff_ = ( $last_t1_pnl_ - $last_t2_pnl_ ) ;
    my $abs_pos_diff_ = abs ( $last_t1_pos_ - $last_t2_pos_ ) ;
    my $abs_vol_diff_ = ( $last_t1_vol_ - $last_t2_vol_ ) ;

#    print "$time_ $last_t1_pnl_ $last_t2_pnl_ \n";

    $time_to_abs_pnl_diff_ { $time_ } = $abs_pnl_diff_ ;
    $time_to_abs_pos_diff_ { $time_ } = $abs_pos_diff_ ;
    $time_to_abs_vol_diff_ { $time_ } = $abs_vol_diff_ ;
    if ($time_ >= $time_to_start_ and $time_ <= $time_to_stop_ )
    {
      print "$time_ $time_to_abs_pnl_diff_{$time_} $time_to_abs_pos_diff_{$time_} $time_to_abs_vol_diff_{$time_}\n";
      print DIFFTRADEFILE "$time_ $time_to_abs_pnl_diff_{$time_} $time_to_abs_pos_diff_{$time_} $time_to_abs_vol_diff_{$time_}\n";
    }
}

close DIFFTRADEFILE;
