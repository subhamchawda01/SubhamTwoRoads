#!/usr/bin/perl

use strict;
use warnings;
use FileHandle;
use File::Basename;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl";  #Get commission for shortcode
require "$GENPERLLIB_DIR/get_hft_commission_discount_for_shortcode.pl"; # Get hft discount according to volume tiers for shortcode 
require "$GENPERLLIB_DIR/get_cme_commission_discount_for_shortcode.pl"; # Get CME comission change based on trading hours
require "$GENPERLLIB_DIR/get_cme_eu_volumes_for_shortcode.pl";  #Get CME comission change based on trading hours
require "$GENPERLLIB_DIR/get_number_of_working_days_BMF_between_two_dates.pl" ;
require "$GENPERLLIB_DIR/get_rts_ticksize.pl"; #RITickSize
require "$GENPERLLIB_DIR/get_commission_for_DI.pl"; #GetCommissionForDI
require "$GENPERLLIB_DIR/calc_next_business_day.pl"; 


my $USAGE = "$0 <year> <month[1-12]>";

if ( $#ARGV <1 ) { 
  print $USAGE."\n";
  exit ( 0 ) ;
}

my $year_ = $ARGV[0];
my $month_ = $ARGV[1];

if ( $month_ < 10 ) {
  $month_ = "0".int ( $month_ ) ;  
}

my $st_date_ = $year_."$month_"."01";
my $end_date_ = $year_."$month_"."31";
my $date_ = $st_date_ ; 

