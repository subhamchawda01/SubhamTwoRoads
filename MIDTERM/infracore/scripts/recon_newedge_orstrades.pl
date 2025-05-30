#!/usr/bin/perl

use strict;
use warnings;
#use Array::Utils qw(:all);

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."/scripts";

require "$GENPERLLIB_DIR/calc_next_business_day.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # ExistsWithSize


# objective is to recon yyyymmdd_ ( TOKYO DATE ) 
# they could be in  

my $usage = "exec fdate_ tdate_ exchange_code_ exchange_name_ newedgecode_ riccode_ " ;

if ( scalar ( @ARGV ) < 6 )
{
    print $usage, "\n" ;
    exit ( 0 ) ;
}


my $fdate_= $ARGV [ 0 ] ;
my $tdate_= $ARGV [ 1 ] ;

my $exchange_code_ = $ARGV [ 2 ] ;
my $exchange_name_ = $ARGV [ 3 ] ;


my $newedgecode_= $ARGV [ 4 ] ;
my $riccode_= $ARGV [ 5 ] ;

my @newedge_current_day_trades_ = ( ) ;
my @newedge_next_day_trades_ = ( ) ;
my @newedge_trades_ = ( ) ;
my @ors_trades1_ = ( ) ;
my @ors_trades2_ = ( ) ;
my $start_time_ = -1 ;
my $end_time_ = -1 ;



