#!/usr/bin/perl
use strict;
use warnings;
use List::Util qw(first);

#Script takes as input the adf script's results and figures out the list 
#of pairs that shoule be done
if( $#ARGV != 5 )
{
  die "Usage: script <stat_file> <max_pairs_per_date> <adf_stat_thresh> <halflife_stat_thresh> <stability_percentile> <tstat_column_in_file>\n";
}

#stat_file has lines of the form sorted by ADF stat per date
#eg Stocks	TATAMOTORS_MARUTI	SDate: 20060101	EDate: 20070101	Statistic: -2.824106	Statistic: -2.892174	Statistic: -3.036392	Statistic: -2.842949	Half_Life: 37.713159
my $stat_file = $ARGV[0];
my $num_max_pairs = $ARGV[1];
my $adf_stat_thresh = $ARGV[2];
my $halflife_thresh = $ARGV[3];
my $stability_thresh = $ARGV[4];
my $tstat_col_in_file = $ARGV[5]; #in example above tstat for price reg no intercept is 9

my @dates = `cat $stat_file | awk '{ print \$6 }' | sort -u`; #all dates for which we have data in stat file
my @pairs = `cat $stat_file | awk '{ print \$2 }' | sort -u`; #all pairs for which we have data in stat file
my %adfstat_value_map = ();
my %halflife_value_map = ();
my %adfstab_value_map = ();

chomp @dates;
chomp @pairs;

#process file and store tstat and halflife values in maps
open FILE, "<$stat_file" or die "Could not open $stat_file \n";
while( my $line = <FILE> )
{
  chomp $line;
  my @tokens = split(' ', $line);
  my $map_key = $tokens[1]."_".$tokens[5];
  $adfstat_value_map{$map_key} = $tokens[$tstat_col_in_file];
  $halflife_value_map{$map_key} = $tokens[$#tokens];  
}
close FILE;

#process and store adf stability value map
my @map_keys = keys %adfstat_value_map;

for( my $ctr = 0; $ctr <= $#pairs; $ctr = $ctr + 1 )
{
  my @pair_match = grep { /$pairs[$ctr]/ } @map_keys;
  my @pair_match_sorted = sort @pair_match;
  my $adf_sum = 0.0;
  my $count = 0;
  #TODO - verify vector sort order
  for( my $ctr2 = 0; $ctr2 <= $#pair_match_sorted; $ctr2 = $ctr2 + 1 )
  {
    if( $adfstat_value_map{$pair_match_sorted[$ctr2]} <= -2.9 ) #parameterize it later if needed - TODO
    {
      $adf_sum = $adf_sum + 1.0;
    }
    $count = $count + 1;
    #add stability values in map only when sufficient number of samples are available
    if( $count > 12 )
    {
      $adfstab_value_map{$pair_match_sorted[$ctr2]} = $adf_sum/$count;
    }
  }
}

#convert raw adf stability values in percentile values
my @adfstab_map_keys = keys %adfstab_value_map;
my %old_adfstab_value_map = %adfstab_value_map;
for( my $ctr = 0; $ctr <= $#dates; $ctr = $ctr + 1 )
{
  my @match_keys = grep { /$dates[$ctr]/ } @adfstab_map_keys;
  my @match_values = ();
  for( my $ctr2 = 0; $ctr2 <= $#match_keys; $ctr2 = $ctr2 + 1 )
  {
    push @match_values, $adfstab_value_map{$match_keys[$ctr2]};
  }
  my @sorted_match_values = sort { $a <=> $b } @match_values;

  for( my $ctr2 = 0; $ctr2 <= $#match_keys; $ctr2 = $ctr2 + 1 )
  {
    my $index = first {$sorted_match_values[$_] >= $adfstab_value_map{$match_keys[$ctr2]}} 0..$#sorted_match_values;
    $adfstab_value_map{$match_keys[$ctr2]} = 100.0*$index/$#match_keys;
  }
}

#for every date get possible pairs
for( my $ctr = 0; $ctr <= $#dates; $ctr = $ctr + 1 )
{
  my @pair_match = grep { /$dates[$ctr]/ } @map_keys;
  my @potential_pairs = ();
  my @adfstat_array = ();
  my @adfstab_array = ();
  my @halflife_array = ();

  for( my $ctr2 = 0; $ctr2 <= $#pair_match; $ctr2 = $ctr2 + 1 )
  {
    if( $adfstat_value_map{$pair_match[$ctr2]} < $adf_stat_thresh &&
	exists $adfstab_value_map{$pair_match[$ctr2]} && 
        $adfstab_value_map{$pair_match[$ctr2]} > $stability_thresh &&
	$halflife_value_map{$pair_match[$ctr2]} < $halflife_thresh )
    {
      push @potential_pairs, $pair_match[$ctr2];
      push @adfstat_array, $adfstat_value_map{$pair_match[$ctr2]};
      push @adfstab_array, $adfstab_value_map{$pair_match[$ctr2]};
      push @halflife_array, $halflife_value_map{$pair_match[$ctr2]};
    }
  }
  my @adfstat_array_sorted = sort { $a <=> $b } @adfstat_array;
  my @adfstab_array_sorted = sort { $a <=> $b } @adfstab_array;
  my @halflife_array_sorted = sort { $a <=> $b } @halflife_array;    

  #convert all three metrics to percentile basis  
  my %ranked_pair_map = ();
  for( my $ctr2 = 0; $ctr2 <= $#potential_pairs; $ctr2 = $ctr2 + 1 )
  {
    my $adfstat_index = first {$adfstat_array_sorted[$_] >= $adfstat_value_map{$potential_pairs[$ctr2]}} 0..$#adfstat_array_sorted;
    my $adfstat_percentile_ = 100.0 - 100.0*$adfstat_index/$#adfstat_array_sorted;

    my $adfstab_index = first {$adfstab_array_sorted[$_] >= $adfstab_value_map{$potential_pairs[$ctr2]}} 0..$#adfstab_array_sorted;
    my $adfstab_percentile_ = 100.0*$adfstab_index/$#adfstab_array_sorted;

    my $halflife_index = first {$halflife_array_sorted[$_] >= $halflife_value_map{$potential_pairs[$ctr2]}} 0..$#halflife_array_sorted;
    my $halflife_percentile_ = 100.0*(1 - $halflife_index/$#halflife_array_sorted);

    $ranked_pair_map{$potential_pairs[$ctr2]} = ($adfstat_percentile_ + $adfstab_percentile_ + $halflife_percentile_)/3.0;
  }

  my @sorted_pairs = sort { $ranked_pair_map{$b} <=> $ranked_pair_map{$a} } keys %ranked_pair_map;
  my $mtc_str = "";
  my $count = 0;
  for( my $ctr2 = 0; $ctr2 <= $#sorted_pairs; $ctr2 = $ctr2 + 1 )
  {
    my @tokens = split('_', $sorted_pairs[$ctr2] );
    if( $mtc_str =~ /$tokens[0]/ || $mtc_str =~ /$tokens[1]/ )
    {
      next;
    } 
    
    #pick at most num_max_pairs 
    if( $count >= $num_max_pairs )
    {
      last;
    }
    $count = $count + 1;
    printf "Picked %s %f\n", $sorted_pairs[$ctr2], $ranked_pair_map{$sorted_pairs[$ctr2]};
    $mtc_str = $mtc_str.$tokens[0]."_".$tokens[1];
  }
}
