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

my $USAGE="$0 shortcode ilist start_date num_days_lookback starthhmm endhhmm pred_duration(s)";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $strat_name_ = $ARGV[0] ;
my $date_ = $ARGV[1] ;
my $number_of_days_to_look_back_ = 0 ;
my $compute_actual_risk_taken_ = "" ;
my $sum_ = "1";
if ( $#ARGV >= 2 )
{
  $number_of_days_to_look_back_ = $ARGV[2] ;
  if ( $#ARGV >= 3 )
  {
    $compute_actual_risk_taken_ = $ARGV[3] ;
  } 
  if ( $#ARGV >= 4 )
  {
    if (  $ARGV[4] == 0 ) 
    {
      $sum_ = "";
    }
  }
}

my @date_vec_ = ();
my $index_ = 0 ;
my %prod_to_max_pos_ = () ;
push ( @date_vec_ , $date_ ) ;
my $this_date_ = $date_ ;
while ( $index_ < $number_of_days_to_look_back_ ) 
{
  $this_date_ = CalcPrevWorkingDateMult ( $this_date_, 1 ) ;
  if ( ValidDate ( $this_date_ ) && ! IsDateHoliday ( $this_date_ ) )
  {
    push ( @date_vec_, $this_date_ ) ;
    $index_++ ;
  }
}
$index_ = 0 ;
if ( ExistsWithSize ( $strat_name_ ) )
{
  my $strat_im_name_ = `cat $strat_name_ | awk '{print \$2}'`; chomp ( $strat_im_name_ ) ;
  open STRAT_IM_CAT, "< $strat_im_name_ " or PrintStacktraceAndDie ( " COuld not open the im strat for reading \n" ) ;
  my @strat_lines_ =  <STRAT_IM_CAT> ;
  foreach  my $line_ ( @strat_lines_ ) 
  {
    if ( index ( $line_ , "STRATEGYLINE" ) >= 0 )
    {
      my @line_words_ = split (" ", $line_ ) ; chomp ( @line_words_ ) ;
      my $prod_ = $line_words_[1] ;
      my $paramname_ = $line_words_[3] ;
      my $unit_trade_size_ = `cat $paramname_ | grep UNIT_TRADE_SIZE | awk '{print \$3}'`; chomp ( $unit_trade_size_ ) ;
      if ( ! $unit_trade_size_  ) { $unit_trade_size_ = 0 ; } 
      my $mur_ = `cat $paramname_ | grep MAX_UNIT_RATIO | awk '{print \$3}'`; chomp ( $mur_ ) ;
      my $max_position_ = 0 ;
      if ( $mur_ )
      {
        $max_position_ = $mur_ * $unit_trade_size_  ;
      }
      else
      {
        $max_position_ = `cat $paramname_ | grep MAX_POSITION | awk '{print \$3}'`; chomp ( $max_position_ ) ;
        if ( ! $max_position_ ) { $max_position_ = 0 ; } 
      }
      $prod_to_max_pos_{$prod_} = $max_position_ ;
    }
  }
}

$index_ = 0 ;
my $total_risk_ = 0 ;
my %prod_to_risk_ = () ;
my %prod_to_this_day_risk_ = () ;
print "RISK\nDate     TOTAL ";
foreach my $k_ ( keys %prod_to_max_pos_ )      
{
  printf ( "%8s ", $k_ ) ;
};
print "\n";
my $this_day_risk_ = 0 ;
if ( not $compute_actual_risk_taken_ ) 
{
  foreach my $tradingdate_ ( @date_vec_  )
  {  
    foreach  my $prod_ ( keys %prod_to_max_pos_  )      
    {
      $prod_to_this_day_risk_ {$prod_} = 0 ;
      my $exec_cmd_ = "$BIN_DIR/price_printer SIM $prod_ $tradingdate_ BMF_EQ | grep MktSizeWPrice | tail -n1 | awk '{print \$5 }'";
      my $last_traded_price_ = `$exec_cmd_`; chomp ( $last_traded_price_ ) ;

      $this_day_risk_ += $last_traded_price_ * $prod_to_max_pos_ {$prod_};
      $prod_to_this_day_risk_ {$prod_} = $last_traded_price_ * $prod_to_max_pos_ {$prod_};
      $prod_to_risk_ { $prod_ } += $prod_to_this_day_risk_ {$prod_ };
    }
    printf "$tradingdate_ %8d ", $this_day_risk_ ;
    foreach  my $prod_ ( keys %prod_to_max_pos_  )      
    {
      printf " %8d",$prod_to_this_day_risk_{$prod_};
    }
    print "\n";
    $total_risk_ += $this_day_risk_ ;
    $index_++ ;
  }
}
else
{
  my $prog_id_ = `date +%N`; chomp( $prog_id_ ) ; 
  foreach my $tradingdate_ ( @date_vec_  )
  {
    my $exec_cmd_ = "$BIN_DIR/sim_strategy SIM $strat_name_ $tradingdate_ $prog_id_ ADD_DBG_CODE -1 ";
    `$exec_cmd_`;
    my $tradefile_ = "/spare/local/logs/tradelogs/trades.$tradingdate_.$prog_id_";
    if ( ExistsWithSize ( $tradefile_ ) )
    {
      $exec_cmd_ = "$MODELSCRIPTS_DIR/get_max_notional_risk_from_tradefile.pl $tradefile_ $sum_  ";
      my @outlines_ = `$exec_cmd_`; chomp ( $exec_cmd_ ) ;
      foreach my $line_  ( @outlines_ )  
      {
        my @line_words_ = split ( " ", $line_ ) ;
        if ( index ( $line_, "TOTAL" )  >= 0 ) 
        {
          $this_day_risk_ = $line_words_[1] ; 
        }
        else
        {
          $prod_to_this_day_risk_{ $line_words_[0] } = $line_words_[1] ;
          $prod_to_risk_ { $line_words_[0] } += $prod_to_this_day_risk_ {$this_day_risk_ };
        }
      }
      printf "$tradingdate_ %8d ", $this_day_risk_  ;
      foreach  my $prod_ ( keys %prod_to_max_pos_  )      
      {
        printf " %8d ",$prod_to_this_day_risk_ { $prod_};
      }
      print "\n";
      $total_risk_ += $this_day_risk_ ;
      $index_++ ;
    }
  }
}
print "Avg ".int ( $total_risk_/$index_ ) ;
foreach  my $prod_ ( keys %prod_to_max_pos_  )      
{
  printf " %8d ",$prod_to_risk_{$prod_}/$index_
}
print "\n"
