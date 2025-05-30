# \file GenPerlLib/gen_ind_utils.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
use strict;
use warnings;
use feature "switch";

my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_exch_from_shortcode.pl"; #GetExchFromSHC
require "$GENPERLLIB_DIR/get_port_constituents.pl"; # GetPortConstituents
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

my %stdev_present_cache_ = ();
my %per_vol_present_cache_ = ();
my %lrdb_sources_for_dep_ = ();

sub IsProjectedIndicator
{
    my $indicator_line_ = shift;
    my $retval = 0;

    my @words_ = split(/\s+/, $indicator_line_);

    given ( $words_[0] )
    {
    	when ( "ProjectedPriceConstPairs" ) { $retval = 1; }
    	when ( "ProjectedPricePairs" ) { $retval = 1; }
    	when ( "ProjectedPriceTypePairs" ) { $retval = 1; }
    }
    $retval;
}

sub IsProjectedPairShc
{
  my $dep_shc_ = shift;
  my $indep_shc_ = shift;
  
  my $projected_pairs_filename_ = $HOME_DIR."/modelling/indicatorwork/projected_pairs.txt";
  open PROJECTED_PAIRS_FILEHANDLE, "< $projected_pairs_filename_" or PrintStacktraceAndDie("can't open file $projected_pairs_filename_ for reading\n");

  while( my $line_ = <PROJECTED_PAIRS_FILEHANDLE> )
  {
    chomp($line_);
    my @words_ = split( ' ', $line_);
    if($#words_ >= 1)
    {
      if($dep_shc_ eq $words_[0]) 
      {
        for(my $i=1; $i<=$#words_; $i++)
        {
          if($indep_shc_ eq $words_[$i])
          {
            close(PROJECTED_PAIRS_FILEHANDLE);
            return 1;
          }
        }
      }  
    }
  }
  close(PROJECTED_PAIRS_FILEHANDLE);

  return 0;
}

sub ShcHasExchSource
{
  my $shc_ = shift;
  my $exch_ = shift;
  if ( GetExchFromSHC($shc_) eq $exch_ ) { return 1; }

  # incase it is a port
  my @port_consti_ = GetPortConstituents($shc_);
  foreach my $shc_consti_ ( @port_consti_ )
  {
    if ( ShcHasExchSource($shc_consti_, $exch_) ) { return 1; }
  }
  return 0;
}

sub OrderInfoAbsent
{
  my $shc_ = shift;

  my @OrderFeedAbsentExchanges = qw( LIFFE TSE ESPEED RTS MICEX TMX HONGKONG CFE );

  if ( IsValidShc( $shc_ ) ) {
    if ( FindItemFromVec( GetExchFromSHC($shc_), @OrderFeedAbsentExchanges ) ) { return 1; }
  }

  foreach my $shc_consti_ ( GetPortConstituents($shc_) ) {
    if ( FindItemFromVec( GetExchFromSHC($shc_consti_), @OrderFeedAbsentExchanges ) ) { return 1; }
  }
  return 0;
}

sub IsStdevPresentForShc
{
#if shc_ is a valid shc or port , this will return 0 or 1 else it will return -1
  my $shc_ = shift;
  if ( exists($stdev_present_cache_{$shc_}) ) { return $stdev_present_cache_{$shc_}; }

  my @shc_list_ = ();
  if ( IsValidShc( $shc_ ) )  { @shc_list_ = ($shc_) ;} 
  else { @shc_list_ = GetPortConstituents($shc_); }

  if ( $#shc_list_ >=0 )
  {
    foreach my $t_shc_(@shc_list_)
    {
      my $t_found_ = `grep -c "SHORTCODE_STDEV $t_shc_" /spare/local/tradeinfo/PCAInfo/shortcode_stdev_DEFAULT.txt`; chomp($t_found_);
      if ( ! $t_found_) 
      {
        $stdev_present_cache_{$shc_} = 0; 
        return 0;
      } 
    }
        
    $stdev_present_cache_{$shc_} = 1; 
    return 1;
  }
  else
  {
    return -1;
  }
}

sub LoadLRDBSourcesForDep
{
  my $shc_ = shift;
  my $date_ = shift;
  if ( ! defined $date_ ) {
    $date_ = `date +%Y%m%d`; chomp ( $date_ );
  }

  if ( ! exists $lrdb_sources_for_dep_{ $shc_ } ) {
    my $cmd_ = "$HOME_DIR/basetrade_install/bin/print_lrdb_values $date_ $shc_";
    my @plines_ = `$cmd_ 2>/dev/null`; chomp ( @plines_ );
    @plines_ = grep { $_ =~ /^$shc_/ } @plines_;

    my @dep_sources_ = ( );
    foreach my $line_ ( @plines_ ) {
      my @pwords_ = split(/\s+/, $line_);
      my @depindep_ = split(/\^/, $pwords_[0] );

      if ( $depindep_[0] ne $shc_ ) { next; }
      push ( @dep_sources_, $depindep_[1] );
    }

    @{$lrdb_sources_for_dep_{ $shc_ } } = @dep_sources_;
  }
}

sub IsLRDBPairPresent
{
  my $dep_ = shift;
  my $indep_ = shift;

  LoadLRDBSourcesForDep ( $dep_ );

  if ( FindItemFromVec ( $indep_, @{$lrdb_sources_for_dep_{ $dep_ } } ) ) {
   return 1;
  } else {
   return 0;
  }
}

1
