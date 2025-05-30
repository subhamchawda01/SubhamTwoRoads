#!/usr/bin/perl

# \file scripts/merge_two_ilists.pl
#
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes as input :
# key file_with_columns

use strict;
use warnings;

sub ProcLine ;

#my $SPARE_DIR="/spare/local/basetrade/";
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE="$0 input_ilist_ input_regdata_ out_ilist_ out_regdata_";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

my $in_ilist_ = $ARGV[0];
my $in_regdata_ = $ARGV[1];
my $out_ilist_ = $ARGV[2];
my $out_regdata_ = $ARGV[3];

my @indicators_list_ = ( );
my @count_arr_ = ( );
my @valid_indices_ = ( );
my $start_text_ = "";
my $end_text_ = "";

open FILE1H, "< $in_ilist_" or PrintStacktraceAndDie ( "$0 could not open $in_ilist_\n" );
my $start_ = 1;

while ( my $txt_line_ = <FILE1H> )
{
    chomp ( $txt_line_ );
    if ( $txt_line_ =~ /INDICATOR / )
    {
      push (@indicators_list_, $txt_line_ );
      push (@count_arr_, 0 );
      $start_ = 0;
    }
    elsif ( $start_ == 1 )
    {
      $start_text_ = $start_text_.$txt_line_."\n";
    }
    else
    {
      $end_text_ = $end_text_.$txt_line_."\n";
    }
}

close FILE1H;
open FILE2H, "< $in_regdata_" or PrintStacktraceAndDie ( "$0 could not open $in_regdata_\n" );
my $checked_once_ = 0;
my $num_lines_ = 0;

while ( my $txt_line_ = <FILE2H> )
{
    chomp ( $txt_line_ );
    my @txt_words_ = split ( ' ', $txt_line_ );
    if ( $checked_once_ == 0 )
    {
      if ( $#txt_words_ != $#indicators_list_ + 1 ) 
      {
        PrintStacktraceAndDie ( "Wrong ilist and regdata pair\n" );
      }
      else
      {
        $checked_once_ = 1;
      }
    }
    else
    {
      for ( my $i=1; $i<=$#txt_words_; $i++ )
      {
        if ( $txt_words_[ $i ]== 0 )
        {
          $count_arr_[ $i-1 ] = $count_arr_[ $i-1 ] + 1;
        }
      }
    }
    $num_lines_ ++;
}

close FILE2H;

for ( my $i=0; $i<=$#count_arr_; $i++ )
{
  if ( $count_arr_[ $i ] * 100 /$num_lines_ < 80 )
  {
    push ( @valid_indices_, $i );
  }
}

open FILE2H, "< $in_regdata_" or PrintStacktraceAndDie ( "$0 could not open $in_regdata_\n" );
open OUT1, "> $out_regdata_" or PrintStacktraceAndDie ( "$0 could not open $out_regdata_\n" );

while ( my $txt_line_ = <FILE2H> )
{
    chomp ( $txt_line_ );
    my @txt_words_ = split ( ' ', $txt_line_ );
    print OUT1 "$txt_words_[ 0 ] "; 
    for ( my $i=0; $i <= $#valid_indices_; $i++ )
    {
      print OUT1 "$txt_words_[ $valid_indices_[ $i ] + 1 ] ";
    }
    print OUT1"\n";
}
close OUT1;

open OUT2, "> $out_ilist_" or PrintStacktraceAndDie ( "$0 could not open $out_ilist_\n" );
print OUT2 "$start_text_";
for ( my $i=0; $i <= $#valid_indices_; $i++ )
{
  print OUT2 "$indicators_list_[ $valid_indices_[ $i ] ]\n";
}
print OUT2 "$end_text_";
close OUT2;

exit ( 0 );


