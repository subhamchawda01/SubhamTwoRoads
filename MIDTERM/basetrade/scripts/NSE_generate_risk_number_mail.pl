#!/usr/bin/perl
#
# \file ModelScripts/find_best_model_for_strategy_var_pert.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

my @products_ = (
    "ADANIPORTS_FUT0",
    "AMBUJACEM_FUT0",
    "ASHOKLEY_FUT0",
    "ASIANPAINT_FUT0",
    "AUROPHARMA_FUT0",
    "AXISBANK_FUT0",
    "BANKBARODA_FUT0",
    "BANKNIFTY_FUT0",
    "BHARTIARTL_FUT0",
    "BHEL_FUT0",
    "BPCL_FUT0",
    "CANBK_FUT0",
    "CIPLA_FUT0",
    "COALINDIA_FUT0",
    "DLF_FUT0",
    "GIND10YR_FUT0",
    "HCLTECH_FUT0",
    "HDFCBANK_FUT0",
    "HDFC_FUT0",
    "HINDPETRO_FUT0",
    "HINDUNILVR_FUT0",
    "ICICIBANK_FUT0",
    "INDUSINDBK_FUT0",
    "INFY_FUT0",
    "ITC_FUT0",
    "KOTAKBANK_FUT0",
    "LT_FUT0",
    "LUPIN_FUT0",
    "MARUTI_FUT0",
    "MOTHERSUMI_FUT0",
    "NIFTY_FUT0",
    "ONGC_FUT0",
    "PNB_FUT0",
    "RCOM_FUT0",
    "RELCAPITAL_FUT0",
    "RELIANCE_FUT0",
    "RELINFRA_FUT0",
    "SBIN_FUT0",
    "SUNTV_FUT0",
    "TATAMOTORS_FUT0",
    "TCS_FUT0",
    "TECHM_FUT0",
    "USDINR_FUT0",
    "WOCKPHARMA_FUT0",
    "YESBANK_FUT0",
    "ZEEL_FUT0"
    );

sub GetAvg
{
  my ($shortcode_, $num_prev_days_) = @_;
  my $exec_cmd_  = "grep \" $shortcode_\" /NAS1/data/MFGlobalTrades/EODPnl/* | tail -$num_prev_days_";
  my $content_ = `$exec_cmd_`;
#print $content_;

  my $exec_cmd1_ = "grep \" $shortcode_\" /NAS1/data/MFGlobalTrades/EODPnl/* | tail -$num_prev_days_ | awk '{print \$6}'";
  my $content1_ = `$exec_cmd1_`;
  my @pnls_ = split ( '\n', $content1_);

  my $average_ = 0;
  for( my $j = 0; $j <= $#pnls_; $j++ )
  { 
    $average_ += $pnls_[$j];
  }
  if( $#pnls_+1 > 0)
  {
    $average_ = $average_/($#pnls_+1);
  }
  else 
  {
    $average_ = 0;
  }
  return ($average_, ($#pnls_+1), $content_);
}

sub PrintMat
{
  my ( $mat ) = @_;
  for ( my $i = 0; $i <= $#$mat ; $i ++)
  {
    my $row_ref_ = $$mat[$i];
    for ( my $j = 0; $j <= $#$row_ref_ ; $j ++)
    {
      print "$$row_ref_[$j]\t";
    }
    print "\n" ;
  }
}

my @num_days_ = ( 1,3,5,10);
my @mail_table_ = ();
my @header_ = ( "Product", "_1 DAY AVG_", "_3 DAY AVG_", "_5 DAY AVG_", "_10 DAY AVG_" );
push( @mail_table_, \@header_ );
my @details_ = ();

for ( my $i = 0; $i <= $#products_ ; $i ++)
{
  my @row_ = ();
  push( @row_, $products_[$i] );
  for ( my $j = 0; $j <= $#num_days_ ; $j ++)
  {
    my ($temp_, $temp_len_, $content_) = GetAvg( $products_[$i], $num_days_[$j] );
    if($j == $#num_days_)
    {
      my @content_array_ = split ( '\n', $content_);
      push( @details_, \@content_array_ );
    }
    if( $temp_len_ == $num_days_[$j] )
    {
      push( @row_, int($temp_ + 0.5) );
    }
    else
    {
      push( @row_, "***" );
    }
  }
  push( @mail_table_, \@row_ );
}

sub PrintArray
{
  my ($array_) = @_;
  for( my $i = 0; $i<=$#$array_; $i++ )
  {
      print "$$array_[$i] \n";
  }
  print "\n";
}


sub SendMail
{
  my @col_headers_ = qw( SHORTCODE_GRP TIMEPERIOD );
  my ( $to_, $from_ ,$table_, $text_ ) = @_;
  open ( MAIL , "|/usr/sbin/sendmail -t" );
  print MAIL "To: $to_\n";
  print MAIL "From: $from_\n";
  print MAIL "Subject: NSE risk numbers \n";

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

#printf MAIL "<TD style=\"FONT-SIZE:13px; COLOR:#000000; LINE-HEIGHT:0px; FONT-FAMILY:Arial,Helvetica,sans-serif\">"; 
  printf MAIL "<TD style=\"FONT-SIZE:15px; COLOR:#000000; LINE-HEIGHT:0px; FONT-FAMILY:Times New Roman\">"; 
  for( my $k = 0; $k<=$#$text_; $k ++ )
  {
    my $row_ = $$text_[$k];
    for ( my $j = 0; $j <= $#$row_ ; $j ++)
    {
      printf MAIL "<h4>$$row_[$j]</h4>";
    }
    printf MAIL "<p>________________________________________________________________________________________________________________</p>";
    printf MAIL "<p>\n</p>";
  }
  printf MAIL "</body></html>\n";
  close(MAIL);
}
#my $to_ = "vinit.darda\@tworoads.co.in";
#my $from_ = "vinit.darda\@tworoads.co.in";
my $to_ = "nseall@tworoads.co.in";
my $from_ = "nseall@tworoads.co.in";
SendMail( $to_, $from_, \@mail_table_, \@details_);

#PrintMat( \@mail_table_);

#PrintArray( \@details_ );



