#!/usr/bin/perl

# \file scripts/compare_ec2_and_ny_globalresults.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes as input:
# strategylistfile ( a file with lines with just 1 word corresponding to the name of a strategyfile )
# resultslistfile ( a file with multiple lines corresponding to the results on that day for that file )
# tradingdate
#
# Basically after running simulations for a day,
# this script is called,
# for reading the strategylistfile and resultslistfile
# and printing the values in the appropriate file in global_results_base_dir

use strict;
use warnings;
use File::Basename;
use Fcntl qw (:flock);

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELLING_REPO=$HOME_DIR."/modelling/";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE="$0 prod date ec2_globalresultsdir ny_globalresultsdir [print_not_found ( Y ) ]";
my $print_not_found_ = "Y";
if ( $#ARGV < 3 ) { print $USAGE."\n";exit; }
my $prod_name_ = $ARGV[0];
my $date_ = $ARGV[1];
my $ec2_results_base_dir_ = $ARGV[2];
my $ny_results_base_dir_ = $ARGV[3];
$print_not_found_ = $ARGV[4];

my $prods_ = "";
if ( $prod_name_ eq "ALL" || $prod_name_ eq "all" )
{
  my $cmd_ = "cat /spare/local/tradeinfo/curr_trade_prod_list.txt"; 
  $prods_ = `$cmd_`; chomp ( $prods_ );
}
my @prod_list_ = split ( ' ', $prods_ ) ;

if ( $date_ eq "YESTERDAY" )
{
  $date_ = `cat /tmp/YESTERDAY_DATE`; chomp ( $date_ ) ;
}
my @not_matching_resuls_ = () ;
my @not_found_results_ = () ;
my $mail_body_ = "";
foreach my $prod_ ( @prod_list_ ) 
{
  print $prod_."\n";
  my $ec2_results_dir_ = $ec2_results_base_dir_."/".$prod_."/";
  my $ny_results_dir_= $ny_results_base_dir_."/".$prod_."/";

  my ( $yyyy_, $mm_, $dd_ ) = BreakDateYYYYMMDD ( $date_ ) ;

  my $ec2_result_file_ = $ec2_results_dir_."/".$yyyy_."/".$mm_."/".$dd_."/results_database.txt";
  my $ny_results_file_ = $ny_results_dir_."/".$yyyy_."/".$mm_."/".$dd_."/results_database.txt";

  open EC2_RESULTS_FILE , "< $ec2_result_file_ "; #  or  PrintStacktrace ( "Could not open file $ec2_result_file_ for reading.\n" ); 
  if ( not fileno ( EC2_RESULTS_FILE )  ) { next; }
  my @ec2_results_ = <EC2_RESULTS_FILE>;
  close ( EC2_RESULTS_FILE ) ;

  open NY_RESULT_FILE, "< $ny_results_file_ " ; # or PrintStacktrace ( " Could not open file $ny_results_file_ for reading.\n" ) ; 
  if ( not fileno ( NY_RESULT_FILE ) ) { next; } 
  while ( my $line_ = <NY_RESULT_FILE> )
  {
    my @line_words_ = split ( ' ', $line_ ) ;
    if ( $#line_words_ < 15 ) { print "Malformed line: $line_ \n"  ; }
    else
    {
      my $strat_name_ = $line_words_[0] ;
      my $strat_found_ = "";
      foreach my $ec2_line_  ( @ec2_results_ ) 
      {
        my @ec2_res_line_words_ = split ( ' ', $ec2_line_ ) ;
        if ( $#ec2_res_line_words_ < 15 ) { print "Malformed line in EC2  res: $ec2_line_ \n"  ; }
        else
        {
          if ( $ec2_res_line_words_[0] eq $strat_name_ )
          {
            $strat_found_ = "true";
            print "Compariing: ".$strat_name_."\n";
            my $i = 1 ;
            for ( $i = 1; $i < 8; $i++ )
            {
              if ( $line_words_[$i] != $ec2_res_line_words_[$i] ) 
              { 
                if ( $i == 2 ) #pnl line
                {
                  if ( abs  ( $line_words_[$i] - $ec2_res_line_words_[$i] )/$line_words_[$i]  < 0.05  )
                  {
                    last ;
                  }
                  else
                  {
                    push ( @not_matching_resuls_, " STRAT: $strat_name_ \n       NY: ". join ( ' ', @line_words_[1..$#line_words_]) . "\n       EC2: ". join ( ' ', @ec2_res_line_words_[1..$#ec2_res_line_words_] ) ) ;
                  }
                }
                elsif ( $i == 3 ) #vol line_ 
                {
                  if ( abs  ( $line_words_[$i] - $ec2_res_line_words_[$i] )/$line_words_[$i]  < 0.01  )
                  {
                    last ;
                  }
                  else
                  {
                    push ( @not_matching_resuls_, " STRAT: $strat_name_ \n       NY: ". join ( ' ', @line_words_[1..$#line_words_]) . "\n       EC2: ". join ( ' ', @ec2_res_line_words_[1..$#ec2_res_line_words_] ) ) ;
                  }
                }
                else
                {
                  push ( @not_matching_resuls_, " STRAT: $strat_name_ \n       NY: ". join ( ' ', @line_words_[1..$#line_words_]) . "\n       EC2: ". join ( ' ', @ec2_res_line_words_[1..$#ec2_res_line_words_] ) ) ;
                }
                last ;
              }
            }
          }
          else
          {
            next ;
          }
        } 

      }
    
      if ( not  $strat_found_  )
      {
        push ( @not_found_results_ , $strat_name_ ) ;
      }
    }
  }

  if ( $#not_found_results_ >= 0 && $print_not_found_ eq "Y" )
  {
    $mail_body_ = $mail_body_." RESULTS NOT FOUND FOR THESE STRAT IN EC2 DIR on date $date_ for $prod_ \n\n ";
    foreach my $line_ ( @not_found_results_ ) 
    {
      $mail_body_ = $mail_body_." \n $line_ ";
    }
    $mail_body_ = $mail_body_."\n";
  }

  if ( $#not_matching_resuls_ >= 0 )
  {
    $mail_body_ = $mail_body_." RESULTS  ARE DIFFERENT FOR THESE STRAT ON NY AND EC2 on date $date_ for $prod_ \n" ;
    foreach my $line_ ( @not_matching_resuls_ ) 
    {
      $mail_body_ = $mail_body_."\n $line_ ";
    }
    $mail_body_ = $mail_body_."\n";
  }
}

if ( $mail_body_ )
{
  open ( MAIL , "|/usr/sbin/sendmail -t" ); 
  print MAIL "To: diwakar\@circulumvite.com, nseall@tworoads.co.in,ravi.parikh\@tworoads.co.in\n ";
  print MAIL "Subject:  NY and EC2 Globalresults comparision \n\n";
  print MAIL $mail_body_;
  close(MAIL);
}

