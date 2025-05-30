#!/usr/bin/perl
use strict;
use warnings;
use Math::Complex ; # sqrt
use List::Util qw/max min/;    # for max

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
require "$GENPERLLIB_DIR/array_ops.pl"; 

# start
my $USAGE="$0 PROD DATE\n";

sub GetNMoreThanEqualValue ;
if ( $#ARGV < 1 )
{ 
    print $USAGE. "\n"; exit(0); 
}

my $prod_ = $ARGV[0];
my $date_ = $ARGV[1];

my @lines_ = `~/basetrade_install/bin/mds_log_l1_trade $prod_ $date_ 2>/dev/null` ;

my $current_midpx_ = 0 ;
my $avg_tr_px_ = 0 ;
my $n = 0 ;
my $size_ = 0 ;


my @avg_tr_px_minus_midpx_ = ( ) ;
my @sizes_ = ( ) ;
my @levels_ = ( ) ;

my %avg_tr_px_minus_midpx_map_ ;
my %sizes_map_ ;
my %levels_map_ ;

#L1    1397077199.692394 355     5       2199    2199.5  2       490
#L1    1397077199.980757 170     4       2199    2199.5  2       490
#TRADE 1397077199.980758 185     2199.5
#L1    1397077199.980758 170     4       2199    2199.5  2       305

foreach my $inline_ ( @lines_ )
{
    my @words_ = split( ' ', $inline_ );
    if ( $words_[0] eq "L1" )
    {
	if ( $n > 0 )
	{
	    $avg_tr_px_ = $avg_tr_px_ / $n ;
	    push ( @avg_tr_px_minus_midpx_ , abs( $avg_tr_px_ - $current_midpx_ ) ) ;
	    $avg_tr_px_minus_midpx_map_ { abs ( $avg_tr_px_ - $current_midpx_ ) } += 1 ;
	    push ( @levels_ , $n ) ;
	    $levels_map_{ $n } += 1 ;
	    push ( @sizes_ , $size_ ) ;
	    $sizes_map_{ $size_ } += 1 ;

	    $avg_tr_px_ = 0 ;
	    $n = 0 ;
	    $size_ = 0 ;
	}
	$current_midpx_ = ( $words_[4] + $words_[5] ) / 2 ;
    }
    if ( $words_[0] eq "TRADE" )
    {
	$avg_tr_px_ += $words_[ 3 ] ;
	$n += 1 ;
	$size_ += $words_[ 2 ] ;
    }
}

#for ( my $i = 0 ; $i < ( @avg_tr_px_minus_midpx_ ) ; $i ++ )
#{
#    print $avg_tr_px_minus_midpx_[$i]." ".$sizes_[$i]." ".$levels_[$i]."\n" ;
#}

# avg_tr_px_minus_midpx_
print  "TRADE_PX_MID_PX_DIFF\n";

for my $key ( sort { $a <=> $b } keys ( %avg_tr_px_minus_midpx_map_ ) )
{
    print $key." ".$avg_tr_px_minus_midpx_map_{ $key }."\n";
}

print  "SIZES\n";
for my $key ( sort { $a <=> $b } keys ( %sizes_map_ ) )
{
    print $key." ".$sizes_map_{ $key }."\n";
}

print  "LEVELS\n";
for my $key ( sort { $a <=> $b } keys ( %levels_map_ ) )
{
    print $key." ".$levels_map_{ $key }."\n";
}


my $avg_distance_ = GetAverage ( \@avg_tr_px_minus_midpx_ ) ;
my $median_distance_ = GetMedianConst ( \@avg_tr_px_minus_midpx_ ) ;
my $max_distance_ = $avg_tr_px_minus_midpx_ [ GetIndexOfMaxValue ( \@avg_tr_px_minus_midpx_ ) ] ;
my $min_distance_ = $avg_tr_px_minus_midpx_ [ GetIndexOfMinValue ( \@avg_tr_px_minus_midpx_ ) ] ;

my $n_more_than_avg_distance_ = GetNMoreThanEqualValue ( \@avg_tr_px_minus_midpx_ , $avg_distance_ ) ;
my $n_more_than_median_distance_ = GetNMoreThanEqualValue ( \@avg_tr_px_minus_midpx_ , $median_distance_ ) ;
my $n_more_than_max_distance_ = GetNMoreThanEqualValue ( \@avg_tr_px_minus_midpx_ , $max_distance_ ) ;
my $n_more_than_min_distance_ = GetNMoreThanEqualValue ( \@avg_tr_px_minus_midpx_ , $min_distance_ ) ;

print "stats(avg/median/max/min):distance:\n";
print $avg_distance_." ".$n_more_than_avg_distance_."\n" ;
print $median_distance_." ".$n_more_than_median_distance_."\n" ;
print $max_distance_." ".$n_more_than_max_distance_."\n" ;
print $min_distance_." ".$n_more_than_min_distance_."\n" ;

# sizes_
my $avg_actual_trade_size_ = GetAverage ( \@sizes_ ) ;
my $median_actual_trade_size_ = GetMedianConst ( \@sizes_ ) ;
my $max_actual_trade_size_ = $sizes_ [ GetIndexOfMaxValue ( \@sizes_ ) ] ;
my $min_actual_trade_size_ = $sizes_ [ GetIndexOfMinValue ( \@sizes_ ) ] ;

my $n_more_than_avg_actual_trade_size_ = GetNMoreThanEqualValue ( \@sizes_ , $avg_actual_trade_size_ ) ;
my $n_more_than_median_actual_trade_size_ = GetNMoreThanEqualValue ( \@sizes_ , $median_actual_trade_size_ ) ; 
my $n_more_than_max_actual_trade_size_ = GetNMoreThanEqualValue ( \@sizes_ , $max_actual_trade_size_ ) ;
my $n_more_than_min_actual_trade_size_ = GetNMoreThanEqualValue ( \@sizes_ , $min_actual_trade_size_ ) ; 

print "stat:sizes:\n";
print $avg_actual_trade_size_." ".$n_more_than_avg_actual_trade_size_."\n" ;
print $median_actual_trade_size_." ".$n_more_than_median_actual_trade_size_."\n" ;
print $max_actual_trade_size_." ".$n_more_than_max_actual_trade_size_."\n" ;
print $min_actual_trade_size_." ".$n_more_than_min_actual_trade_size_."\n" ;


# levels_

my $avg_levels_swept_ = GetAverage ( \@levels_ ) ;
my $median_levels_swept_ = GetMedianConst  ( \@levels_ ) ;
my $max_levels_swept_ = $levels_ [ GetIndexOfMaxValue ( \@levels_ ) ] ;
my $min_levels_swept_ = $levels_ [ GetIndexOfMinValue ( \@levels_ ) ] ;

my $n_more_than_avg_levels_swept_ = GetNMoreThanEqualValue ( \@levels_ , $avg_levels_swept_ ) ; 
my $n_more_than_median_levels_swept_ = GetNMoreThanEqualValue ( \@levels_ , $median_levels_swept_ ) ; 
my $n_more_than_max_levels_swept_ = GetNMoreThanEqualValue ( \@levels_ , $max_levels_swept_ ) ; 
my $n_more_than_min_levels_swept_ = GetNMoreThanEqualValue ( \@levels_ , $min_levels_swept_ ) ;

print "stat:levels:\n";
print $avg_levels_swept_." ".$n_more_than_avg_levels_swept_."\n" ;
print $median_levels_swept_." ".$n_more_than_median_levels_swept_."\n" ;
print $max_levels_swept_." ".$n_more_than_max_levels_swept_."\n" ;
print $min_levels_swept_." ".$n_more_than_min_levels_swept_."\n" ;


sub GetNMoreThanEqualValue
{
    @_ == 2 or PrintStacktraceAndDie ( "Sub usage: \$value_ = GetNMoreThanEqualValue ( \@array , \$value_ ); " ) ;
    my $array_ = shift;
    my $value_ = shift;
    
    my $retval = 0; 
    foreach my $t_v ( @$array_ ) 
      { 
	if ( $t_v >= $value_ )
	  {
	    $retval += 1;
	  }
      }
    return $retval;
}






