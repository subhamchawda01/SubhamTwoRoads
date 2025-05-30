use strict;
use warnings;
use FileHandle;
use File::Basename;
use List::Util qw/max min/; # for max
use Data::Dumper;

my $USAGE="$0 trade_file_0(broker) trade_file_1(ors) (both in ors format) [dump_EOW_positions] [Load_overweekend_positions_file]";
if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $trade_file_0 = shift;
my $trade_file_1 = shift;
my $dump_eow_positions = shift || 0;
my $EOW_positions = shift || "";

my %symbol_to_position_map_0 = ();
my %symbol_to_position_map_1 = ();
my %symbol_to_volume_map_0 = ();
my %symbol_to_volume_map_1 = ();
my %spread_to_neg_symbol_ = ();
my %spread_to_pos_symbol_ = ();

my %symbol_to_expiry_year_ = ("0" => "2010", "1" => "2011", "2" => "2012", "3" => "2013", "4" => "2014", "5" => "2015", "6" => "2016", "7" => "2017", "8" => "2018", "9" => "2019");
my %symbol_to_expiry_month_ = ("F" => "01", "G" => "02", "H" => "03", "J" => "04", "K" => "05", "M" => "06", "N" => "07", "Q" => "08", "U" => "09", "V" => "10", "X" => "11", "Z" => "12");
my %expiry_month_to_symbol_ = ("01" => "F", "02" => "G", "03" => "H", "04" => "J", "05" => "K", "06" => "M", "07" => "N", "08" => "Q", "09" => "U", "10" => "V", "11" => "X", "12" => "Z");

