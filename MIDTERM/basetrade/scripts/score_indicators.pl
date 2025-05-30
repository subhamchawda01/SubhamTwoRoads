#!/usr/bin/perl

# \file scripts/score_indicators.pl
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
use FileHandle;
use List::Util qw/max min/; # for max

my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'}; 

my $REPO = "basetrade";

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $MINE_SCRIPT = $SCRIPTS_DIR."/mine_strategy_logs.pl";

require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

use File::Basename;
use Term::ANSIColor; 

my $COLOR = 0;
my $DEBUG = 0;

sub CreateIndicatorMapping;
sub CreateIndicatorTimeSeries;

sub InterpolateIndicatorValuesBeforeTime;

sub ScoreExecutions;
sub ScoreCancellations;

sub NextPriceMoveAfter;

my $USAGE="$0 LOG_FILE [DC]";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $log_file_ = $ARGV [ 0 ];

if ( index ( $log_file_ , "gz" ) >= 0 )
{
    my $t_indicator_dir_ = "$HOME_DIR/scoreindtemp";
    `mkdir -p $t_indicator_dir_`;

    `cp $log_file_ $t_indicator_dir_`;

    $log_file_ = $t_indicator_dir_."/".basename ( $log_file_ );

    `gunzip -f $log_file_`;

    $log_file_ = substr ( $log_file_ , 0 , -3 );
}

