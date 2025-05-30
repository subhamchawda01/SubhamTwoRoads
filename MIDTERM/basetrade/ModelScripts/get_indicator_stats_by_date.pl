#!/usr/bin/perl
# \file ModelScripts/get_indicator_stats_by_date.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
use strict;
use warnings;
use FileHandle;

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/array_ops.pl";
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE="$0 FILENAME START_DATE STOP_DATE";

if ( $#ARGV != 2 ) { print $USAGE."\n"; exit (0); }

my $input_file_ = $ARGV[0];
my $start_date_ = GetIsoDateFromStrMin1 ( $ARGV[1] );
my $stop_date_ = GetIsoDateFromStrMin1 ( $ARGV[2] );

my %indicator_to_correlation_ = ();

open INDICATOR_DB_, "<", $input_file_ or PrintStacktraceAndDie ( "Could not open file : $input_file_\n" );

while (my $line_ = <INDICATOR_DB_>) {
    my @words_ = split (' ', $line_);

    if ($#words_ < 5) {
#	printf "Ignoring Malformed line : %s\n", $line_;
	next;
    }

    # Form the indicator name to key into the hash.
    my $indicator_name_ = $words_[3];
    for (my $word_no_ = 4; $word_no_ <= $#words_; ++$word_no_) {
	$indicator_name_ = $indicator_name_." ".$words_[$word_no_];
    }

    if ($words_[0] >= $start_date_ && $words_[0] <= $stop_date_) {
	# This is an indicator within our timeframe.
#	push @{ $indicator_to_correlation_{$indicator_name_} }, $words_[2];
	$indicator_to_correlation_{$indicator_name_} {$words_[0]} = $words_[2];
    }
}
close INDICATOR_DB_;

# We have a list of correlations for each indicator
# Now we compute the mean, the std. dev & the median.
my %indicator_to_mean_ = ();
my %indicator_to_stdev_ = ();

# We will use this as the master hash when displaying the results.
# The 2 hashes above will be reverse indexed using the indicator names obtained from this hash.
my %indicator_to_median_ = ();

for my $indicator_ ( keys %indicator_to_correlation_ ) 
{
    # First the mean.
    my $sum_ = 0.0;
    my $no_values_ = 0;
    my $ref_map_ = $indicator_to_correlation_{$indicator_} ;
    my @dates_ = keys %$ref_map_ ;
    my @values = ();
    for ( my $i = 0 ; $i <= $#dates_ ; $i ++ )
    {
	push ( @values, $indicator_to_correlation_{$indicator_}{$dates_[$i]} ) ;
    }

    $indicator_to_median_{$indicator_} = GetMedianAndSort ( \@values );
    $indicator_to_mean_{$indicator_} = GetAverage ( \@values );
    $indicator_to_stdev_{$indicator_} = GetStdev ( \@values );
}

my @indicators_sorted_by_median_ = sort { abs($indicator_to_median_{$b}) <=> abs($indicator_to_median_{$a}) } keys %indicator_to_median_;
printf "#MEAN   STDEV    MEDIAN    INDICATOR\n";
for my $indicator_ ( @indicators_sorted_by_median_ ) 
{
    my $ref_map_ = $indicator_to_correlation_{$indicator_} ;
    my @dates_ = keys %$ref_map_ ;
    my $sample_size_ = $#dates_ + 1;

    # Change this line to obtain output in desired form.
#    printf "Mean : %-12.8lf | Std. Dev : %-12.8lf | Median : %-12.8lf | %-100s \n", $indicator_to_mean_{$indicator_}, $indicator_to_stdev_{$indicator_}, $indicator_to_median_{$indicator_}, $indicator_ ;
    printf "%.4lf %.4lf %.4lf %s \n", $indicator_to_mean_{$indicator_}, $indicator_to_stdev_{$indicator_}, $indicator_to_median_{$indicator_}, $indicator_ ;
}
