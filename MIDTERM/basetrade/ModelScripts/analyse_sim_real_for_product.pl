#! /usr/bin/perl
#\file : analyse_sim_real_for_product.pl
# for given product analyse the sim_real match  
#
#
#


use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME = "/spare/local/".$USER."/";
my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $QUERY_TRADE_DIR  = "/NAS1/logs/QueryTrades/";
my $QUERY_LOG_DIR = "/NAS1/logs/QueryLogs/";
my $TMP_DIR = "/tmp/";

sub GetProductIdOnDate;

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAn
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst

my $TRADE_DIFF_GENERATOR_ = $HOME_DIR."/generate_trades_file_diffs.pl";
my $SIM_REAL_DIFF = $SCRIPTS_DIR."/run_accurate_sim_real_and_create_tradediff.pl";
my $SIM_REAL_PLOT = $SCRIPTS_DIR."/run_accurate_sim_real.pl";
my $STATS_CALC_ = $SCRIPTS_DIR."/statcalcsinglevec.pl";

my $usage_ = "$0 SHORTCODE TRADING_DATE TIME_PERIOD";

if ( $#ARGV < 1 )
{
  print $usage_."\n";
  exit ( 0 );
}
my $product_pnl_score_thresh_ = 0.3;
my $product_vol_score_thresh_ = 0.08;
my $query_pnl_score_thresh_ = 0.25;
my $query_vol_score_thresh_ = 0.04;
my $yyyymmdd_ = $ARGV[1];
my $shortcode_ = $ARGV[0];
my $time_period_ = $ARGV[2] ;
my $sim_config_file_name_ = "INVALIDFILE";
my $mail_format_out_ = "";
if ( $#ARGV > 2 )
{
  $sim_config_file_name_ = $ARGV [ 3 ];
  if ($#ARGV > 3  && $ARGV[4] != "0" )
  {
    $mail_format_out_ ="1";
    if ( $#ARGV > 4 )
    {
      $product_pnl_score_thresh_ = $ARGV[5];
      if ( $#ARGV > 5 )
      {
        $query_pnl_score_thresh_ = $ARGV[6];
        if ( $#ARGV > 6 )
        {
          $product_vol_score_thresh_ = $ARGV[7];
          if ( $#ARGV > 7 )
          {
            $query_vol_score_thresh_ = $ARGV[8];
          }
        }
      }
    }
  }
}

my $trade_file_prefix_ = $QUERY_TRADE_DIR."/".substr ( $yyyymmdd_, 0, 4 )."/".substr ( $yyyymmdd_, 4, 2 )."/". substr ( $yyyymmdd_, 6, 2 )."/trades.".$yyyymmdd_.".";
my $log_file_prefix_ = $QUERY_LOG_DIR."/".substr ( $yyyymmdd_, 0, 4 )."/".substr ( $yyyymmdd_, 4, 2 )."/". substr ( $yyyymmdd_, 6, 2 )."/log.".$yyyymmdd_.".";

if ( index ( $ARGV[0], "TODAY" ) >= 0 )
{
  $yyyymmdd_ = `date +%y%m%d`; chomp($yyyymmdd_);
}
if ( index ( $time_period_, "AS_MORN") >= 0 ) 
{
  $time_period_ = "AS";
}
elsif ( index ( $time_period_, "EUS_") >= 0 )
{
  $time_period_ = "EUS";
}
elsif ( index ( $time_period_, "EU_") >= 0 )
{
  $time_period_ = "EU";
}
elsif ( index ( $time_period_, "US_") >= 0 )
{
  $time_period_ = "US";
}



my @prod_id_list_ = GetProductIdOnDate($shortcode_, $yyyymmdd_ , $time_period_ );
my $avg_pnl_score_ = 0.0;
my $avg_volume_score_= 0.0;
my $num_prod_strats_ = 0;

foreach my $prod_id_ ( @prod_id_list_ )
{
  print $prod_id_. " ". $yyyymmdd_." ====> \n";
  my $trade_diff_out_file_name_ = $TMP_DIR."temp_trade_diff.".$yyyymmdd_.".".$prod_id_;
  my $calculate_sim_real_diff_ = "$SIM_REAL_DIFF $yyyymmdd_ $prod_id_ $trade_diff_out_file_name_ $sim_config_file_name_";
  if ( $USER eq "dvctrader" || $USER eq "diwakar" )
  {
    print STDERR $calculate_sim_real_diff_."\n";
  }
  my $sim_pnl_ = "";
  my $sim_vol_= "";
  my $real_pnl_ = "";
  my $real_vol_ = "";
  my $max_abs_sim_pnl_ = "";
  my $max_abs_real_pnl_ = "";
  my @run_accurate_out_ = `$calculate_sim_real_diff_`; chomp( @run_accurate_out_ );
  
#`$SIM_REAL_PLOT $yyyymmdd_ $prod_id_ `;
  foreach my $line_  ( @run_accurate_out_)
   {
     if ( index ( $line_, "SIMRESULT") >= 0 )
     {
       print $line_."\n";
       my @sim_line_words_ = split (' ', $line_ );
       $sim_pnl_ = int ($sim_line_words_[1]);
       $sim_vol_ = int ($sim_line_words_[2]); 
     }
     if ( index ( $line_, "REALRESULT" ) >= 0 )
     {
       print $line_."\n";
       my @real_line_words_ = split (' ', $line_ );
       $real_pnl_ = int ($real_line_words_[1] );
       $real_vol_ = int ( $real_line_words_[2]);
     }
     if ( index ( $line_, "MAXABSREALPNL" ) >= 0 )
     {
       my @line_words_ = split ( ' ', $line_ );
       $max_abs_real_pnl_ = int ( $line_words_ [1] ) ;
     }
     if ( index ( $line_, "MAXABSSIMPNL") >= 0 )
     {
       my @line_words_ = split ( ' ', $line_ );
       $max_abs_real_pnl_ = int ( $line_words_[1] );
     }
   }

  my $max_abs_pnl_ = max ( abs ( $max_abs_real_pnl_ ) , abs ( $max_abs_sim_pnl_ ) ) ;
  my $max_end_pnl_ = max( abs ($real_pnl_), abs( $sim_pnl_) );
  my $max_end_vol_ = max ( abs ( $real_vol_ ), abs ( $sim_vol_) );
  my $pnl_score_ = 0.0;
  my $vol_score_ = 0.0;
  #now we have the diff trade
  my $stat_calc_cmd_ = $STATS_CALC_." -f ".$trade_diff_out_file_name_; 
  my @stat_calc_out_ = `$stat_calc_cmd_`; chomp($stat_calc_cmd_);

  #there will be 4 lines
  my $pnl_line_ = $stat_calc_out_[1];
  my @pnl_words_ = split(' ', $pnl_line_); 
#instead of doing the diff on complete diff file once, we can take the diffs in parts 
#and take the MEDIAN of the pnl/vol diff
#

  if ( ! $mail_format_out_ )
  {
    print " Diff Pnl Stats: ( Mean & Std ) ".$pnl_line_."\n";
    printf ( " Percentage pnl diff: (MeanPnlDiff/EndPnl) %0.3f \n", $pnl_words_[0]/$max_abs_pnl_);
  }

  $pnl_score_ = sprintf ("%0.3f", abs ($pnl_words_[1]/$max_abs_pnl_) );
  $avg_pnl_score_ = $avg_pnl_score_ + $pnl_score_;

  my $position_line_ = $stat_calc_out_[2];
  my @position_words_ = split ( ' ', $position_line_ );
   
  my $volume_line_ = $stat_calc_out_[3];
  my @volume_words_ = split ( ' ', $volume_line_ );
  if (! $mail_format_out_ )
  {
    print " Diff Vol Stats: ( Mean & Std ) ".$volume_line_."\n";
    printf ( " Percentage volume diff ( MeanVolDiff/total_vol ): %0.3f \n ", $volume_words_[0]/$max_end_vol_);
  }

  $vol_score_ = sprintf ( "%0.3f", abs ($volume_words_[1]/$max_end_vol_) ) ;
  $avg_volume_score_ = $avg_volume_score_ + $vol_score_;
  if ( $mail_format_out_ )
  {
    my $str_ = " PNL_SCORE : ";
    if ( $pnl_score_ > $query_pnl_score_thresh_)
    {
      $str_ = $str_."<FONT COLOR=\"RED\" >" . $pnl_score_;
    }
    else
    {
      $str_ = $str_."<FONT COLOR=\"BLUE\" >" . $pnl_score_;
    }

    $str_ = $str_." </FONT> VOLUME_SCORE: ";
    if ( $vol_score_ > $query_vol_score_thresh_)
    {
      $str_ = $str_."<FONT COLOR=\"RED\" >" . $vol_score_ ;
    }
    else
    {
      $str_ = $str_."<FONT COLOR=\"BLUE\" >" . $vol_score_ ;
    }
    $str_=$str_." </FONT> \n";
    print $str_;
  }
  else
  {
    print " PNL_SCORE : " . $pnl_score_." VOLUME_SCORE: " . $vol_score_."\n";
  }

  $num_prod_strats_++;
  for ( my $index_ = 0; $index_ <= $#stat_calc_out_ ; $index_++ )
   {
     if ($index_ == 0 ) 
     {
#time  
     }

   }
   foreach my $line  (@stat_calc_out_ )
   {

   }
  `rm $trade_diff_out_file_name_`;
  my $REAL_LOGFILE = $log_file_prefix_.$prod_id_.".gz";
  if ( ExistsWithSize ( $REAL_LOGFILE ) )
  {
#   my $strat_full_path_ = `less -f $REAL_LOGFILE | head -1`; chomp ( $strat_full_path_ ); 
#   my $strat_file_name_ = `basename $strat_full_path_`;chomp ( $strat_file_name_ );
#   my $cut_strat_file_name_ = `echo $strat_file_name_ | rev | cut -c \5- | rev`; chomp ( $cut_strat_file_name_ ) ;
#   my $sim_strat_file_name_ = `ls ~/modelling/strats/*/*/$cut_strat_file_name_ 2>/dev/null`; chomp ( $sim_strat_file_name_ );
#   if ( $sim_strat_file_name_ )
#   {
#     if (-e $sim_strat_file_name_ )
#     {

#wrong?? timeings can be different# late start, early end (max loss)
#     }
#   }   
  }  
}
if ( $num_prod_strats_ <= 0 )
{
  exit;
  }

if ( ! $mail_format_out_  )
{
  print "\n\n\nPNLVOL_AVG_SCORE: ".$avg_pnl_score_/$num_prod_strats_."    ".$avg_volume_score_/$num_prod_strats_."\n";
}
else
{
  $avg_pnl_score_ = sprintf ( "%0.3f", $avg_pnl_score_/$num_prod_strats_ );
  $avg_volume_score_ = sprintf ( "%0.3f", $avg_volume_score_/$num_prod_strats_ );
  my $str_ = "\n\n\nPNLVOL_AVG_SCORE: ";
  my $bad_sim_ = "";
  if ( $avg_pnl_score_ > $product_pnl_score_thresh_ )
  {
    $str_ = $str_."<FONT COLOR=\"RED\" >" . $avg_pnl_score_;
    $bad_sim_ = "true";
  }
  else
  {
    $str_ = $str_."<FONT COLOR=\"BLUE\" >" . $avg_pnl_score_;
  }
  $str_ = $str_."</FONT> ";
  if ( $avg_volume_score_ > $product_vol_score_thresh_ )
  {
    $str_ = $str_."<FONT COLOR=\"RED\" >" . $avg_volume_score_ ;
    $bad_sim_ = "true"
  }
  else
  {
    $str_ = $str_."<FONT COLOR=\"BLUE\" >" . $avg_volume_score_ ;
  }
  $str_ = $str_."</FONT> \n";
  if ( $bad_sim_ )
  {
  $str_ = $str_."\n<b> CHECK SIM-REAL BIAS  FOR THIS PRODUCT </b>\n"
  }
  print $str_;
}
exit;

sub GetProductIdOnDate
{
  my ( $shortcode_, $yyyymmdd_, $period_ ) = @_;
  my $query_start_id_ = "";
  my @query_id_list_ = ();

  if ($period_ eq "AS")
  {
  given ( $shortcode_ )
  {
    when( "HHI_0")
    {
      $query_start_id_ = "1000";
    }

    when ("HSI_0")
    {
      $query_start_id_ = "1100";
    }

    when ("MCH_0")
    {
      $query_start_id_ = "1200";
    }

    when ("MHI_0")
    {
      $query_start_id_ = "1300";
    }

    when ("NK_0")
    {
      $query_start_id_ = "2100";
    }

    when("NKM_0")
    {
      $query_start_id_ = "2200";
    }
    when ("TOPIX_0")
    {
      $query_start_id_= "2500";
    }
    when ("JGBL_0")
    {
      $query_start_id_ = "2400" ;
    }
    when ("NKMF_0")
    { 
      $query_start_id_ = "2300";
    }
  }
  }

  elsif ($period_ eq "EUS")
  {
    given($shortcode_)
    {
    when ("LFI_0")
    {
      $query_start_id_ = "900";
    }
    when ("LFI_1")
    {
      $query_start_id_ = "901";
    }
    when ("LFI_2")
    {
      $query_start_id_ = "902";
    }
    when ("LFI_3")
    {
      $query_start_id_ = "903";
    }
    when ("LFI_4")
    {
      $query_start_id_ = "904";
    }
    when ("LFI_5")
    {
      $query_start_id_ = "905";
    }
    when ("LFI_6")
    {
      $query_start_id_ = "906";
    }
    }
  }

  elsif ($period_ eq "US" )
  {
     given ( $shortcode_ )
     {
        when ( "FBTS_0" )
        {
          $query_start_id_ = "10";
        }
        when ("FGBS_0")
        {
          $query_start_id_ = "20";
        }
        when ( "FGBM_0" )
        {
          $query_start_id_ = "30";
        }
        when ( "FGBL_0")
        {
          $query_start_id_ = "40";
        }
        when ( "FGBX_0")
        {
          $query_start_id_ = "50";
        }
        when ( "FESX_0" )
        {
          $query_start_id_ = "60";
        }
        when ( "FOAT_0" )
        {
          $query_start_id_ = "90";
        }
        when ( "FDAX_0" ) 
        {
          $query_start_id_ = "70";
        }
        when ( "FBTP_0" )
        {
          $query_start_id_ = "80";
        }
        when ("ZN_0")
        {
          $query_start_id_ = "130";
        }
        when ("ZF_0")
        {
          $query_start_id_ = "120";
        }
        when ("ZB_0")
        {
          $query_start_id_ = "140";
        }
        when ("UB_0")
        {
          $query_start_id_ = "150";
        }
        when ("BR_DOL_0")
        {
          $query_start_id_ = "300";
        }
        when ("DI1F15")
        {
          $query_start_id_ = "350";
        }
        when ("DI1F16")
        {
          $query_start_id_ = "390";
        }
        when ("DI1F17")
        {
          $query_start_id_ = "380";
        }
        when ("DI1F18")
        {
          $query_start_id_ = "370";
        }
        when ("DI1F21")
        {
          $query_start_id_ = "360";
        }
        when("DI1N15")
        {
          $query_start_id_ = "308";
        }
        when ("DI1J15")
        {
          $query_start_id_ = "309";
        }
        when( "BR_IND_0")
        {
          $query_start_id_ = "310";
        }
        when ("BR_WIN_0")
        {
          $query_start_id_ = "320";
        }
        when ("LFR_0")
        {
          $query_start_id_ = "600";
        }
        when ("LFZ_0")
        {
          $query_start_id_ = "700";
        }
        when ("SXF_0" )
        {
	  $query_start_id_ = "200";
    	}
    	when ("CGB_0")
    	{
      	  $query_start_id_ = "210";
    	}
	when ("NKD_0")
	{
	  $query_start_id_ = "180";
	}
	when ("NIY_0")
	{
	  $query_start_id_ = "190";
	}
	when ("GE")
	{
	  $query_start_id_ = "160";
	}
	when ("ZT_0")
	{
	  $query_start_id_ = "110";
	}
	
     }
  }
  elsif ( $period_ eq "EU" )
  {
    given ( $shortcode_ )
    {
      when ( "FGBS_0")
      {
        $query_start_id_ = "3";
      }
      when ("FGBM_0")
      {
        $query_start_id_ = "1";
      }
      when ("FGBL_0")
      {
        $query_start_id_ = "4";
      }
      when ("FESX_0")
      {
        $query_start_id_ = "2";
      }
      when ("FDAX_0")
      {
        $query_start_id_ = "70";
      }
      when ("FBTP_0")
      {
        $query_start_id_ = "8";
      }
      when ("JFFCE_0")
      {
        $query_start_id_ = "400";
      }
      when ( "KFFTI_0" )
      {
        $query_start_id_ = "500";
      }
      when ("LFR_0")
      {
        $query_start_id_ = "12";
      }
      when ("LFZ_0")
      {
        $query_start_id_ = "13";
      }
    }
  }



#add for other products
#
    for ( my $i = $query_start_id_."00"; $i <= $query_start_id_."99"; $i++ )
    {
      if ( ExistsWithSize ( $trade_file_prefix_.$i ) )
      {
        push ( @query_id_list_, $i );
      }
    }
    return @query_id_list_;
}
