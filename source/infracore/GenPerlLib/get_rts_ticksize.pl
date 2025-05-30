#!/usr/bin/perl
use strict;
use warnings;
use POSIX;
use feature "switch";
my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $EXEC_DIR=$HOME_DIR."/LiveExec/bin" ;
require "$GENPERLLIB_DIR/calc_prev_date.pl"; #CalcPrevDate

my $file_prefix="/spare/local/files/RTS/tickvalues/rts-tick-value.";
my $date=`date +%Y%m%d`;
chomp ($date);

sub GetRTSTickSize
{

  my $shortcode = shift;
  my $retval = 1.0 ;
  my $min_px_increment = 1.0 ;

  given ( $shortcode )
  {

    when ( "RI_0" )
    {
      $retval = 0.65 ;
      $min_px_increment = 10 ;
    }

    when ( "Si_0" )
    {
      $retval = 1 ;
      $min_px_increment = 1 ;
    }

    when ( "GD_0" )
    {
      $retval = 32.401 ;
      $min_px_increment = 0.1 ;
    }

    when ( "ED_0" )
    {
      $retval = 32401 ;
      $min_px_increment = 0.0001 ;
    }

    when ( "SR_0" )
    {
      $retval = 1 ;
      $min_px_increment = 1 ;
    }

  }

  my $exch_symbol = `$EXEC_DIR/get_exchange_symbol $shortcode $date` ;

  for ( my $i = 0 ; $i <= 30 ; $i ++ )  #loop over 30 days, same as sec def
  {

    my $file = $file_prefix.$date.".txt" ;

    if ( -e $file ) {

      my $tick_size = `grep $exch_symbol $file | awk '{print \$NF}' | uniq` ;

      if ( $tick_size != 0.0 ) {

        $tick_size = $tick_size + 0 ;
        $tick_size = $tick_size / $min_px_increment ;
        $retval = $tick_size ;

        last; #break out of loop when value found

      }else{

        $date = CalcPrevDate ( $date ) ;

      }


    }else {

      $date = CalcPrevDate ( $date ); #Calculate Previoud Date

    }

  }

  $retval ; #tickval to return

}