if ( $#ARGV > 0 )
{
    if ( index ( $ARGV [ 1 ] , "D" ) != -1 ) { $DEBUG = 1; }
    if ( index ( $ARGV [ 1 ] , "C" ) != -1 ) { $COLOR = 1; }
}

my @indicator_list_ = `$MINE_SCRIPT $log_file_ I`;
my @indicator_name_list_ = `grep \"INDICATOR \" $log_file_`;

my %column_to_indicator_name_ = ( );
my %indicator_to_weight_ = ( );
my %indicator_to_score_ = ( );

CreateIndicatorMapping ( ); # Create column. no. to ind. name mapping.

my %time_to_indicator_to_value_ = ( );
my @indicator_value_times_ = ( );

CreateIndicatorTimeSeries ( ); # Generate a timed series of indicator values.

my %time_to_mkt_price_ = ( );
my @mkt_price_times_ = ( );

my @price_list_ = `grep \"  mkt\" $log_file_`;
CreatePriceTimeSeries ( ); # Generate a timed series of mkt prices.

my $exec_weight_ = 1.0;
my @exec_list_ = `$MINE_SCRIPT $log_file_ E`;
ScoreExecutions ( $exec_weight_ );

my $cancel_weight_ = 0.4;
my @cxl_list_ = `$MINE_SCRIPT $log_file_ C`;
ScoreCancellations ( $cancel_weight_ );

`rm -f $log_file_`;

exit ( 0 );

## Indicator subs ##
sub CreateIndicatorMapping
{
    if ( $DEBUG ) 
    { 
	print "CreateIndicatorMapping\n"; 
	print "Col IndicatorName IndicatorWeight IndicatorScore\n";
    }

    my $t_indicator_column_ = 0;
    my %is_processed_indicator_line_ = ( );

    for ( my $i = 0 ; $i <= $#indicator_name_list_ ; $i ++ )
    {
	my $t_indicator_line_ = $indicator_name_list_ [ $i ]; chomp ( $t_indicator_line_ );

	# INDICATOR 0.00871172 ScaledTrendMktEventsCombo UBFUT 3 MktSizeWPrice # 0.31
	my @indicator_words_ = split ( ' ' , $indicator_name_list_ [ $i ] );

	if ( $#indicator_words_ < 3 ) 
	{ PrintStacktraceAndDie ( "Invalid indicator line ' $indicator_name_list_[$i] '\n" ); }

	my $t_indicator_name_ = $indicator_words_ [ 2 ];
	my $t_indicator_weight_ = $indicator_words_ [ 1 ];

	if ( exists ( $is_processed_indicator_line_ { $t_indicator_line_ } ) )
	{ next; }
	$is_processed_indicator_line_ { $t_indicator_line_ } = 1;

	$t_indicator_column_ ++;
	$column_to_indicator_name_ { $t_indicator_column_ } = $t_indicator_name_;
	$indicator_to_weight_ { $t_indicator_name_ } = $t_indicator_weight_ + 0.0; # Check numeric
	$indicator_to_score_ { $t_indicator_name_ } = 0.0;

	if ( $DEBUG )
	{
	    print $t_indicator_column_." ".$column_to_indicator_name_ { $t_indicator_column_ }." ".$indicator_to_weight_ { $t_indicator_name_ }." ".$indicator_to_score_ { $t_indicator_name_ }."\n";
	}
    }

    return;
}

sub CreateIndicatorTimeSeries
{
    if ( $DEBUG ) { print "CreateIndicatorTimeSeries\n"; }

    for ( my $i = 0 ; $i <= $#indicator_list_ ; $i ++ )
    {
	my @indicator_words_ = split ( ' ' , $indicator_list_ [ $i ] );

	if ( $#indicator_words_ < keys ( %column_to_indicator_name_ ) ) 
	{ next; }

	my $t_time_ = $indicator_words_ [ 0 ];

	for ( my $j = 1 ; $j <= $#indicator_words_ ; $j ++ )
	{
	    $time_to_indicator_to_value_ { $t_time_ } { $column_to_indicator_name_ { $j } } = $indicator_words_ [ $j ];
	}

	if ( FindItemFromVec ( $t_time_ , @indicator_value_times_ ) ne $t_time_ )
	{ push ( @indicator_value_times_ , $t_time_ ); }
    }

    @indicator_value_times_ = sort ( @indicator_value_times_ );

    if ( $DEBUG )
    {
	for ( my $i = 0 ; $i <= $#indicator_value_times_ ; $i ++ )
	{
	    print "\n\t".$indicator_value_times_ [ $i ]."\n";

	    foreach my $column_ ( sort { $a <=> $b }
				  keys %column_to_indicator_name_ )
	    {
		print $column_to_indicator_name_ { $column_ }." ".$time_to_indicator_to_value_ { $indicator_value_times_ [ $i ] } { $column_to_indicator_name_ { $column_ } }."\n";
	    }
	}
    }
}

sub InterpolateIndicatorValuesBeforeTime
{
    my ( $target_time_ ) = @_;

    if ( $DEBUG ) { print "InterpolateIndicatorValuesBeforeTime $target_time_\n"; }

    my $time_before_target_time_ = 0;

    for ( my $i = 0 ; $i <= $#indicator_value_times_ && $indicator_value_times_ [ $i ] <= $target_time_ ; $i ++ )
    { $time_before_target_time_ = $indicator_value_times_ [ $i ]; }

    my @t_indicator_values_ = ( );

    foreach my $column_ ( sort { $a <=> $b }
			  keys %column_to_indicator_name_ )
    {
	my $t_indicator_name_ = $column_to_indicator_name_ { $column_ };
	push ( @t_indicator_values_ , $time_to_indicator_to_value_ { $time_before_target_time_ } { $t_indicator_name_ } );
    }

    if ( $DEBUG )
    {
	print "\n\t".$time_before_target_time_."\n";

	foreach my $column_ ( sort { $a <=> $b }
			      keys %column_to_indicator_name_ )
	{
	    print $column_to_indicator_name_ { $column_ }." ".$t_indicator_values_ [ $column_ - 1 ]."\n";
	}
    }

    return @t_indicator_values_;
}

## Score subs ##
sub ScoreExecutions
{
    my ( $weight_ ) = @_;

    if ( $DEBUG ) { print "ScoreExecutions\n"; }

    for ( my $i = 0 ; $i <= $#exec_list_ ; $i ++ )
    {
	if ( index ( $exec_list_ [ $i ] , "SAOS" ) != -1 ) { next; }

	# TODO Weigh both the conf & exec time indicators ?
	my @exec_words_ = split ( ' ' , $exec_list_ [ $i ] );

	my $t_exec_time_ = $exec_words_ [ 0 ];
	my $t_conf_time_ = $exec_words_ [ 4 ];
	my $t_buysell_ = $exec_words_ [ 3 ];

	my $next_price_move_ = NextPriceMoveAfter ( $t_exec_time_ );
	my $t_exec_score_ = 0;

	if ( ( $t_buysell_ eq "B" && $next_price_move_ > 0 ) || 
	     ( $t_buysell_ eq "S" && $next_price_move_ < 0 ) )
	{   # Bought and next price move was an uptick or
	    # Sold and next price move was a downtick
	    $t_exec_score_ = 1; # TODO Maybe a function of the unreal profit ?
	}
	elsif ( ( $t_buysell_ eq "S" && $next_price_move_ > 0 ) || 
		( $t_buysell_ eq "B" && $next_price_move_ < 0 ) )
	{   # Bought and next price move was a downtick or
	    # Sold and next price move was an uptick
	    $t_exec_score_ = -1; # TODO Maybe a function of the unreal loss ?
	}

	# Find indicator values responsible for keeping this order on.
	my @t_indicator_values_ = InterpolateIndicatorValuesBeforeTime ( $t_exec_time_ );
    }

    return;
}

sub ScoreCancellations
{
    my ( $weight_ ) = @_;

    if ( $DEBUG ) { print "ScoreCancellations\n"; }

    return;
}

## Price subs ##
sub CreatePriceTimeSeries
{
    if ( $DEBUG ) { print "CreatePriceTimeSeries\n"; }

    for ( my $i = 0 ; $i <= $#price_list_ ; $i ++ )
    {
	my @price_words_ = split ( ' ' , $price_list_ [ $i ] );

	my $t_time_ = $price_words_ [ 0 ];
	my $t_price_ = ( $price_words_ [ 4 ] + $price_words_ [ 6 ] ) / 2.0; # Mid-price.

	if ( FindItemFromVec ( $t_time_ , @mkt_price_times_ ) ne $t_time_ )
	{ push ( @mkt_price_times_ , $t_time_ ); }

	$time_to_mkt_price_ { $t_time_ } = $t_price_;
    }

    @mkt_price_times_ = sort ( @mkt_price_times_ );

    if ( $DEBUG )
    {
	for ( my $i = 0 ; $i <= $#mkt_price_times_ ; $i ++ )
	{
#	    print $mkt_price_times_ [ $i ]." ".$time_to_mkt_price_ { $mkt_price_times_ [ $i ] }."\n";
	}
    }

    return;
}

sub NextPriceMoveAfter
{
    my ( $time_ ) = @_;

    if ( $DEBUG ) { print "NextPriceMoveAfter $time_\n"; }

    my $time_before_target_ = 0;
    my $time_after_target_ = 0;

    for ( my $i = 0 ; $i <= $#mkt_price_times_ ; $i ++ )
    {
	if ( $mkt_price_times_ [ $i ] < $time_ )
	{ $time_before_target_ = $mkt_price_times_ [ $i ]; }
	elsif ( $time_to_mkt_price_ { $time_before_target_ } != $time_to_mkt_price_ { $mkt_price_times_ [ $i ] } # We found a price change.
		|| ( $mkt_price_times_ [ $i ] - $time_before_target_ ) > 10 ) # Or 10 secs past target time.
	{
	    $time_after_target_ = $mkt_price_times_ [ $i ];
	    last;
	}
    }

    my $price_before_target_ = $time_to_mkt_price_ { $time_before_target_ };
    my $price_after_target_ = $time_to_mkt_price_ { $time_after_target_ };

    if ( $DEBUG ) 
    { print "[ ( ".$time_before_target_." , ".$price_before_target_." ) - ( ".$time_after_target_." , ".$price_after_target_." ) ]\n"; }

    my $next_price_move_ = 0;

    if ( $price_after_target_ > $price_before_target_ ) { $next_price_move_ = 1; } # Price went up
    elsif ( $price_after_target_ < $price_before_target_ ) { $next_price_move_ = -1; } # Price went down

    return $next_price_move_;
}