if($EOW_positions ne ""){
	open EOW_TRADES_FILE_HANDLE, "< $EOW_positions";
	if(tell(EOW_TRADES_FILE_HANDLE) != -1){
	    my @ors_trades_file_lines_0 = <EOW_TRADES_FILE_HANDLE>;
	    close EOW_TRADES_FILE_HANDLE;
		for ( my $i = 0 ; $i <= $#ors_trades_file_lines_0; $i ++ )
		{
		  my @words_ = split ( '', $ors_trades_file_lines_0[$i] );
		  if ( $#words_ >= 2 )
		  {
		    my $symbol_ = $words_[0];
		    my $position_ = $words_[1];
		    my $buysell_ = 0;
		    my $tsize_ = abs($position_);
		    if($position_){
		    	$buysell_ = 1;
		    }
		    generatePositions($symbol_, $buysell_, $tsize_, \%symbol_to_position_map_0, \%symbol_to_volume_map_0);
		    generatePositions($symbol_, $buysell_, $tsize_, \%symbol_to_position_map_1, \%symbol_to_volume_map_1);
		  }
		}
	}
}


open BROKER_TRADES_FILE_HANDLE, "< $trade_file_0" or die "compare_positions.pl could not open broker file $trade_file_0 \n";
my @ors_trades_file_lines_0 = <BROKER_TRADES_FILE_HANDLE>;
close BROKER_TRADES_FILE_HANDLE;

for ( my $i = 0 ; $i <= $#ors_trades_file_lines_0; $i ++ )
{
  my @words_ = split ( '', $ors_trades_file_lines_0[$i] );
  if ( $#words_ >= 4 )
  {
    my $symbol_ = $words_[0];
    my $buysell_ = $words_[1];
    my $tsize_ = $words_[2];
    generatePositions($symbol_, $buysell_, $tsize_, \%symbol_to_position_map_0, \%symbol_to_volume_map_0);
  }
}

open ORS_TRADES_FILE_HANDLE, "< $trade_file_1" or die "compare_positions.pl could not open broker file $trade_file_1 \n";
my @ors_trades_file_lines_1 = <ORS_TRADES_FILE_HANDLE>;
close ORS_TRADES_FILE_HANDLE;

for ( my $i = 0 ; $i <= $#ors_trades_file_lines_1; $i ++ )
{
  my @words_ = split ( '', $ors_trades_file_lines_1[$i] );
  if ( $#words_ >= 4 )
  {
    my $symbol_ = $words_[0];
    my $buysell_ = $words_[1];
    my $tsize_ = $words_[2];
    generatePositions($symbol_, $buysell_, $tsize_, \%symbol_to_position_map_1, \%symbol_to_volume_map_1);
  }
}

foreach my $symbol_ (sort keys %symbol_to_position_map_0)
{
  #dump positions in EOW positions file
  if($dump_eow_positions == 1){
        if(exists ($symbol_to_position_map_1{$symbol_}) && $symbol_to_position_map_1{$symbol_} == $symbol_to_position_map_0{$symbol_} 
              && $symbol_to_volume_map_1{$symbol_} == $symbol_to_volume_map_0{$symbol_} && $symbol_to_position_map_1{$symbol_} != 0){
        	print "$symbol_$symbol_to_position_map_1{$symbol_}\n";
        }
  }
  else{
	  if(! exists ($symbol_to_position_map_1{$symbol_})){
	  	print "SYMBOL_NOT_IN_ORS_TRADES SYMBOL: $symbol_ BROKER_VALUES POS: $symbol_to_position_map_0{$symbol_} VOL: $symbol_to_volume_map_0{$symbol_}\n\n";
	  }
	  elsif ($symbol_to_position_map_1{$symbol_} != $symbol_to_position_map_0{$symbol_} 
	          || $symbol_to_volume_map_1{$symbol_} != $symbol_to_volume_map_0{$symbol_}){
	    print "SYMBOL: $symbol_ BROKER_VALUES POS: $symbol_to_position_map_0{$symbol_} VOL: $symbol_to_volume_map_0{$symbol_}\n";
	    print "SYMBOL: $symbol_ ORS_VALUES    POS: $symbol_to_position_map_1{$symbol_} VOL: $symbol_to_volume_map_1{$symbol_}\n\n";
	  }
	  elsif($symbol_to_position_map_1{$symbol_} != 0){
	  	print "Open position in SYMBOL: $symbol_ POS: $symbol_to_position_map_1{$symbol_} $symbol_to_volume_map_1{$symbol_}\n\n";
	  }
	  else{
	  	print "Perfect match for SYMBOL: $symbol_ POS: $symbol_to_position_map_1{$symbol_} $symbol_to_volume_map_1{$symbol_}\n\n"
	  }
  }
}

foreach my $symbol_ (sort keys %symbol_to_position_map_1)
{
  if(! exists ($symbol_to_position_map_0{$symbol_})){
    print "SYMBOL_NOT_IN_BROKER_TRADES SYMBOL: $symbol_ ORS_VALUES POS: $symbol_to_position_map_1{$symbol_} $symbol_to_volume_map_1{$symbol_}\n\n";
  }
}

sub generatePositions{
	my $symbol_ = shift;
	my $buysell_ = shift ;
    my $tsize_ = shift ;
    my $symbol_to_position_map = shift;
    my $symbol_to_volume_map = shift;
    
    if(index ($symbol_, "VX") != -1 && index ($symbol_, "_") != -1 ){
    	splitVxSpreads($symbol_, $buysell_, $tsize_, $symbol_to_position_map, $symbol_to_volume_map);
    	return;
    }
    
    if(index ($symbol_, "DOL") == 0 ){
        splitDOL($symbol_, $buysell_, $tsize_, $symbol_to_position_map, $symbol_to_volume_map);
        return;
    }
    
    if(index ($symbol_, "BAX") == 0  && length($symbol_) > 6){
        splitBaxSpreads($symbol_, $buysell_, $tsize_, $symbol_to_position_map, $symbol_to_volume_map);
        return;
    }
    
    
	if( ! ( exists $$symbol_to_position_map{$symbol_} )){
    $$symbol_to_position_map{$symbol_} = 0;
    $$symbol_to_volume_map{$symbol_} = 0;
    
  }
  
  if($buysell_ == 0){
    $$symbol_to_position_map{$symbol_} += $tsize_;
  }
  
  if($buysell_ == 1){
    $$symbol_to_position_map{$symbol_} -= $tsize_;
  }
  
  $$symbol_to_volume_map{$symbol_}+=$tsize_;
}
  
sub splitVxSpreads{
	my $symbol_ = shift;
    my $buysell_ = shift ;
    my $tsize_ = shift ;
    my $symbol_to_position_map = shift;
    my $symbol_to_volume_map = shift;
    
    my $base_symbol_length_ = length ("VX");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    if(! exists $spread_to_neg_symbol_ {$symbol_}){
	    my $symbol_1_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_,1)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	    $spread_to_neg_symbol_{$symbol_}=$symbol_1_name_with_expiry_;
	    $base_symbol_length_ = length ("VX-----");
	    $expiry_month_offset_ = $base_symbol_length_;
	    $expiry_year_offset_ = $expiry_month_offset_ + 1;
	    my $symbol_2_name_with_expiry_ = substr ($symbol_, 0, length("VX")).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	    $spread_to_pos_symbol_{$symbol_}=$symbol_2_name_with_expiry_;
    }
    
    generatePositions($spread_to_pos_symbol_{$symbol_}, ($buysell_^1), $tsize_, $symbol_to_position_map, $symbol_to_volume_map);
    generatePositions($spread_to_neg_symbol_{$symbol_}, ($buysell_), $tsize_, $symbol_to_position_map, $symbol_to_volume_map);
}

sub splitDOL{
    my $symbol_ = shift;
    my $buysell_ = shift ;
    my $tsize_ = shift ;
    my $symbol_to_position_map = shift;
    my $symbol_to_volume_map = shift;
    
    my $base_symbol_length_ = length ("DOL");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = "WDO".substr ($symbol_, $expiry_month_offset_);

    generatePositions($symbol_name_with_expiry_, $buysell_, ($tsize_ * 5), $symbol_to_position_map, $symbol_to_volume_map);
}

sub splitBaxSpreads{
    my $symbol_ = shift;
    my $buysell_ = shift ;
    my $tsize_ = shift ;
    my $symbol_to_position_map = shift;
    my $symbol_to_volume_map = shift;
    
    if(! exists $spread_to_neg_symbol_ {$symbol_}){
	    my $symbol2_pos = rindex($symbol_,"BAX");
	    my $symbol_1 = substr ($symbol_, 0, $symbol2_pos);
	    $symbol_1 = substr ($symbol_1, 0, 4).substr ($symbol_1, 5);
	    
	    my $symbol_2 = substr ($symbol_, $symbol2_pos);
	    $symbol_2 = substr ($symbol_2, 0, 4).substr ($symbol_2, 5);
	    
	    $spread_to_neg_symbol_ {$symbol_} = $symbol_2;
	    $spread_to_pos_symbol_ {$symbol_} = $symbol_1;
    }
    
    generatePositions($spread_to_pos_symbol_{$symbol_}, ($buysell_), $tsize_, $symbol_to_position_map, $symbol_to_volume_map);
    generatePositions($spread_to_neg_symbol_{$symbol_}, ($buysell_^1), $tsize_, $symbol_to_position_map, $symbol_to_volume_map);
}
