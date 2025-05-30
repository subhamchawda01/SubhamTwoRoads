# \file GenPerlLib/get_trade_fields_from_trade_line.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
use strict;
use warnings;
use feature "switch";

sub GetTradeTimeFromTradeLine
{
    my $t_trade_line_ = shift; chomp ( $t_trade_line_ );

    # 1330709281.415405 FLAT ZFM2.12019 B    2 123.4296875    0        0       75 [  1637 123.421875 X 123.429688    76 ]

    my @trade_words_ = split ( ' ' , $t_trade_line_ );

    my $ret_field_ = $trade_words_ [ 0 ]; chomp ( $ret_field_ );

    $ret_field_;
}

sub GetTradeTypeFromTradeLine
{
    my $t_trade_line_ = shift; chomp ( $t_trade_line_ );

    # 1330709281.415405 FLAT ZFM2.12019 B    2 123.4296875    0        0       75 [  1637 123.421875 X 123.429688    76 ]

    my @trade_words_ = split ( ' ' , $t_trade_line_ );

    my $ret_field_ = $trade_words_ [ 3 ]; chomp ( $ret_field_ );

    $ret_field_;
}

sub GetTradeSizeFromTradeLine
{
    my $t_trade_line_ = shift; chomp ( $t_trade_line_ );

    # 1330709281.415405 FLAT ZFM2.12019 B    2 123.4296875    0        0       75 [  1637 123.421875 X 123.429688    76 ]

    my @trade_words_ = split ( ' ' , $t_trade_line_ );

    my $ret_field_ = $trade_words_ [ 4 ]; chomp ( $ret_field_ );

    $ret_field_;
}

sub GetTradePriceFromTradeLine
{
    my $t_trade_line_ = shift; chomp ( $t_trade_line_ );

    # 1330709281.415405 FLAT ZFM2.12019 B    2 123.4296875    0        0       75 [  1637 123.421875 X 123.429688    76 ]

    my @trade_words_ = split ( ' ' , $t_trade_line_ );

    my $ret_field_ = $trade_words_ [ 5 ]; chomp ( $ret_field_ );

    $ret_field_;
}

sub GetMarketBidPriceFromTradeLine
{
    my $t_trade_line_ = shift; chomp ( $t_trade_line_ );

    # 1330709281.415405 FLAT ZFM2.12019 B    2 123.4296875    0        0       75 [  1637 123.421875 X 123.429688    76 ]

    my @trade_words_ = split ( ' ' , $t_trade_line_ );

    my $ret_field_ = $trade_words_ [ 11 ]; chomp ( $ret_field_ );

    $ret_field_;
}

sub GetMarketAskPriceFromTradeLine
{
    my $t_trade_line_ = shift; chomp ( $t_trade_line_ );

    # 1330709281.415405 FLAT ZFM2.12019 B    2 123.4296875    0        0       75 [  1637 123.421875 X 123.429688    76 ]

    my @trade_words_ = split ( ' ' , $t_trade_line_ );

    my $ret_field_ = $trade_words_ [ 13 ]; chomp ( $ret_field_ );

    $ret_field_;
}

1;