my %shc_to_vol_ = ();
while ( $date_ <= $end_date_ )  {
  my $exec_cmd_ = "grep \"CGB\\|CGF\\|SXF\\|BAX\" /NAS1/data/MFGlobalTrades/EODPnl/ors_pnls_".$date_.".txt";
  my @out_lines_ = `$exec_cmd_`; chomp ( @out_lines_ ) ;
  foreach my $line_ ( @out_lines_ ) {
    my @line_words_ = split ( " ", $line_ ) ;
    if ( $#line_words_ <= 12 ) { print "ERROR, malformed pnls.txt line $line_\n"; next ;  } 
    my $sec_ = $line_words_[1];
    my $vol_ = $line_words_[13];
    my $shc_exec_cmd_ = "$BIN_DIR/get_shortcode_for_symbol $sec_ $date_";
    my $shc_ = `$shc_exec_cmd_`; chomp ( $shc_ ) ;
    if ( index ( $shc_, "BAX_" ) >= 0 || index ( $shc_, "BAX") < 0 )  { 
      if ( exists ( $shc_to_vol_{$shc_} ) ) { $shc_to_vol_{$shc_} += $vol_ ; }
      else { $shc_to_vol_{$shc_} = $vol_ ; }
    }
    elsif ( index ( $shc_, "SP_BAX" ) >= 0 ) {
      my @shc_words_ = split ( "_", $shc_ ) ;
      if ( $#shc_words_ <= 2 ) {
        my $shc1_ = substr ( $shc_words_[1], 0, 3 ). "_". substr ( $shc_words_[1],3,1 ) ;
        if ( exists ( $shc_to_vol_{$shc1_} ) ) { $shc_to_vol_{$shc1_} += $vol_ ; }
        else { $shc_to_vol_{$shc1_} = $vol_ ; }

        my $shc2_ = substr ( $shc_words_[2], 0, 3 ). "_". substr ( $shc_words_[2],3,1 ) ;
        if ( exists ( $shc_to_vol_{$shc2_} ) ) { $shc_to_vol_{$shc2_} += $vol_ ; }
        else { $shc_to_vol_{$shc2_} = $vol_ ; }
      }
    }
  }
  $date_ = CalcNextBusinessDay ( $date_ ) ;
}

my $rebates_ = 0 ;
my $total_rebate_ = 0 ;
my $front_4_vol_ = 0 ;
my %shc_to_rebate_ = ();
my $other_ = 0 ;
my $other_vol_ = 0 ;
foreach my $shc_ ( keys %shc_to_vol_  ) {

  my $vol_ = $shc_to_vol_{$shc_};
  if ( $shc_ eq "BAX_0" or $shc_ eq "BAX_1" or $shc_ eq "BAX_2" or $shc_ eq "BAX_3" ) {
    $front_4_vol_ += $vol_;
  }
  elsif ( $shc_ eq "SXF_0" ) {
    $rebates_ = 0 ;
    my $num1_ = 24000; my $num2_ = 12000; my $num3_ = 6000; my $num4_ = 3000;
    if ( $vol_ > $num1_ ) { $rebates_ +=( 0.16 * ($vol_-$num1_ )  + ($num1_-$num2_) * 0.13 + ($num2_-$num3_) * 0.10 + ($num3_-$num4_) * 0.05 ) ; }
   elsif ( $vol_ > $num2_ ) { $rebates_ += (0.13 *  ($vol_-$num2_) + ($num2_-$num3_) * 0.10 + ($num3_-$num4_) * 0.05 ) ; } 
   elsif ( $vol_ > $num3_ ) { $rebates_ += ( 0.10 * ($vol_-$num3_) + ($num3_-$num4_) * 0.05 ) ; } 
   elsif ( $vol_ > $num4_ ) { $rebates_ += ( 0.05 * ($vol_-$num4_) ) ; } 
   $shc_to_rebate_{$shc_} = $rebates_;
   $total_rebate_ += $rebates_;
  }
  elsif ( $shc_ eq "CGB_0" ) { 
    $rebates_ = 0 ;
    my $num1_ = 40000; my $num2_ = 20000; my $num3_ = 10000; my $num4_ = 5000;
    if ( $vol_ > $num1_ ) { $rebates_ +=( 0.16 * ($vol_-$num1_ )  + ($num1_-$num2_) * 0.13 + ($num2_-$num3_) * 0.10 + ($num3_-$num4_) * 0.05 ) ; }
   elsif ( $vol_ > $num2_ ) { $rebates_ += (0.13 *  ($vol_-$num2_) + ($num2_-$num3_) * 0.10 + ($num3_-$num4_) * 0.05 ) ; } 
   elsif ( $vol_ > $num3_ ) { $rebates_ += ( 0.10 * ($vol_-$num3_) + ($num3_-$num4_) * 0.05 ) ; } 
   elsif ( $vol_ > $num4_ ) { $rebates_ += ( 0.05 * ($vol_-$num4_) ) ; } 
   $shc_to_rebate_{$shc_} = $rebates_;
   $total_rebate_ += $rebates_;
  } 
  else {
    $rebates_ = 0 ;
    $rebates_ += ( 0.16 * $vol_ ) ;
    $other_ += $rebates_;
    $other_vol_ += $vol_ ;
   $total_rebate_ += $rebates_;
  }

#  print "$shc_ $vol_ $rebates_\n";
}

$rebates_ = 0 ;
my $num1_ = 80000; my $num2_ = 40000; my $num3_ = 20000; my $num4_ = 10000;
if ( $front_4_vol_ > $num1_ ) { $rebates_ +=( 0.16 * ($front_4_vol_-$num1_ )  + ($num1_-$num2_) * 0.13 + ($num2_-$num3_) * 0.10 + ($num3_-$num4_) * 0.05 ) ; }
elsif ( $front_4_vol_ > $num2_ ) { $rebates_ += (0.13 *  ($front_4_vol_-$num2_) + ($num2_-$num3_) * 0.10 + ($num3_-$num4_) * 0.05 ) ; } 
elsif ( $front_4_vol_ > $num3_ ) { $rebates_ += ( 0.10 * ($front_4_vol_-$num3_) + ($num3_-$num4_) * 0.05 ) ; } 
elsif ( $front_4_vol_ > $num4_ ) { $rebates_ += ( 0.05 * ($front_4_vol_-$num4_) ) ; } 

$total_rebate_ += $rebates_;

print "#instrument #rebate #vol \n";
foreach my $key_ ( keys %shc_to_rebate_ )  {
  if ( $shc_to_rebate_{$key_} > 0 ) {
    print "$key_ ".$shc_to_rebate_{$key_}." ".$shc_to_vol_{$key_}."\n";
  }
}

if ( $rebates_ > 0 ) {
  print "BAX03 $rebates_  $front_4_vol_\n";
}

if ( $other_ > 0 ) {
  print "REST $other_ $other_vol_ \n";
}
print "\nTOTAL $total_rebate_ \n";
