# \file GenPerlLib/array_ops.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#

use List::Util qw/max min/; # for max

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetSum 
{
    @_ == 1 or die ('Sub usage: $sum = GetSum(\@array);');
    my ($array_ref) = @_;
    my $sum = 0;
    my $count = scalar @$array_ref;
    if ( $count <= 0 )
    {
	return 0;
    }
    foreach (@$array_ref) 
    { 
	$sum += $_; 
    }
    return $sum ;
}

sub GetSumLosses
{
    @_ == 1 or die ('Sub usage: $average = GetSumLosses(\@array);');
    my ($array_ref) = @_;
    my $sum_losses_ = 0;
    my $count = scalar @$array_ref;
    if ( $count <= 1 )
    {
	return 0;
    }
    
    foreach (@$array_ref) 
    { 
	if ( $_ < 0 )
	{
	    $sum_losses_ += -1 * $_ ;
	}
    }
    return $sum_losses_ ;
}

sub GetGainToPainRatio
{
    @_ == 1 or die ('Sub usage: $average = GetGainToPainRatio(\@array);');
    my ($array_ref) = @_;
    my $sum_ = 0;
    my $sum_losses_ = 0;
    my $count = scalar @$array_ref;
    if ( $count <= 1 )
    {
	return 0;
    }
    
    foreach (@$array_ref) 
    { 
	if ( $_ < 0 )
	{
	    $sum_losses_ += -1 * $_ ;
	}
	$sum_ += $_;
    }
    my $gpr_ = 100;
    if ( $sum_losses_ > 0.001 )
    {
	$gpr_ = $sum_ / $sum_losses_ ;
    }
    return $gpr_ ;
}

sub GetMaxDrawdown
{
    @_ == 1 or die ('Sub usage: $sum_values_ = GetSum(\@array);');
    my ($array_ref) = @_;

    my $max_drawdown_ = 0;
    my $max_pnl_ = 0;
    my $current_pnl_ = 0;
    foreach (@$array_ref) 
    { 
	$current_pnl_ += $_;
	$max_pnl_ = max ( $max_pnl_, $current_pnl_ );
	my $current_drawdown_ = $max_pnl_ - $current_pnl_;
	$max_drawdown_ = max ( $max_drawdown_, $current_drawdown_ );
    }
    return $max_drawdown_;
}    

sub GetAverage 
{
    @_ == 1 or PrintStacktraceAndDie ( "Sub usage: $average = GetAverage(\@array);" );
    my ($array_ref) = @_;
    my $sum = 0;
    my $count = scalar @$array_ref;
    if ( $count <= 0 )
    {
	return 0;
    }
    foreach (@$array_ref) 
    { 
	$sum += $_; 
    }
    return $sum / $count;
}

sub GetStdev
{
    @_ == 1 or PrintStacktraceAndDie ( "Sub usage: $average = GetStdev(\@array);" );
    my ($array_ref) = @_;
    my $sum_l1 = 0;
    my $sum_l2 = 0;
    my $count = scalar @$array_ref;
    if ( $count <= 1 )
    {
	return 1;
    }
    
    foreach (@$array_ref) 
    { 
	$sum_l1 += $_; 
	$sum_l2 += $_ * $_;
    }
    return sqrt ( ( $sum_l2 - ( $sum_l1 * $sum_l1 / $count ) ) / ($count -1) ) ;
}

sub GetMeanDeviationFromTrend
{
    @_ == 1 or PrintStacktraceAndDie ( "Sub usage: $mean_deviation_ = GetMeanDeviationFromTrend ( \@array );" );
    my ($array_ref) = @_;
    my $average_ = GetAverage ( $array_ref );

    my $sum_l1 = 0;
    my $sum_devs_ = 0;
    my $count = scalar @$array_ref;
    if ( $count <= 1 )
    {
	return 1;
    }
    
    my $i = 0;
    foreach (@$array_ref) 
    { 
	$i++;
	$sum_l1 += $_;
	$sum_devs_ += ( $sum_l1 - ( $i * $average_ ) ) * ( $sum_l1 - ( $i * $average_ ) ) ;
    }
    return sqrt ( $sum_devs_ / $count ) ;
}

sub GetL2NormLosses
{
    @_ == 1 or PrintStacktraceAndDie ( "Sub usage: $average = GetStdev(\@array);" );
    my ($array_ref) = @_;
    my $sum_l2 = 0;
    my $count = scalar @$array_ref;
    if ( $count <= 1 )
    {
	return 1;
    }
    
    foreach (@$array_ref) 
    { 
	if ( $_ < 0 )
	{
	    $sum_l2 += $_ * $_;
	}
    }
    return sqrt ( $sum_l2 / $count ) ;
}

