#!/usr/bin/perl

#for equity strats, computes the max notional total risk based on last day's price
#results average risk if look back numnber of days specified
use strict;
use warnings;
use FileHandle;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";

my $SPARE_HOME="/spare/local/".$USER."/";
my $SPARE_GCI_DIR=$SPARE_HOME."GCI/";
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $yyyymmdd_=`date +%Y%m%d`; chomp ( $yyyymmdd_ );

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDi

require "$GENPERLLIB_DIR/exists_with_size.pl";
require "$GENPERLLIB_DIR/create_enclosing_directory.pl";
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl";
require "$GENPERLLIB_DIR/calc_prev_date.pl";
require "$GENPERLLIB_DIR/calc_next_date.pl";

my $USAGE="$0 tradefilename sum/max[1/0=1] ";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $tradefilename_ = $ARGV[0] ;
my $date_ = $ARGV[1] ;
my $sum_ = "1";
if ( $#ARGV >= 1 )
{
  if ( $ARGV[1] == 0 )
  {
    $sum_ =  "";
  }
  else
  {
    $sum_ =  $ARGV[1] 
  }
}

my %prod_to_risk_ = () ;
my %prod_to_so_far_max_ = () ;
my $sum_across_all_prod_ = 0 ;
my $max_sum_across_all_prod_ = 0 ;
if ( ExistsWithSize ( $tradefilename_ ) )
{
  open TRADEFILE, "< $tradefilename_ " or PrintStacktraceAndDie ( " Could no open the tradefile for reading\n") ;
  while ( my $line_ = <TRADEFILE> ) 
  {
  	if ( index ( $line_, "PNLSPLIT")>= 0 ) { next ; } 
    my @line_words_ = split ( " ", $line_ ) ;
    if ( $#line_words_ <  16 ) { next; }
    my $prod_name_ = $line_words_[2] ;
    my @prod_words_ = split ( /\./, $prod_name_ ) ;
    $prod_name_ = $prod_words_[0] ;
    my $trd_qty_ = $line_words_[4] ;
    my $trd_price_ = $line_words_[5] ;
    my $buysell_ = $line_words_[3] ;
    if ( $sum_ )
    {
      $prod_to_risk_ {$prod_name_} += $trd_qty_ * $trd_price_ ;
    }
    else
    {
      if ( $buysell_ eq "B" )
      {
        $sum_across_all_prod_ += $trd_qty_ * $trd_price_ ;
      }
      else
      {
        $sum_across_all_prod_ -= $trd_qty_ * $trd_price_ ;
      }
      if ( abs ( $sum_across_all_prod_ > $max_sum_across_all_prod_ )  )
      {
        $max_sum_across_all_prod_ = abs ( $sum_across_all_prod_ ) ;
      }
      if ( $prod_to_risk_ {$prod_name_ } )
      {
        if ( $buysell_ eq "B" )
        {
          $prod_to_risk_{$prod_name_} += $trd_qty_ * $trd_price_ ;
        }
        else
        {
          $prod_to_risk_{$prod_name_} -= $trd_qty_ * $trd_price_ ;
        }
        if  ( abs ( $prod_to_risk_ { $prod_name_} ) > $prod_to_so_far_max_ {$prod_name_} ) 
        {
          $prod_to_so_far_max_ { $prod_name_} = abs ( $prod_to_risk_ {$prod_name_} ) ;
        }
      }
      else
      {
        if ( $buysell_ eq "B" )
        {
          $prod_to_risk_{$prod_name_} = $trd_qty_ * $trd_price_ ;
        }
        else
        {
          $prod_to_risk_{$prod_name_} = $trd_qty_ * $trd_price_ ;
        }
        $prod_to_so_far_max_ { $prod_name_} = abs ( $prod_to_risk_ {$prod_name_} ) ;
      }
    }
  }
}

my $total_sum_ = 0 ;
foreach  my $prod_ ( keys %prod_to_risk_  )      
{
  if ( $sum_ ) 
  {
    print "$prod_ $prod_to_risk_{$prod_}\n";
    $total_sum_+= $prod_to_risk_{$prod_};
  }
  else
  {
    print "$prod_ $prod_to_so_far_max_{$prod_}\n";
  }
}
if ( $sum_ ) 
{
  print "TOTAL $total_sum_\n";
}
else
{
  print "TOTAL $max_sum_across_all_prod_\n";
}

