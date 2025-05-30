#!/usr/bin/perl

# It mails the pool_similarity statistics for all the pools 
# for shortcodes in /spare/local/tradeinfo/riskinfo/risk_stats_shclist
# Pool_similarity statistics: 20%,70%,90%,100% percentiles in the nC2 correlations series
#

use strict;
use warnings;
use feature "switch";          # for given, when
use File::Basename;            # for basename and dirname
use File::Copy;                # for copy
use List::Util qw/max min/;    # for max
use Math::Complex;             # sqrt
use FileHandle;
use POSIX;

my $USER     = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'};

my $REPO = "basetrade";

my $MODELING_BASE_DIR = $HOME_DIR . "/modelling";

my $SCRIPTS_DIR      = $HOME_DIR . "/" . $REPO . "_install/scripts";
my $GENPERLLIB_DIR   = $HOME_DIR . "/" . $REPO . "_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR . "/" . $REPO . "_install/ModelScripts";

my $BIN_DIR      = $HOME_DIR . "/" . $REPO . "_install/bin";
my $LIVE_BIN_DIR = $HOME_DIR . "/LiveExec/bin";

require "$GENPERLLIB_DIR/find_item_from_vec.pl";         # FindItemFromVec
require "$GENPERLLIB_DIR/print_stacktrace.pl";           # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_unique_list.pl";            # GetUniqueList
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl";    #MakeStratVecFromDir
require "$GENPERLLIB_DIR/array_ops.pl";                  # GetAverage
require "$GENPERLLIB_DIR/strat_utils.pl";                # IsStagedStrat
require "$GENPERLLIB_DIR/sample_data_utils.pl";          # FetchPnlSamplesStrats 
require "$GENPERLLIB_DIR/stratstory_db_access_manager.pl";          # FetchAllCorrelationForStrats 
require "$GENPERLLIB_DIR/performance_risk_helper.pl";

my $mail_address_ = "nseall@tworoads.co.in";

my $SHORTCODE_LIST = "/spare/local/tradeinfo/riskinfo/risk_stats_shclist";

my $current_date_ = `date +%Y%m%d`; chomp ( $current_date_ );

my $USAGE = "$0 SHORTCODE_LIST";
if ( @ARGV > 0 ) { $SHORTCODE_LIST = shift; }


my @quartiles_to_print_ = qw ( 0 25 70 90 100 );
my @metric_headers_ = ( "[  MEAN  ]", "[ MEDIAN ]" );
push( @metric_headers_, map "[ " . $_ . "%ILE ]", @quartiles_to_print_ );

my %shortcode2timeperiod2metrics_   = ();
my %shortcode2timeperiod2numstrats_ = ();
my %shortcode2timeperiod2numpairs_  = ();
my %exch2shcvec_ = ();
my %shc2exch_ = ();

open SHCLISTHANDLE, "< $SHORTCODE_LIST" or PrintStacktraceAndDie("Could not open file $SHORTCODE_LIST for reading..\n");
my @shc_vec_ = <SHCLISTHANDLE>;
chomp(@shc_vec_);
close SHCLISTHANDLE;

