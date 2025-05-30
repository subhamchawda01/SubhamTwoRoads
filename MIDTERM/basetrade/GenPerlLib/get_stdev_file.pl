# \file GenPerlLib/get_stdev_file.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#

use Math::Complex ; # sqrt

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetStdevOfNonZeroValues
{
    my $datafilename_ = shift;
    my $startcolindex_ = shift;

    my $lastcolindex_ = -1;
    my @l1_sums_ = ();
    my @l2_sums_ = ();
    my @num_lines_ = ();

    open ( INFILE, "< $datafilename_" ) or PrintStacktraceAndDie ( " --f and no filename!\n" );
    while ( my $inline_ = <INFILE> ) 
    {
	chomp ( $inline_ );
	my @words_ = split ( ' ', $inline_ );
	if ( $lastcolindex_ == -1 ) 
	{ #first iteration
	    $lastcolindex_ = $#words_;
	    for ( my $i = $startcolindex_ ; $i <= $lastcolindex_ ; $i ++ ) 
	    {
		push ( @l1_sums_, 0 );
		push ( @l2_sums_, 0 );
		push ( @num_lines_, 0 );
	    }
	}
	if ( $#words_ >= $lastcolindex_ ) 
	{
	    for ( my $i = $startcolindex_; $i <= $lastcolindex_ ; $i ++ ) 
	    {
		my $this_val = $words_[$i];
		if ( ( $this_val > 0.00000001 ) ||
		     ( $this_val < -0.00000001 ) )
		{ # only ad it to l1 and l2 summaries if the value is strictly nonzero
		    $l1_sums_[($i-$startcolindex_)] += $this_val;
		    $l2_sums_[($i-$startcolindex_)] += $this_val * $this_val;
		    $num_lines_[($i-$startcolindex_)] ++;
		}
	    }
	}
    }
    close ( INFILE );
    
    my @mean_values_;
    my @stdev_values_;
    for ( my $i = $startcolindex_ ; $i <= $lastcolindex_ ; $i ++ ) 
    {
	my $column_index_ = ($i-$startcolindex_);
	if ( ($num_lines_[$column_index_]) > 1 ) 
	{
	    push ( @mean_values_, ( $l1_sums_[$column_index_] / ($num_lines_[$column_index_]) ) ) ;
	    push ( @stdev_values_, sqrt ( ( $l2_sums_[$column_index_] - ( $l1_sums_[$column_index_] * $l1_sums_[$column_index_] / ($num_lines_[$column_index_]) ) ) / ($num_lines_[$column_index_] -1) ) ) ;
	} 
	else 
	{
	    push ( @mean_values_, 0 ) ;
	    push ( @stdev_values_, 0 ) ;
	}
##       printf "%d %d\n", $l1_sums_[$column_index_], $l2_sums_[$column_index_] ;
## 	 printf "Col:%d %d %f %f\n", $i, $num_lines_[$column_index_], $mean_values_[$column_index_], $stdev_values_[$column_index_] ;
    }
    @stdev_values_;
}

1;
