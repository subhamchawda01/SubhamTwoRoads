use strict;
use warnings;
use FileHandle;

my $USAGE="$0 ne_trades_filename_ ";
if ( $#ARGV < 0 ) { print "$USAGE\n"; exit ( 0 ); }
my $ne_trades_filename_ = $ARGV[0];
my $today_date_ = `date +"%Y%m%d"` ; chomp ( $today_date_ ) ;

my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore_install";
my $INSTALL_BIN="$HOME_DIR/$REPO/bin";
my %ne_exch_code_to_shortcode_ = ();
my %shortcode_to_exch_symbol_ = ();

$ne_exch_code_to_shortcode_{"NK225F"} = "NK_0";
$ne_exch_code_to_shortcode_{"NK225MF"} = "NKMF_0";
$ne_exch_code_to_shortcode_{"NK225"} = "NK_1";
$ne_exch_code_to_shortcode_{"NK225M"} = "NKM_0";
$ne_exch_code_to_shortcode_{"JGB"} = "JGBL_0";
$ne_exch_code_to_shortcode_{"JTPX"} = "TOPIX_0";
$ne_exch_code_to_shortcode_{"JN400F"} = "JP400_0";
$ne_exch_code_to_shortcode_{"HSI"} = "HSI_0";
$ne_exch_code_to_shortcode_{"HHI"} = "HHI_0";

# PRECID # Needed to figure out which records to use towards pnl computation.
my $buysell_offset_ = 12; # PBS # 1 = buy ; 2 = sell
my $quantity_offset_ = 13; # PQTY
my $trade_price_offset_ = 14; # PTPRIC # Trade price may be different than price of order
my $exch_code_offset_ = 6;
my $time_offset = 5;


open NE_TRADES_FILE_HANDLE, "< $ne_trades_filename_" or die "could not open ne_trades_filename_ $ne_trades_filename_\n";

my @ne_trades_file_lines_ = <NE_TRADES_FILE_HANDLE>;

close NE_TRADES_FILE_HANDLE;

for (my $i = 1; $i <= $#ne_trades_file_lines_; $i++) {
  my @fields_ = split (',', $ne_trades_file_lines_[$i]);
  
  $fields_[$buysell_offset_]=~s/[^a-zA-Z0-9. _()-]//g;
  $fields_[$quantity_offset_]=~s/[^a-zA-Z0-9. _()-]//g;
  $fields_[$trade_price_offset_]=~s/[^a-zA-Z0-9. _()-]//g;
  $fields_[$exch_code_offset_]=~s/[^a-zA-Z0-9. _()-]//g;
  $fields_[$time_offset]=~s/[^a-zA-Z0-9. _()-]//g;
  
  my $buysell_ = $fields_[$buysell_offset_]-1; #in ors trade files.. 0 is buy and 1 is sell
  my $quantity_ = $fields_[$quantity_offset_];
  my $trade_price_ = $fields_[$trade_price_offset_];
  my $exch_code_ = $fields_[$exch_code_offset_];
  my $short_code_=$ne_exch_code_to_shortcode_{$exch_code_};
  my $time_=$fields_[$time_offset];
  
  if( ! (($exch_code_ eq "HHI") or ($exch_code_ eq "JN400F"))){
  	$trade_price_ = $trade_price_ * 100;
  }
  
  if ( ! ( exists $shortcode_to_exch_symbol_{$short_code_} ) )
  {
  	my $exchange_symbol_ = `$INSTALL_BIN/get_exchange_symbol "$short_code_" $today_date_ ` ; 
  	chomp ( $exchange_symbol_ ) ;
  	$shortcode_to_exch_symbol_{$short_code_}=$exchange_symbol_;
  }
  
  if( $exch_code_ eq "HHI" || $exch_code_ eq "HSI"){
  	if($time_ > 90000 && $time_ < 170000){
  		print "$shortcode_to_exch_symbol_{$short_code_}$buysell_$quantity_$trade_price_0\n";
  	}
  }
  else{
  	if($time_ > 900 && $time_ < 1500){
  		print "$shortcode_to_exch_symbol_{$short_code_}$buysell_$quantity_$trade_price_0\n";
  	}
  }
}