foreach my $shortcode_ (@shc_vec_) {
  my $texch_ = `$HOME_DIR/basetrade_install/bin/get_contract_specs $shortcode_ $current_date_ EXCHANGE | cut -d' ' -f2`;
  chomp( $texch_ );
  $texch_ = "UNDEF" if ! defined $texch_;
  push ( @{ $exch2shcvec_{ $texch_ } }, $shortcode_ );
  $shc2exch_{ $shortcode_ } = $texch_;

  my @pools_ = ( );
  if ( $timeperiod_ ne "-1" ) {
    push ( @pools_, $timeperiod_ );
  } else {
    my $pool_fetch_cmd_ = "$HOME_DIR/walkforward/get_pools_for_shortcode.py -shc $shortcode_";
    @pools_ = `$pool_fetch_cmd_ 2>/dev/null`; chomp ( $pool_fetch_cmd_ );
  }

  foreach my $timeperiod_ ( @pools_ ) {
    my $fetch_configs_cmd_ = "$HOME_DIR/walkforward/get_pool_configs.py -m POOL -shc $shortcode_ -tp $timeperiod_ -type N";
    my @strats_base_ = `$fetch_configs_cmd_ 2>/dev/null`; chomp ( @all_strats_ );

    my %similarity_map_ = ();
    FetchAllCorrelationForStrats ( \@strats_base_, \%similarity_map_ );

    my $num_similarity_pairs_ = keys %similarity_map_;

    if ( $num_similarity_pairs_ < 5 ) { next; }

    $shortcode2timeperiod2numstrats_{$shortcode_}{$timeperiod_} = @strats_base_;
    $shortcode2timeperiod2numpairs_{$shortcode_}{$timeperiod_}  = $num_similarity_pairs_;

    my @similarity_scores_ = values %similarity_map_;
    my $num_scores_        = @similarity_scores_;

    my $smedian_ = GetMedianAndSort( \@similarity_scores_ );
    my $smean_   = GetAverage( \@similarity_scores_ );

    my @metric_vec_ = ( $smean_, $smedian_ );
    foreach my $t_quantile_ (@quartiles_to_print_) {
      my $t_idx_ = int( $t_quantile_ / 100.0 * $num_scores_ );
      if ( $t_idx_ != 0 ) { $t_idx_ -= 1; }
      push( @metric_vec_, $similarity_scores_[$t_idx_] );
    }

    $shortcode2timeperiod2metrics_{$shortcode_}{$timeperiod_} = \@metric_vec_;
    #    print "Metric for Pool: ".$shortcode_.",".$timeperiod_.": ".join(" ", @metric_vec_)."\n";
  }
}

#PrintLargeShcPools (30);
DumpStatsInMailBody(0);
DumpStatsInMailBody(1);

sub PrintLargeShcPools {
  my $pool_size_limit_ = shift;

  my %shc_trader_map_ = ( );
  LoadProductToTraderMap ( \%exch2shcvec_, \%shc2exch_, \%shc_trader_map_ );

  my %trader_to_badpools_ = ( );
  foreach my $exch_ ( sort keys %exch2shcvec_ ) {
    foreach my $shc_ ( sort @{ $exch2shcvec_{ $exch_ } } ) {
      next if ! defined $shortcode2timeperiod2numstrats_{$shc_};
      next if ! defined $shc_trader_map_{$shc_};
      foreach my $timeperiod_ ( keys %{ $shortcode2timeperiod2numstrats_{$shc_} } ) {
        next if ( $shortcode2timeperiod2numstrats_{$shc_}{$timeperiod_} <= 30 );

        my $session_ = GetSession ( $timeperiod_ );
        my $trader_ = "UNDEF";
        $trader_ = $shc_trader_map_{$shc_}{ "ALL" } if defined $shc_trader_map_{$shc_}{ "ALL" };
        $trader_ = $shc_trader_map_{$shc_}{ $session_ } if defined $shc_trader_map_{$shc_}{ $session_ };

        push ( @{ $trader_to_badpools_{ $trader_ } }, [($shc_, $timeperiod_)] );
      }
    }
  }

  print "Traders BadPools:\n";
  foreach my $trader_ ( keys %trader_to_badpools_ ) {
    print $trader_."\n";
    print $$_[0]." ".$$_[1]." ".$shortcode2timeperiod2numstrats_{$$_[0]}{$$_[1]}."\n" foreach @{ $trader_to_badpools_{ $trader_ } }; 
    print "\n";
  }
}

