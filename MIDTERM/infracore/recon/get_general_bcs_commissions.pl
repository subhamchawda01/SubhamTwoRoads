#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use List::Util qw/max min/; # for max

my $USAGE="$0 YYYYMMDD";
if ( @ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }
my $expected_date_ = $ARGV[0];
my $bcs_trades_filename_ ="/apps/data/BCSTrades/BCSFiles/".$expected_date_."_trades"; 

my $REPO="infracore_install";
my $GENPERLLIB_DIR=$ENV{"HOME"}."/".$REPO."/GenPerlLib";
require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; # 

# cat  8279396_trades_20131009_111533.csv | head -n1 | awk -F';' '{ for ( i = 1 ; i <= NF ; i ++ ) print i" "$i }' 
#1 TradeNum
#2 TradeDate              # 1
#3 TradeTime
#4 TSSection_Name
#5 BuySell                # 1
#6 FunctionType
#7 AgreeNum
#8 Subacc_SubaccCode      # 1
#9 PutAccount_AccountCode
#10 PayAccount_AccountCode
#11 Security_SecCode      # 1
#12 Asset_ShortName       # 1
#13 CurrPayAsset_ShortName# 1
#14 CPFirm_FirmShortName
#15 Price                 # 1
#16 Qty                   # 1
#17 IsAccrued
#18 AccruedInt
#19 Volume1               # 1
#20 ClearingComission     # 1
#21 ExchangeComission     # 1
#22 TechCenterComission   # 1
#23 PayPlannedDate
#24 PutPlannedDate
#25 IsRepo2
#26 RepoRate
#27 BackDate
#28 RepoDate#2
#29 BackPrice
#30 Volume2
#31 Accruedint2
#32 VarMargin             # 1
#33 Comment               

my $date_offset_ = 0; # TradeDate MM.DD.YYYY
my $buysell_offset_= 1 ;
my $account_code_offset_ = 2; # Subacc_SubaccCode 8279396 10396
my $curr_offset_ = 5; # Subacc_SubaccCode 8279396 10396
my $bcs_future_code_offset_ = 3; # Security_SecCode
my $trade_price_offset_ = 6; # Price
my $quantity_offset_ = 7; # Qty

my %symbol_to_commissions = ();
my $clearing_comm_offset = 9;
my $exchange_comm_offset = 10;
my $tech_centre_comm_offset =11;
my $broker_comm_offset = 12;
# RUBUSD
my $RUB_TO_USD_ = 0.0309056 ;

#================================= fetching conversion rates from currency info file if available ====================#

my $last_seen_todtom_price = 0; #this to to fix todtom issue. they give seperate trades for tod and tom with todtom name

my $curr_date_ = $expected_date_ ;
my $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$curr_date_.'.txt' ;

my $counter_ = 1 ;
while ( ! -e $curr_filename_ && $counter_ < 5 )
{
    $curr_date_ = CalcPrevBusinessDay ( $curr_date_ ) ;
    $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$curr_date_.'.txt' ;
    $counter_ += 1 ;
}

my @curr_file_lines_ ;
if ( -e $curr_filename_ )
{
    open CURR_FILE_HANDLE, "< $curr_filename_" ;
    my @curr_file_lines_ = <CURR_FILE_HANDLE>;
    close CURR_FILE_HANDLE;
}

for ( my $itr = 0 ; $itr <= $#curr_file_lines_; $itr ++ )
{
    my @cwords_ = split ( ' ', $curr_file_lines_[$itr] );
    if ( $#cwords_ >= 1 )
    {
    if( index ( $cwords_[0], "USDRUB" ) == 0 )
    {
        $RUB_TO_USD_ = sprintf( "%.4f", 1 / $cwords_[1] );
    }
    }
}
#======================================================================================================================#

my $mail_body_="";
my @bcs_trades_file_lines_ = `awk -F';' '{ print \$2" "\$5" "\$8" "\$11" "\$12" "\$13" "\$15" "\$16" "\$19" "\$20" "\$21" "\$22" "\$34" "\$32 }' $bcs_trades_filename_` ;


for ( my $i = 1; $i <= $#bcs_trades_file_lines_; $i++ ) 
{
    my @fields_ = split (' ', $bcs_trades_file_lines_[$i] ); 
    
    
    my $date_ = $fields_[$date_offset_];
    my @mmddyyyy_ = split ( '\.' , $date_ ) ;
    if ( scalar ( @mmddyyyy_ == 3 ) )
    {
      $date_ = $mmddyyyy_[2].$mmddyyyy_[1].$mmddyyyy_[0];
    }
    
    my $exchange_symbol_ = $fields_[$bcs_future_code_offset_] ; $exchange_symbol_ =~ s/^\s+|\s+$//g;
    my $quantity_ = $fields_[$quantity_offset_]; $quantity_ =~ s/^\s+|\s+$//g;

    if (index ($exchange_symbol_, "Security") != -1){
    	next;
    }
     
    my $clearing_comm =$fields_[$clearing_comm_offset]; $clearing_comm =~ s/^\s+|\s+$//g;
    my $exchange_comm = $fields_[$exchange_comm_offset]; $exchange_comm =~ s/^\s+|\s+$//g;
    my $tech_centre_comm =$fields_[$tech_centre_comm_offset]; $tech_centre_comm =~ s/^\s+|\s+$//g;
    my $broker_comm = $fields_[$broker_comm_offset]; $broker_comm=~ s/^\s+|\s+$//g;
    my $total_comm_ = ($clearing_comm + $exchange_comm + $tech_centre_comm + $broker_comm);

     if(! exists $symbol_to_commissions{$exchange_symbol_}){
    $symbol_to_commissions{$exchange_symbol_} = 0;
  }
    $symbol_to_commissions{$exchange_symbol_} += $total_comm_;

}

foreach my $key (keys %symbol_to_commissions) {
    print "$key|$symbol_to_commissions{$key}\n";
}