for ( my $yyyymmdd_ = $fdate_ ; $yyyymmdd_ <= $tdate_ ; $yyyymmdd_ = CalcNextBusinessDay ( $yyyymmdd_ ) )
{
    my $prev_date_ = CalcPrevDate ( $yyyymmdd_ ) ; # check if this have to be a regular day for ors, most likely ?  
    my $next_date_ = CalcNextBusinessDay ( $yyyymmdd_ ) ; # check if this have to be a business day for newedge, most likey ?

    my $newedge_file_ = "NA" ;
    my $newedge_next_day_file_ = "NA" ;

    print "\n\n" ;
    
    if ( $yyyymmdd_>= 20111101  && $yyyymmdd_ <= 20111130 )
    {
	$newedge_file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/FIMTRNTEXT20111101_20111130.csv" ;
    }
    elsif ( $yyyymmdd_>= 20111201  && $yyyymmdd_ <= 20111231 )
    {
	$newedge_file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/FIMTRNTEXT20111201_20111231.csv" ;
    }
    elsif ( $yyyymmdd_>= 20120101  && $yyyymmdd_ <= 20120131 )
    {
	$newedge_file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/FIMTRNTEXT20120101_20120131.csv" ;
    }
    else
    {
 	$newedge_file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/GMITRN_" . $yyyymmdd_ . ".csv" ;
    }

    if ( $next_date_ >= 20111101  && $next_date_ <= 20111130 )
    {
	$newedge_next_day_file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/FIMTRNTEXT20111101_20111130.csv" ;
    }
    elsif ( $next_date_ >= 20111201  && $next_date_ <= 20111231 )
    {
	$newedge_next_day_file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/FIMTRNTEXT20111201_20111231.csv" ;
    }
    elsif ( $next_date_ >= 20120101  && $next_date_ <= 20120131 )
    {
	$newedge_next_day_file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/FIMTRNTEXT20120101_20120131.csv" ;
    }
    else
    {
 	$newedge_next_day_file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/GMITRN_" . $next_date_ . ".csv" ;
    }

    if ( -e $newedge_file_ )
    {

	@newedge_current_day_trades_ =  `awk -F',' '{ if ( ( \$1 == \"\\\"T\\\"\" || \$1 == \"\\\"FREID\\\"\" ) && ( \$24 == \"\\\"$exchange_code_\\\"\" || \$24 == \"\\\"FEXCH\\\"\" ) && ( \$25 == \"\\\"$newedgecode_\\\"\" || \$25 == \"\\\"FFC\\\"\") ) { gsub (\"\\"\","",\$0 ) ; print \$12"|"\$51"|"\$25"|"\$95"|"\$18"|"\$13 ; } }' $newedge_file_ | sort -k2n -t'|' ` ;

	chomp ( @newedge_current_day_trades_ ) ;

	if ( scalar ( @newedge_current_day_trades_ ) > 0 )
	{

	    for ( my $idx = 0 ; $idx  < scalar ( @newedge_current_day_trades_ ) ; $idx ++ )
	    {
		my @tks_ = (  ) ;

		@tks_ = split ( '\|' , $newedge_current_day_trades_ [ $idx ] ) ;
		

		if ( int ( $tks_ [ 1 ] )  <= 170000 ) 
		{
#		    print "$newedge_current_day_trades_[ $idx ]\n" ;
		    push ( @newedge_trades_, "$newedge_current_day_trades_[ $idx ]\n" ) ;
		}
		else
		{
       #	    print $tks_[ 1 ], "\n" ;
		}
	    }
	    
	}
	
    }


# since newedge has no operating brain ... seek three days farther

    my $count_ = 0 ;

    while ( -e $newedge_next_day_file_ && $count_ <=  0 )
    {
	@newedge_next_day_trades_ =  `awk -F',' '{ if ( ( \$1 == \"\\\"T\\\"\" || \$1 == \"\\\"FREID\\\"\" ) && ( \$24 == \"\\\"$exchange_code_\\\"\" || \$24 == \"\\\"FEXCH\\\"\" ) && ( \$25 == \"\\\"$newedgecode_\\\"\" || \$25 == \"\\\"FFC\\\"\") ) { gsub (\"\\"\","",\$0 ) ; print \$12"|"\$51"|"\$25"|"\$95"|"\$18"|"\$13 ; } }' $newedge_next_day_file_ | sort -k2n -t'|' ` ;

	chomp ( @newedge_next_day_trades_ ) ;

	$count_ = scalar ( @newedge_next_day_trades_ ) ;

#	print "KP :: DEBUG :: ", $newedge_next_day_file_ , " exists && count_ ", $count_, "\n" ;

	if ( $count_ > 0 )
	{

	    for ( my $idx = 0 ; $idx < $count_ ; $idx ++ )
	    {
		my @tks_ = (  ) ;
		
		@tks_ = split ( '\|' , $newedge_next_day_trades_ [ $idx ] ) ;
		
		if ( int ( $tks_ [ 1 ] ) > 170000 )
		{

		    $tks_ [ 0 ]  = $yyyymmdd_ ;
		    $newedge_next_day_trades_[ $idx ] = join ( '|',  @tks_ ) ;
		    push ( @newedge_trades_, $newedge_next_day_trades_[ $idx ] ) ;
#		    print "$newedge_next_day_trades_[ $idx ]\n" ;
		}
		else
		{
	#	    print $tks_[ 1 ], "\n" ;
		}
		
	    }
	    
	}
	else
	{
	    $next_date_ = CalcNextBusinessDay ( $next_date_ ) ; # check if this have to be a business day for newedge, most likey ?
	    $newedge_next_day_file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/GMITRN_" . $next_date_ . ".csv" ;
	}
	
    }

#now work with ors files 

    my @temp_ = split ( '_' , $riccode_ )  ;
    my $bcode_ = $temp_ [ 0 ] ;
    $bcode_ = $bcode_."1" ;  # this is hack to differentiate NK NKM 

    my ( $year, $month, $day ) = BreakDateYYYYMMDD ( $prev_date_ );    
    my $orsfile1_ = "/NAS1/data/ORSData/$exchange_name_/$year/$month/$day/$bcode_*$prev_date_*" ;

    ( $year, $month, $day ) = BreakDateYYYYMMDD ( $yyyymmdd_ );    
    my $orsfile2_ = "/NAS1/data/ORSData/$exchange_name_/$year/$month/$day/$bcode_*$yyyymmdd_*" ;


    my $f1_ = 0 ;
    my $f2_ = 0 ;

    $f1_ = `ls -ltr $orsfile1_ 2>/dev/null | wc -l  ` ;
    $f2_ = `ls -ltr $orsfile2_ 2>/dev/null | wc -l  ` ;

    
    if ( int ( $f1_ )  > 0 )
    {

	@ors_trades1_ = `$BIN_DIR/ors_binary_reader $riccode_ $prev_date_ | grep Exec | awk '{print \$3\$5":"\$9\$11\$12":"\$18":"\$28}' 2> /dev/null` ;
	chomp ( @ors_trades1_ ) ;
    }

    if ( int ( $f2_ )  > 0 )
    {
	@ors_trades2_ = `$BIN_DIR/ors_binary_reader $riccode_ $yyyymmdd_ | grep Exec | awk '{print \$3\$5":"\$9\$11\$12":"\$18":"\$28}' 2> /dev/null` ;
	chomp ( @ors_trades2_ ) ;
    }


    my @ors_all_trades_ = ( ) ;
    my @ors_trades_ = ( ) ;

    my %trade_buy_map_  = ( ) ;
    my %trade_buy_map_counter_ = ( ) ;

    my %trade_sell_map_  = ( ) ;
    my %trade_sell_map_counter_ = ( ) ;

    my %zero_qty_soas_ = ( ) ;
    my %saos_time_ = ( ) ;
    
    my $last_record_btime_ = 0 ;
    my $last_record_bsaos_ = 0 ;

    my $last_record_stime_ = 0 ;
    my $last_record_ssaos_ = 0 ;

    for ( my $i = 0 ; $i < scalar ( @ors_trades1_ ) ; $i++ ) 
    {

	my @tks_ = split ( ':' , $ors_trades1_[ $i ] ) ;

	my $tdate_ = $prev_date_ ;

	( $tdate_ , $tks_[ 3 ] ) = split ( ' ' , `$SCRIPTS_DIR/unixtime2TZ.sh $tks_[ 3 ]  TOK ` ) ;

	chomp ( $tks_[ 3 ] ) ;

	next if ( $tdate_ != $yyyymmdd_ ) ;

	if ( int ( $tks_[ 5 ] ) == 0 ) 
	{
	    $zero_qty_soas_ { "$tks_[ 4 ]" } += 1 ;
	    next ;
	}

	$saos_time_ { "$tks_[ 4 ]$tks_[ 3 ]" } += 1 ;


	if ( $tks_[ 2 ] eq 'B' )
	{
	    if ( ( $last_record_btime_ - $tks_[ 3 ] )  < 2 && $last_record_bsaos_ == $tks_[ 4 ] )
	    {
		$tks_[ 3 ] = $last_record_btime_ ;
		
	    }
	    
	    $trade_buy_map_{ "$tdate_|$tks_[ 3 ]|$tks_[ 0 ]|$tks_[ 1 ]|$tks_[ 5 ]" } = $tks_[ 4 ] ;
	    $trade_buy_map_counter_{ "$tdate_|$tks_[ 3 ]|$tks_[ 0 ]|$tks_[ 1 ]|$tks_[ 5 ]|$tks_[ 4 ]" } += 1 ;
	    $last_record_btime_ = $tks_[ 3 ] ;
	    $last_record_bsaos_ = $tks_[ 4 ] ;

	}
	if ( $tks_[ 2 ] eq 'S' )
	{
	    if ( ( $last_record_stime_ - $tks_[ 3 ] )  < 2 && $last_record_ssaos_ == $tks_[ 4 ] )
	    {
		$tks_[ 3 ] = $last_record_stime_ ;
		
	    }

	    $trade_sell_map_{ "$tdate_|$tks_[ 3 ]|$tks_[ 0 ]|$tks_[ 1 ]|$tks_[ 5 ]" } = $tks_[ 4 ];
	    $trade_sell_map_counter_{ "$tdate_|$tks_[ 3 ]|$tks_[ 0 ]|$tks_[ 1 ]|$tks_[ 5 ]|$tks_[ 4 ]" } += 1 ;
	    $last_record_stime_ = $tks_[ 3 ] ;
	    $last_record_ssaos_ = $tks_[ 4 ] ;
	}

	push ( @ors_all_trades_, join ( '|' ,$tdate_ ,  $tks_[ 3 ] , $tks_[ 0 ] , $tks_[ 1 ] , $tks_[ 5 ] , $tks_[ 2 ] , $tks_[ 4 ] ) ) ;

    }

    for ( my $i = 0 ; $i < scalar ( @ors_trades2_ ) ; $i++ ) 
    {
	
      	my @tks_ = split ( ':' , $ors_trades2_[ $i ] ) ;
	
	my $tdate_ = $yyyymmdd_ ;

	( $tdate_, $tks_[ 3 ] ) = split ( ' ' , `$SCRIPTS_DIR/unixtime2TZ.sh $tks_[ 3 ]  TOK ` ) ;

	chomp ( $tks_[ 3 ] ) ;

	next if ( $tdate_ != $yyyymmdd_ ) ;

	if ( int ( $tks_[ 5 ] ) == 0 ) 
	{
	    $zero_qty_soas_ { "$tks_[ 4 ]" } += 1 ;
	    next ;
	}

	$saos_time_ { "$tks_[ 4 ]$tks_[ 3 ]" } += 1 ;

	if ( $tks_[ 2 ] eq 'B' )
	{
	    if ( ( $last_record_btime_ - $tks_[ 3 ] )  < 2 && $last_record_bsaos_ == $tks_[ 4 ] )
	    {
		$tks_[ 3 ] = $last_record_btime_ ;
		
	    }

	    $trade_buy_map_{ "$tdate_|$tks_[ 3 ]|$tks_[ 0 ]|$tks_[ 1 ]|$tks_[ 5 ]" } = $tks_[ 4 ] ;
	    $trade_buy_map_counter_{ "$tdate_|$tks_[ 3 ]|$tks_[ 0 ]|$tks_[ 1 ]|$tks_[ 5 ]|$tks_[ 4 ]" } += 1 ;
	    $last_record_btime_ = $tks_[ 3 ] ;
	    $last_record_bsaos_ = $tks_[ 4 ] ;

	}

	if ( $tks_[ 2 ] eq 'S' )
	{
	    if ( ( $last_record_stime_ - $tks_[ 3 ] )  < 2 && $last_record_ssaos_ == $tks_[ 4 ] )
	    {
		$tks_[ 3 ] = $last_record_stime_ ;
		
	    }

	    $trade_sell_map_{ "$tdate_|$tks_[ 3 ]|$tks_[ 0 ]|$tks_[ 1 ]|$tks_[ 5 ]" } = $tks_[ 4 ];
	    $trade_sell_map_counter_{ "$tdate_|$tks_[ 3 ]|$tks_[ 0 ]|$tks_[ 1 ]|$tks_[ 5 ]|$tks_[ 4 ]" } += 1 ;
	    $last_record_stime_ = $tks_[ 3 ] ;
	    $last_record_ssaos_ = $tks_[ 4 ] ;
	}
	
	push ( @ors_all_trades_ , join ( '|' ,$tdate_ ,  $tks_[ 3 ] , $tks_[ 0 ] , $tks_[ 1 ] , $tks_[ 5 ] , $tks_[ 2 ] , $tks_[ 4 ] ) ) ;

	
    }


    my %internal_trades_ = ( ) ;

    foreach my $key ( keys %trade_buy_map_ )
    {
	
	if ( exists ( $trade_sell_map_{ $key } ) && 
	     ( ( $trade_sell_map_counter_ { "$key|$trade_sell_map_{ $key }" } > 1 ) || 
	       ( $trade_buy_map_counter_ { "$key|$trade_buy_map_{ $key }" } > 1 ) ) )
	    
	{
#	    print $key , " ", $trade_sell_map_{ $key }, " ", $trade_buy_map_{ $key }, "\n" ;
	    $internal_trades_{ "$key|B|$trade_buy_map_{ $key }" } += 1  ;
	    $internal_trades_{ "$key|S|$trade_sell_map_{ $key }" } += 1 ;
	    
	    if ( $trade_sell_map_counter_ { "$key|$trade_sell_map_{ $key }" } > 1 )
	    {
		$trade_sell_map_counter_ { "$key|$trade_sell_map_{ $key }" } -= 1 ;
	    }

	    if  ( $trade_buy_map_counter_ { "$key|$trade_buy_map_{ $key }" } > 1 )
	    {
		$trade_buy_map_counter_ { "$key|$trade_buy_map_{ $key }" }  -= 1 ;
	    }
	}
    }


    my $duplicate_saos_ = 0 ;

    foreach my $key ( keys %saos_time_ )
    {
	if ( $saos_time_ { $key } > 1 )
	{
	    $duplicate_saos_ += 1 ;
#	    print $key, " ",  $saos_time_{ $key },"\n" ;
	}
    }


    foreach my $orstrade_ ( @ors_all_trades_ )
    {
#	print $orstrade,"\n" ;

	if ( exists ( $internal_trades_{ $orstrade_ } ) )
	{
#	    print "exits\n" ;
	    if ( $internal_trades_{ $orstrade_ } >  0 ) 
	    {
		$internal_trades_{ $orstrade_ } -= 1 ; 
#		print $orstrade_, "\n" ;     # to get internal trades
	    }
	    else
	    {
#		print $orstrade_, "\n" ;  # to get external trades - 1
		push ( @ors_trades_ , $orstrade_ ) ;
	    }
	}
	else
	{
#	    print $orstrade_, "\n" ; # to get external trades  - 2 
	    push ( @ors_trades_ , $orstrade_ ) ;
	    
	}
    
    }

    print "no. of transaction records for $yyyymmdd_ ( TOK_DATE ) in ors_binary file:: ", scalar ( @ors_all_trades_) , " no.of duplicate: " , $duplicate_saos_ , " ",  scalar (@ors_trades_ ) - 2 * scalar ( keys %zero_qty_soas_ ) ,"\n" ;
    print "no. of transaction records for $yyyymmdd_ ( TOK_DATE ) in newedge_trans_file:: ", scalar( @newedge_trades_ ),"\n" ;

    if ( scalar ( keys %zero_qty_soas_ ) > 0 )
    {
	print "no of zero qty transactions: ", scalar ( keys %zero_qty_soas_ ), "\n" ; 
    }
    
    print "estimated internally matched transactions:: ", scalar ( keys %internal_trades_ ) + 2 * scalar ( keys %zero_qty_soas_ ) , "\n" ;

    ### ors_trades_ ### newedge_trades_ ### 

    my $n_pnl_ = 0 ;
    my $n_volumne_ = 0 ;
    my $n_position_ = 0 ;

    foreach my $n_trade_ ( @newedge_trades_ )
    {

	my @tkns_ = split ( '\|' , $n_trade_ ) ;

	if ( int ( $tkns_ [ 5 ] ) == 1 )
	{
	    $n_pnl_ -= $tkns_[ 3 ] * $tkns_[ 4 ] ;
	    $n_volumne_ += $tkns_[ 4 ] ;
	    $n_position_ += $tkns_ [ 4 ] ;

	}
	elsif ( int ( $tkns_[ 5 ] ) == 2 )
	{
	    $n_pnl_ += $tkns_[ 3 ] * $tkns_[ 4 ] ;
	    $n_volumne_ += $tkns_[ 4 ] ;
	    $n_position_ -= $tkns_ [ 4 ] ;
	}
	
    }
    
    print "newedge pnl:: " , $n_pnl_, " newdge volume:: ", $n_volumne_, " newedge position:: ", $n_position_ ,"\n" ;
    
} 
