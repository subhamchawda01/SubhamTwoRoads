#!/usr/bin/perl
#
# \file ModelScripts/compute_daily_l1_norm.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
# modelfile startdate enddate start_hhmm end_hhmm

my $USAGE="$0 regdata_path_";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $regdatafile_ = $ARGV [ 0 ];

my $corr_ = `~/basetrade_install/bin/get_dep_corr $regdatafile_`;
my $stdev_ = `Rscript  ~/basetrade/RScripts/get_stdev.R $regdatafile_`;

sub PrintArray
{
  my ($array_) = @_;
  for( my $i = 0; $i<=$#$array_; $i++ )
  {
      print "$$array_[$i] ";
  }
  print "\n";
}



my @corr_array_ = split ' ', $corr_;
my @stdev_array_all_ = split ' ', $stdev_;

my $product_stdev_ = $stdev_array_all_[0];

my @stdev_array_ = ();
for( my $i = 1; $i <= $#stdev_array_all_; $i ++ )
{
 push( @stdev_array_, $stdev_array_all_[$i] );
}

my @multiples_ = (0, 0.5, 1, 2, 4);


for( my $i = 2; $i <= $#stdev_array_all_ + 1; $i ++ )
{
  printf ("%.2f  ", $corr_array_[$i - 2]);
  for( my $j = 0; $j <= $#multiples_; $j ++ )
  { 
    my $sigma_ = $stdev_array_[$i - 2] * $multiples_[$j];
    my $neg_sigma_ = -1 * $stdev_array_[$i - 2] * $multiples_[$j];
    my $total_points_ 	= `cat $regdatafile_ | awk '{if ( (\$$i >= $sigma_ || \$$i <= $neg_sigma_) ) print \$$i; }' | wc -l`; chomp($total_points_);
    my $points_ 	= `cat $regdatafile_ | awk '{if ( (\$$i >= $sigma_ || \$$i <= $neg_sigma_) && \$$i * \$1 > 0 ) print \$$i; }' | wc -l`; chomp($points_);
    if($total_points_ != 0 )
    {
      my $accuracy_ = $points_/ $total_points_ * 100;
      printf ("%.1f  ", $accuracy_);
    }
    else
    {
      printf ( "NA  ");
    }
  }
  print "\n";
}