sub DumpStatsInMailBody {
  my $sendmail_ = shift;

  my @col_headers_           = qw( SHORTCODE TIMEPERIOD NUM_STRATS );
  my $percentile_background_col_ = "#cbece8";
  my $meanmedian_backgroud_col_ = "#d2f6de";
  my $metric_col_            = "#00008b";
  my $bad_metric_col_        = "#8b0000";
  my $good_metric_col_       = "#006400";

  my $mailhandle_;
  if ( $sendmail_ ) {
    open ( $mailhandle_ , "|/usr/sbin/sendmail -t" );

    print $mailhandle_ "To: $mail_address_\n";
    print $mailhandle_ "From: $mail_address_\n";
    my $date_ = `date +%Y%m%d`; chomp($date_);
    printf $mailhandle_ "Subject: POOL SIMILARITY STATISTICS %s\n", $date_;
    print $mailhandle_ "X-Mailer: htmlmail 1.0\nMime-Version: 1.0\nContent-Type: text/html; charset=US-ASCII\n\n";
  }
  else {
    $mailhandle_ = *STDOUT;
  }

  print $mailhandle_ "<html><body>\n";
  print $mailhandle_ "<table border = \"1\"><tr>";

  foreach my $elem_str_ (@col_headers_) {
    printf $mailhandle_
      "<td align=center rowspan=\"2\"><font size = \"2\" color=darkblue>%s</font></td>",
      $elem_str_;
  }
  printf $mailhandle_
"<td align=center colspan=\"%d\"><font size = \"2\" color=darkblue>PNLSAMPLES_CORRELATIONS</font></td></tr>\n",
    scalar @metric_headers_;

  printf $mailhandle_ "<tr>";
  foreach my $elem_str_ (@metric_headers_) {
    printf $mailhandle_
"<td align=center padding-left=\"2px\" padding-right=\"2px\"><font size = \"2\" color=darkblue>%s</font></td>",
      $elem_str_;
  }
  printf $mailhandle_ "</tr>\n";

  foreach my $exch_ ( sort keys %exch2shcvec_ ) {
    foreach my $shc_ ( sort @{ $exch2shcvec_{ $exch_ } } ) {
      next if ! defined $shortcode2timeperiod2numstrats_{$shc_};
      foreach my $timeperiod_ ( keys %{ $shortcode2timeperiod2numstrats_{$shc_} } ) {
        if ( $shortcode2timeperiod2numstrats_{$shc_}{$timeperiod_} < 30 ) { printf $mailhandle_ "<tr>"; }
        else { printf $mailhandle_ "<tr style=\"font-weight:bold\">"; }

        printf $mailhandle_ "<td align=center>%s</td>", $shc_;
        printf $mailhandle_ "<td align=center>%s</td>", $timeperiod_;
        printf $mailhandle_ "<td align=center>%s</td>", $shortcode2timeperiod2numstrats_{$shc_}{$timeperiod_};
#printf $mailhandle_ "<td align=center>%s</td>", $shortcode2timeperiod2numpairs_{$shc_}{$timeperiod_};

        my @metric_vec_ = @{ $shortcode2timeperiod2metrics_{$shc_}{$timeperiod_} };

        foreach my $i ( 0..$#metric_headers_ ) {
          $metric_vec_[$i] = ( defined $metric_vec_[$i] ) ? sprintf("%.2f", $metric_vec_[$i] ) : "-";
        }
        my $mean_ = shift @metric_vec_;
        my $median_ = shift @metric_vec_;

        my $t_metric_col_ = $metric_col_;
        if ( $mean_ ne "-" && $median_ ne "-" ) {
          if ( $mean_ > 0.5 && $median_ > 0.5 ) { $t_metric_col_ = $bad_metric_col_; }
          if ( $mean_ < 0.3 && $median_ < 0.3 ) { $t_metric_col_ = $good_metric_col_; }
        }

        printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font color=\"%s\">%s</td>",
               $meanmedian_backgroud_col_, $t_metric_col_, $mean_;
        printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font color=\"%s\">%s</td>",
               $meanmedian_backgroud_col_, $t_metric_col_, $median_;

        foreach my $metric_elem_ ( @metric_vec_ ) { 
          printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font color=\"%s\">%s</td>",
                 $percentile_background_col_, $t_metric_col_, $metric_elem_;
        }
        printf $mailhandle_ "</tr>\n";
      }
    }
  }
  print $mailhandle_ "</table>";
  print $mailhandle_ "</body></html>\n";

  close($mailhandle_);
}
