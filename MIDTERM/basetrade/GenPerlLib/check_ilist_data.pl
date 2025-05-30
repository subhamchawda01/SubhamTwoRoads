# \file GenPerlLib/check_ilist_data.pl
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
use feature "switch"; # for given, when
use List::Util qw/max min/; # for max

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $USER=$ENV{'USER'};
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday, IsProductHolidayIncludingWeekends
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_port_constituents.pl"; # GetPortConstituents
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName

#arg 1 is date, arg2 is ilist file
#return 1 if any data source is missing, else return 0
sub CheckIndicatorData
{
  my $trading_date = $_[0];
  my $indicator_file = $_[1];
  if ( ! ( -e $indicator_file ) ) { return 1; }

  my $shc_vec_cmd_ = "$BIN_DIR/collect_shortcodes_from_model $indicator_file $trading_date";
  my $shc_vec_str_ = `$shc_vec_cmd_ 2>/dev/null`; chomp ( $shc_vec_str_ );
  my @shc_vec_ = grep { $_ ne "NONAME" } split(' ', $shc_vec_str_);

  foreach my $shc_ ( @shc_vec_ ) {
   if ( IsProductHolidayIncludingWeekends( $trading_date, $shc_ ) == 1 ) {
     return 1;
   }
  }
# We don't holidays for any shortcode
  return 0;
}

sub GetIndicatorShcVec
{
  my $indicator = $_[0];
  my $trading_date = $_[1];

  $indicator =~ s/^\s+//;

  my $shc_vec_cmd_ = "$BIN_DIR/collect_shortcodes_from_model \"$indicator\" $trading_date 1";
  my $shc_vec_str_ = `$shc_vec_cmd_ 2>/dev/null`; chomp ( $shc_vec_str_ );

  my @shc_vec_ = grep { $_ ne "NONAME" } split(' ', $shc_vec_str_);
  return @shc_vec_;
}

sub GetIlistShcVec
{
  my $t_ilist_ = $_[0];
  my $trading_date = $_[1];
  my $indc_shc_vec_ref_ = $_[2];

  my $shc_vec_cmd_ = "$BIN_DIR/collect_shortcodes_from_model $t_ilist_ $trading_date 2";
  my @shc_str_vec_ = `$shc_vec_cmd_ 2>/dev/null`; chomp ( @shc_str_vec_ );
#  print join("\n",@shc_str_vec_)."\n";

  @$indc_shc_vec_ref_ = ( );
  foreach my $shc_vec_str_ ( @shc_str_vec_ ) {
    my @shc_vec_ = grep { $_ ne "NONAME" } split(' ', $shc_vec_str_);
    push ( @$indc_shc_vec_ref_, \@shc_vec_ );
  }
}

#arg 1 is date, arg2 is ref to array of ilist_lines
#return array of 1 or 0. 1 means no data for that indicator
sub CheckIlistData
{
  if ( @_ < 2 ) 
  { 
    print STDERR "USAGE: CheckIndicatorData YYYYMMDD indicators_vec [indicators_shclist_ref] \n"; 
  }
  else 
  {
    my $trading_date = $_[0];
    my $all_lines = $_[1];
    my $indc_shcvec_ref_ = $_[2];
    if ( ! defined $indc_shcvec_ref_ ) { %$indc_shcvec_ref_ = ( ); }

    my @lines_shcvec_not_computed_ = grep { ! exists $$indc_shcvec_ref_{ $_ } } @$all_lines;
    if ( $#lines_shcvec_not_computed_ >= 0 ) {
      my $t_ilist_ = GetCSTempFileName ( $HOME_DIR."/indicatorwork/ilists" );
      open ILIST_HANDLE, "> $t_ilist_" or PrintStacktraceAndDie ( "Could not open $t_ilist_" );
      print ILIST_HANDLE join("\n", @lines_shcvec_not_computed_)."\n";
      close ILIST_HANDLE;

      my @t_indc_shcvec_ = ( );
      GetIlistShcVec( $t_ilist_, $trading_date, \@t_indc_shcvec_);
      foreach my $indc_idx_ ( 0..$#lines_shcvec_not_computed_ ) {
        $$indc_shcvec_ref_{ $lines_shcvec_not_computed_[ $indc_idx_ ] } = $t_indc_shcvec_[ $indc_idx_ ];
      }
      `rm -f $t_ilist_`;
    }

    my @data_absent_vector = ();
    my %product_is_holiday_ = ();

    foreach my $line_ ( @$all_lines )
    {
      my $orig_line_ = $line_;
      $line_ =~ s/^\s+//;
      my @words_ = split(/\s+/, $line_);

      my $data_absent_ = 0;
      foreach my $shc_ ( @{ $$indc_shcvec_ref_{ $orig_line_ } } ) {
        if ( ! defined  $product_is_holiday_{ $shc_ } ) {
          $product_is_holiday_{ $shc_ } = IsProductHolidayIncludingWeekends( $trading_date, $shc_ );
        }
        if ( $product_is_holiday_{ $shc_ } ) {
          $data_absent_ = 1;
          last;
        }
      }
      push ( @data_absent_vector, $data_absent_ );
    }
    return @data_absent_vector;
  }
}

#arg 1 is enddate, arg2 is num_of_days , arg3 is ref to shcvec
#return ref to datevec with data for all shortcodes 
sub GetDateVecWithDataForAllShcs 
{
  my $enddate_ = shift;
  my $numdays_ = shift;
  my $shcvecref_ = shift;
  my @datevec_ = ();

  my @shc_vec_ = ( );
  foreach my $shc_ ( @$shcvecref_ )
  {
    if ( IsValidShc( $shc_ ) )  {  push ( @shc_vec_, $shc_ ); }
    else { push ( @shc_vec_, GetPortConstituents($shc_) ); }
  }
  @shc_vec_ = GetUniqueList ( @shc_vec_ );

  my $days_to_consider_ = max ( 20, 2*$numdays_ );
  my $this_date_ = $enddate_ ;
  while ( $days_to_consider_ > 0 && $#datevec_ + 1 < $numdays_ )
  {
    my $data_avail_ = 1;
    foreach my $shc_ ( @shc_vec_ )
    {

      if ( IsProductHolidayIncludingWeekends($this_date_, $shc_) )
      {
        $data_avail_ = 0;
        last;
      }
    }
    if ( $data_avail_ == 1 )
    {
      push ( @datevec_, $this_date_ );
    }
    $this_date_ = CalcPrevWorkingDateMult($this_date_,1);
  }
  return \@datevec_;
}

1

