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

use strict;
use warnings;
use FileHandle;

my $HOME_DIR = $ENV{'HOME'};
my $STRAT_DIR = $HOME_DIR."/modelling/strats/";

sub PrintMat
{
  my ( $mat ) = @_;
  print "                         ***************Strat 01*****************           ***************Strat 02*****************\n";
  print "                         PNL      VOL      Sharp    Improve  Drawdown       PNL      VOL      Sharp    Improve  Drawdown\n"; 
  for ( my $i = 1; $i <= $#$mat ; $i ++)
  {
    my $row_ref_ = $$mat[$i];
    for ( my $j = 0; $j <= $#$row_ref_ ; $j ++)
    {
      if($j==0)
      {
        printf "%-25s", $$row_ref_[$j];
      }
      else
      {
        if($j==5)
        {
          printf "%-15s", $$row_ref_[$j];
        }
        else
        {
          printf "%-9s", $$row_ref_[$j];
        }
      }
    }
    print "\n" ;
  }
}

sub SendMail
{
  my @col_headers_ = qw( SHORTCODE_GRP TIMEPERIOD );
  my ( $to_, $from_ ,$table_, $strat_ ) = @_;
  open ( MAIL , "|/usr/sbin/sendmail -t" );
  print MAIL "To: $to_\n";
  print MAIL "From: $from_\n";
  print MAIL "Subject: NSE top two strats \n";

  printf MAIL "X-Mailer: htmlmail 1.0\nMime-Version: 1.0\nContent-Type: text/html; charset=US-ASCII\n\n";
  printf MAIL "<html><body>\n";
  printf MAIL "<table border = \"2\">" ;
  printf MAIL "\n";
  for( my $k = 0; $k<=$#$table_; $k ++ )
  {
    printf MAIL "<tr>\n";
    my $row_ = $$table_[$k];
    for ( my $j = 0; $j <= $#$row_ ; $j ++)
    {
      if($k == 0)
      {
        printf MAIL "<td align=\"center\"><font font-weight = \"bold\" size = \"4\" color=\"Red\">%-20s</font></td>\n", $$row_[$j];
      }
      else
      {
        printf MAIL "<td align=\"center\"><font font-weight = \"bold\" size = \"3\" color=\"darkblue\">%-20s</font></td>\n", $$row_[$j];
      }
    }
    printf MAIL "</tr>\n";
  }
  printf MAIL "</table>\n";

  printf MAIL "<p></p>";
  printf MAIL "<p>Strat Table :</p>";
  printf MAIL "<table border = \"2\">" ;
  printf MAIL "\n";
  for( my $k = 0; $k<=$#$strat_; $k ++ )
  {
    printf MAIL "<tr>\n";
    my $row_ = $$strat_[$k];
    for ( my $j = 0; $j <= $#$row_ ; $j ++)
    {
      if($j == 0)
      {
        printf MAIL "<td align=\"center\"><font font-weight = \"bold\" size = \"4\" color=\"Red\">%-20s</font></td>\n", $$row_[$j];
      }
      else
      { 
        printf MAIL "<td align=\"center\"><font font-weight = \"bold\" size = \"3\" color=\"darkblue\">%-20s</font></td>\n", $$row_[$j];
      }
    }
    printf MAIL "</tr>\n";
  }
  printf MAIL "</table>\n";
  printf MAIL "</body></html>\n";
  
  close(MAIL);
}

my $product_list_ = `ls $STRAT_DIR | grep _FUT0`;
my @product_array_ = split '\n', $product_list_;

my @mail_table_ = ();
my @strat_ = ();
my @header_ = ( "Product", "__PNL1__", "__Vol1__", "__Sharp1__", "_PNLDD_", "_Drawdown_", "___",  "__PNL2__", "__Vol2__", "__Sharp2__", "_PNLDD_", "_Drawdown_");
push( @mail_table_, \@header_ );

for( my $i = 0; $i <= $#product_array_; $i ++ )
{ 
  my $location_ = $STRAT_DIR.$product_array_[$i]."/";
  my $start_date_ = `date "+%Y%m%d"`;
  chomp($start_date_);
  my $end_date_cmd_ = $HOME_DIR."/basetrade_install/bin/calc_prev_week_day ".$start_date_." "."11";
  my $end_date_ = `$end_date_cmd_`;
  chomp($end_date_);
 
  my $exec_cmd_ = $HOME_DIR."/LiveExec/bin/summarize_strategy_results"." ".$product_array_[$i]." ".$location_." DB ".$end_date_." ".$start_date_." INVALIDFILE kCNAPnlSharpeAverage 0 INVALIDFILE 0 |grep -e \"STRATEGY\" -e \"STATISTICS\" | head -4";
  my $top_two_ =  `$exec_cmd_`;

  my $lot_size_ = `~/basetrade_install/bin/get_contract_specs $product_array_[$i] $start_date_ LOTSIZE | grep $product_array_[$i] | awk '{ print \$2 }'`; chomp($lot_size_);
  my @row_ = ();
  push( @row_, $product_array_[$i] );
  my @arr_ = split ( '\n', $top_two_ );
  for( my $j = 1; $j <= $#arr_; $j +=2 )
  {
    my @arr1_ = split ( ' ', $arr_[$j] );
    my $vol_ = $arr1_[3];
    if($lot_size_ =~ /^\d+?$/)
    {
     $vol_ = $arr1_[3]/$lot_size_;
    }
    push( @row_, $arr1_[1] );
    push( @row_, $vol_ );
    push( @row_, $arr1_[4] );
    push( @row_, $arr1_[21] );
    push( @row_, $arr1_[22] );
    if( $j < $#arr_ )
    {
      push( @row_, "___" );
    }
  }

  my @strat_row_ = ();
  push( @strat_row_, $product_array_[$i] );
  for( my $j = 0; $j <= $#arr_; $j +=2 )
  {
    my @strategy_name_ = split ( ' ', $arr_[$j] );
    push( @strat_row_, $strategy_name_[1] );
  }

  if( $#row_ >0 )
  {
    if( $row_[1] > 0 )
    {
      push( @mail_table_, \@row_ );
      push( @strat_, \@strat_row_ );
    }
  }
}

my $to_ = "nseall@tworoads.co.in ";
my $from_ = "nseall@tworoads.co.in ";
SendMail( $to_, $from_, \@mail_table_, \@strat_);
#PrintMat( \@mail_table_ );
