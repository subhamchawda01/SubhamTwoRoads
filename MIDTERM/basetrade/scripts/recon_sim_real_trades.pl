#!/usr/bin/perl
use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $USAGE="$0 real_trades_file sim_trades_file";

require $GENPERLLIB_DIR."/get_trade_fields_from_trade_line.pl"; # GetTrade*FromTradeLine

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ($#ARGV < 1) { 
    printf "$USAGE\n"; 
    exit (0); 
}

my $real_trades_file_ = $ARGV [ 0 ];
my $sim_trades_file_ = $ARGV [ 1 ];

open REAL_TRADES_FILE , "< $real_trades_file_" or PrintStacktraceAndDie ( "Could not open $real_trades_file_\n" );
my @real_trades_ = <REAL_TRADES_FILE>;
close REAL_TRADES_FILE;

open SIM_TRADES_FILE , "< $sim_trades_file_" or PrintStacktraceAndDie ( "Could not open $sim_trades_file_\n" );
my @sim_trades_ = <SIM_TRADES_FILE>;
close SIM_TRADES_FILE;

# Matching trades from real are placed into this list.
my %matched_sim_lines_ = ( );

my %unmatched_real_lines_ = ( );
my %unmatched_sim_lines_ = ( );

for ( my $real_line_ = 0 ; $real_line_ <= $#real_trades_ ; $real_line_ ++ )
{
    my $t_real_trade_time_ = GetTradeTimeFromTradeLine ( $real_trades_ [ $real_line_ ] );
    my $t_real_trade_type_ = GetTradeTypeFromTradeLine ( $real_trades_ [ $real_line_ ] );
    my $t_real_trade_size_ = GetTradeSizeFromTradeLine ( $real_trades_ [ $real_line_ ] );
    my $t_real_trade_price_ = GetTradePriceFromTradeLine ( $real_trades_ [ $real_line_ ] );

    my $is_matching_trade_ = MatchingTradeExists ( $t_real_trade_time_ , $t_real_trade_type_ , $t_real_trade_size_ , $t_real_trade_price_ );

    if ( $is_matching_trade_ == 0 )
    {

    # 	# Output info.
    # 	# (i) Was this trade an agg. trade ?
    # 	if ( ( $t_real_trade_type_ eq "S" && $t_real_trade_price_ < GetMarketAskPriceFromTradeLine ( $real_trades_ [ $real_line_ ] ) ) || ( $t_real_trade_type_ eq "B" && $t_real_trade_price_ > GetMarketBidPriceFromTradeLine ( $real_trades_ [ $real_line_ ] ) ) )
    # 	{
    # 	    print "< A $real_trades_[$real_line_]";
    # 	}
    # 	# (ii) Print sizes and order count on level when this happened.
    # 	else 
    # 	{
    # 	    print "< N $real_trades_[$real_line_]";
    # 	}

	$unmatched_real_lines_ { $real_trades_ [ $real_line_ ] } = 1;
    }
}

for ( my $sim_line_ = 0 ; $sim_line_ <= $#sim_trades_ ; $sim_line_ ++ )
{
    if ( exists $matched_sim_lines_ { $sim_trades_ [ $sim_line_ ] } )
    { next; }

    $unmatched_sim_lines_ { $sim_trades_ [ $sim_line_ ] } = 1;
}

# Print aggreggated trades.
my $sim_line_ = 0;
my $real_line_ = 0;

use Term::ANSIColor; 

while ( $sim_line_ <= $#sim_trades_ || $real_line_ <= $#real_trades_ )
{
    if ( 
	( $real_line_ <= $#real_trades_ && $sim_line_ <= $#sim_trades_ && GetTradeTimeFromTradeLine ( $sim_trades_ [ $sim_line_ ] ) <= GetTradeTimeFromTradeLine ( $real_trades_ [ $real_line_ ] ) ) ||
	( $sim_line_ <= $#sim_trades_ && $real_line_ > $#real_trades_ )
	)
    {
	if ( exists $unmatched_sim_lines_ { $sim_trades_ [ $sim_line_ ] } )
	{
	    print color ( "red" );
	    my $line_ = AdjustProductName ( $sim_trades_ [ $sim_line_ ] );
	    print $line_;
	    print color ( "reset" );
	}
	else
	{
	    my $line_ = AdjustProductName ( $sim_trades_ [ $sim_line_ ] );
	    print $line_;
	}

	$sim_line_ ++;
    }
    if ( 
	( $real_line_ <= $#real_trades_ && $sim_line_ <= $#sim_trades_ && GetTradeTimeFromTradeLine ( $real_trades_ [ $real_line_ ] ) <= GetTradeTimeFromTradeLine ( $sim_trades_ [ $sim_line_ ] ) ) ||
	( $sim_line_ > $#sim_trades_ && $real_line_ <= $#real_trades_ )
	)
    {
	if ( exists $unmatched_real_lines_ { $real_trades_ [ $real_line_ ] } )
	{
	    print color ( "blue" );
	    my $line_ = AdjustProductName ( $real_trades_ [ $real_line_ ] );
	    print $line_;
	    print color ( "reset" );
	}
	else
	{
	    my $line_ = AdjustProductName ( $real_trades_ [ $real_line_ ] );
	    print $line_;
	}

	$real_line_ ++;
    }
}

sub AdjustProductName
{
    my $line_ = shift;

    $line_;

    # my $adj_line_ = "";
    # my @line_words_ = split ( ' ' , $line_ );

    # for ( my $i = 0; $i < $#line_words_ ; $i ++ )
    # {
    # 	if ( $i != 2 )
    # 	{
    # 	    $adj_line_ = $adj_line_.$line_words_ [ $i ]." ";
    # 	}
    # 	else 
    # 	{
    # 	    my $adj_product_ = sprintf ( "%16s" , $line_words_ [ $i ] );
    # 	    $adj_line_ = $adj_line_.$adj_product_." ";
    # 	}
    # }

    # $adj_line_ = $adj_line_.$line_words_ [ $#line_words_ ]."\n";

    # $adj_line_;
}

sub MatchingTradeExists
{
    my $time_ = shift;
    my $trade_type_ = shift;
    my $size_ = shift;
    my $price_ = shift;

    my $is_matched_ = 0;

    for ( my $sim_line_ = 0 ; $sim_line_ <= $#sim_trades_ ; $sim_line_ ++ )
    {
	if ( exists $matched_sim_lines_ { $sim_trades_ [ $sim_line_ ] } )
	{ next; }

	my $t_sim_trade_time_ = GetTradeTimeFromTradeLine ( $sim_trades_ [ $sim_line_ ] );

	if ( $t_sim_trade_time_ > $time_ + 1.5 )
	{ last; }

	if ( $t_sim_trade_time_ < $time_ - 1.5 )
	{ next; }

	# This sim trade is within range.
	my $t_sim_trade_type_ = GetTradeTypeFromTradeLine ( $sim_trades_ [ $sim_line_ ] );
	my $t_sim_trade_size_ = GetTradeSizeFromTradeLine ( $sim_trades_ [ $sim_line_ ] );
	my $t_sim_trade_price_ = GetTradePriceFromTradeLine ( $sim_trades_ [ $sim_line_ ] );

	if ( $t_sim_trade_type_ eq $trade_type_ && $t_sim_trade_price_ == $price_ )
	{
	    $matched_sim_lines_ { $sim_trades_ [ $sim_line_ ] } = 1;
	    $is_matched_ = 1;
	    last;
	}
    }

    $is_matched_;
}