sub GetPosFrac
{
    @_ == 1 or PrintStacktraceAndDie ( "Sub usage: $fracpos = GetPosFrac(\@array);" );
    my ($array_ref) = @_;
    my $num_pos_ = 0;
    my $count = scalar @$array_ref;
    if ( $count <= 0 )
    {
	return 0;
    }
    foreach (@$array_ref) 
    { 
	if ( $_ > 0 )
	{
	    $num_pos_ ++;
	}
    }
    return $num_pos_ / $count;
}

sub GetPosFracThresh
{
    @_ == 1 or PrintStacktraceAndDie ( "Sub usage: $fracpos = GetPosFracThresh(\@array);" );
    my ($array_ref) = @_;
    my $num_pos_ = 0;
    my $num_neg_ = 0;
    my $count = scalar @$array_ref;
    if ( $count <= 0 )
    {
	return 0;
    }
    foreach (@$array_ref) 
    { 
	if ( $_ > 0 )
	{
	    $num_pos_ ++;
	}
	if ( $_ < -2 )
	{
	    $num_neg_ ++;
	}
    }
    if ( $num_pos_ + $num_neg_ <= 0 )
    {
	return 0;
    }
    return $num_pos_ / ( $num_pos_ + $num_neg_ );
}

sub GetMedianAndSort 
{
    @_ == 1 or PrintStacktraceAndDie ( "Sub usage: $median = GetMedianAndSort(\@array);" );
    my ($array_ref) = @_;
    my $count = scalar @$array_ref;

# If you want to sort a COPY of the array, leaving the original untouched
#    my @array = sort { $a <=> $b } @$array_ref;

    @$array_ref = sort { $a <=> $b } @$array_ref;

    if ($count % 2) 
    {
	return $$array_ref[int($count/2)];
    } 
    else 
    {
	return ($$array_ref[$count/2] + $array_ref[$count/2 - 1]) / 2;
    }
}

sub GetConsMedianAndSort 
{
    @_ == 1 or PrintStacktraceAndDie ( "Sub usage: $median = GetMedianAndSort(\@array);" );
    my ($array_ref) = @_;
    my $count = scalar @$array_ref;

# If you want to sort a COPY of the array, leaving the original untouched
#    my @array = sort { $a <=> $b } @$array_ref;

    @$array_ref = sort { $a <=> $b } @$array_ref;

    my $c_idx_ = int($count * 0.4);
    return $$array_ref[$c_idx_];
}

sub GetMedianConst 
{
    @_ == 1 or PrintStacktraceAndDie ( "Sub usage: $median = GetMedianConst(\@array);" );
    my ($array_ref) = @_;
    my $count = scalar @$array_ref;

# Sort a COPY of the array, leaving the original untouched
    my @array = sort { $a <=> $b } @$array_ref;

    if ($count % 2) 
    {
	return $array[int($count/2)];
    } 
    else 
    {
	return ($array[$count/2] + $array[$count/2 - 1]) / 2;
    }
}

sub GetMeanHighestQuartile
{
    @_ == 1 or PrintStacktraceAndDie ( "Sub usage: $mean_highest_quartile_ = GetMeanHighestQuartile(\@array);" );
    my ($array_ref) = @_;

    my @array_ = sort { $a <=> $b } @$array_ref;
    my $count = $#array_ + 1;

    my $start_index_ = int ( $count - ( $count / 4 ) );

    if ( $start_index_ >= $#array_ )
    {
	if ( $#array_ >= 0 )
	{
	    return $array_ [ $#array_ ];
	}

	return 0;
    }

    my $sum_ = 0;
    for ( my $i = $start_index_ ; $i <= $#array_ ; $i ++ )
    {
	$sum_ += $array_ [ $i ];
    }

    my $t_mean_ = ( $sum_ / ( $count - $start_index_ ) );

    return $t_mean_;
}

sub GetMeanLowestQuartile
{
    @_ == 1 or PrintStacktraceAndDie ( "Sub usage: $mean_lowest_quartile_ = GetMeanLowestQuartile(\@array);" );
    my ($array_ref) = @_;

    my @array_ = sort { $b <=> $a } @$array_ref;
    my $count = $#array_ + 1;

    my $start_index_ = int ( $count - ( $count / 4 ) );

    if ( $start_index_ >= $#array_ )
    {
	if ( $#array_ >= 0 )
	{
	    return $array_ [ $#array_ ];
	}

	return 0;
    }

    my $sum_ = 0;
    for ( my $i = $start_index_ ; $i <= $#array_ ; $i ++ )
    {
	$sum_ += $array_ [ $i ];
    }

    my $t_mean_ = ( $sum_ / ( $count - $start_index_ ) );

    return $t_mean_;
}

1;
