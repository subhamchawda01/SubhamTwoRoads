#!/usr/bin/perl

# \file ModelScripts/run_product_on_event.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes an instructionfilename :
# [ CONFIG_FILE ]
# Working_dir
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

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $AFLASHSCRIPTS_DIR = $HOME_DIR."/".$REPO."/AflashScripts";
my $AFLASH_TRADEINFO_DIR = "/spare/local/tradeinfo/Alphaflash";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr
require "$GENPERLLIB_DIR/get_unix_time_from_utc.pl"; # GetUnixtimeFromUTC


sub LoadConfigFile;
sub GenerateEventData;
sub FindProductBetas;
sub MakeStrategies;
sub RunSimulations;
sub DumpResults;

# start 
my $USAGE="$0 config-file working-dir";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit( 0 ); }

my $config_file_ = $ARGV[0];
my $working_dir_ = $ARGV[1];

my @shortcode_list_ = ( );
my @event_names_ = ( );
my $event_id_;
my $event_time_ = "";
my %event_name_to_datumid_ = ( );
my %event_name_to_evid_ = ( );
my %event_name_to_scale_ = ( );
my %shc_to_working_dir_ = ( );
my $aggressive_threshold_ = 0;
my $skip_dates_file_;
my $getflat_scale_ = 1;
my @datumid_vec_ = ( );

my %revised_event_name_to_datumid_ = ( );
my %revised_event_name_to_scale_ = ( );

my %shc_dur_to_beta_ = ( );
my %shc_dur_to_adj_rsq_ = ( );
my %shc_dur_to_corr_ = ( );
my %shc_dur_to_median_ = ( );
my %shc_dur_to_75percentile_ = ( );
my %shc_dur_to_stratfile_ = ( );
my %shc_dur_to_agg_threshold_ = ( );

my %shc_to_uts_ = ( );
my %shc_to_mur_ = ( );

my $uts_mur_list_ = $AFLASHSCRIPTS_DIR."/utslist";
my $event_map_fxs_ = $AFLASH_TRADEINFO_DIR."/event_fxstreet_map";
my $event_map_bbg_ = $AFLASH_TRADEINFO_DIR."/event_bbg_map";

my $evdat_file_ = $working_dir_."/events.dat";
my $utslist_ = $working_dir_."/utslist_t";

my @durs_ = (2,5,10);
my @sim_durs_ = (2,5,10,"dd");

my $use_fxsbbg_ = "FXS";

my $prep_strats_ = 0;
my $run_sim_ = 0;
my $compute_p2y_ = 0;
my $event_mean_ = 0;
my $datum_id_ = 2;

LoadConfigFile ( $config_file_ );

GenerateEventData ( $evdat_file_ );

#GenerateEstimates ( );

MakeUTSList ( $utslist_ );

FindProductBetas ( );

if ( $prep_strats_ ) {
  PrepStrats ( );
}

if ( $run_sim_ ) {
  RunStrats ( );
}

sub LoadConfigFile
{
  print "Loading Config File ...\n";

  my $t_config_file_ = shift;

  open ( CONFIG_FILE , "<" , $t_config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $t_config_file_" );
  my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
  close ( CONFIG_FILE );

  my $current_param_ = "";
  foreach my $cline_ ( @config_file_lines_ )
  {
    if ( $cline_ =~ /^#/ ) {  # not ignoring lines with # not at the beginning
      next;
    }
    my @t_words_ = split ( ' ' , $cline_ );

    if ( $#t_words_ < 0 ) {
      $current_param_ = "";
      next;
    }

    if ( ! $current_param_ ) {
      $current_param_ = $t_words_ [ 0 ];
      next;
    }
    else {
      given ( $current_param_ ) {
        when ( "SHORTCODE_LIST" ) {
          push ( @shortcode_list_, $t_words_[0] );
        }
        when ( "EVENT_NAMES" ) {
          $cline_ =~ s/^\s+|\s+$//g;
          push ( @event_names_, $cline_ );
        }
        when ( "EVENT_ID" ) {
          $event_id_ = $t_words_[0];
        }
        when ( "EVENT_TIME" ) {
          $event_time_ = $t_words_[0];
        }
        when ( "DATUM_ID" ) {
          push ( @datumid_vec_, $t_words_[0] );
        }
        when ( "UTS_MUR_LIST" ) {
          $uts_mur_list_ = $t_words_[0];
        }
        when ( "AGGRESSIVE_PERCETILE" ) {
          $aggressive_threshold_ = $t_words_[0];
        }
        when ( "USE_BBG" ) {
          if ( $t_words_[0] ) {
            $use_fxsbbg_ = "BBG";
          }
        }
        when ( "RUN_SIM" ) {
          if ( $t_words_[0] ) {
            $run_sim_ = 1;
            $prep_strats_ = 1;
          } else {
            $run_sim_ = 0;
          }
        }
        when ( "PREP_STRATS" ) {
          if ( $t_words_[0] ) {
            $prep_strats_ = 1;
          } else {
            $prep_strats_ = 0;
            $run_sim_ = 0;
          }
        }
        when ( "SKIP_DATES_FILE" ) {
          $skip_dates_file_ = $t_words_[0];
        }
        when ( "GETFLAT_SCALE" ) {
          $getflat_scale_ = $t_words_[0];
        }
      }
    }
  }

  LoadEventMap (  );

  if ( ! defined $event_id_ && scalar @event_names_ == 0 ) {
    PrintStacktraceAndDie ( "Neither EVENT_NAMES nor EVENT_ID has been mentioned in the config" );
  }

  if ( ! defined $event_id_ ) {
    $event_id_ = GetEventIdFromEventName( $event_names_[0] );
    if ( ! defined $event_id_ ) {
      PrintStacktraceAndDie ( "No Event Id corresponding to ".$event_names_[0]." exists in ".$event_map_fxs_ );
    }
  }

  foreach my $event_name_ ( @event_names_ ) {
    my $t_event_id_ = GetEventIdFromEventName( $event_name_ );

    if ( ! defined $t_event_id_ ) {
      PrintStacktraceAndDie ( "No Event Id corresponding to ".$event_name_." exists in ".$event_map_fxs_ );
    }

    if ( $t_event_id_ != $event_id_ ) {
      PrintStacktraceAndDie ( $event_names_[0]." and ".$event_name_." do not have the same event_id_\n" );
    }

    $event_name_to_datumid_{ $event_id_ } = GetDatumIdFromEventName( $event_name_ );
    if ( ! defined $event_name_to_datumid_{ $event_id_ } ) {
      PrintStacktraceAndDie ( "No Datum Id corresponding to ".$event_name_." exists in ".$event_map_fxs_ );
    }
  }

  if ( ! -d $working_dir_ ) {
    `mkdir -p $working_dir_`;
  }

  foreach my $shc_ ( @shortcode_list_ ) {
    $shc_to_working_dir_{ $shc_ } = $working_dir_."/".$shc_;
    if ( ! -d $shc_to_working_dir_{ $shc_ } ) {
      `mkdir -p $shc_to_working_dir_{$shc_}`;
    }
  }
}

sub LoadEventMap
{
  if ( $use_fxsbbg_ eq "FXS" ) {
    LoadEventMapFXS ( );
  }
  elsif ( $use_fxsbbg_ eq "BBG" ) {
    LoadEventMapBBG ( );
  }
}

sub LoadEventMapFXS
{
  open FXSMAPHANDLE, "< $event_map_fxs_" or PrintStacktraceAndDie ( "Could not open $event_map_fxs_ for reading" );
  my @fxsmap_lines_ = <FXSMAPHANDLE>; chomp ( @fxsmap_lines_ );
  close FXSMAPHANDLE;

  foreach my $tline_ ( @fxsmap_lines_ ) {
    my @twords_ = split(',', $tline_);
    
    if ( $#twords_ > 0 ) {
      my $evid_ = $twords_[0];
      foreach my $tword_ ( @twords_[1..$#twords_] ) {
        my @tt_words_ = split(':', $tword_);
        if ( $#tt_words_ >= 1 ) {
          my $evname_ = $tt_words_[1];
          my @evname_words_ = split("|", $evname_);
          if ( $#evname_words_ > 0 && $evname_words_[1] eq "Revised" ) {
            $revised_event_name_to_datumid_{ $evname_words_[0] } = $tt_words_[0];
            if ( $#tt_words_ >= 2 ) {
              $revised_event_name_to_scale_{ $evname_words_[0] } = $tt_words_[2];
            } else {
              $revised_event_name_to_scale_{ $evname_words_[0] } = 1;
            }
          }
          else {
            $event_name_to_datumid_{ $evname_ } = $tt_words_[0];
            $event_name_to_evid_{ $evname_ } = $evid_;
            if ( $#tt_words_ >= 2 ) {
              $event_name_to_scale_{ $evname_ } = $tt_words_[2];
            } else {
              $event_name_to_scale_{ $evname_ } = 1;
            }
          }
        }
      }
    }
  }
}

sub LoadEventMapBBG
{
  open FXSMAPHANDLE, "< $event_map_bbg_" or PrintStacktraceAndDie ( "Could not open $event_map_bbg_ for reading" );
  my @fxsmap_lines_ = <FXSMAPHANDLE>; chomp ( @fxsmap_lines_ );
  close FXSMAPHANDLE;

  foreach my $tline_ ( @fxsmap_lines_ ) {
    my @twords_ = split(',', $tline_);
    
    if ( $#twords_ > 0 ) {
      my $evid_ = $twords_[0];
      foreach my $tword_ ( @twords_[1..$#twords_] ) {
        my @tt_words_ = split(':', $tword_);
        if ( $#tt_words_ >= 1 ) {
          my $evname_ = $tt_words_[1];
          my @evname_words_ = split(" ", $evname_);
          if ( $#evname_words_ > 0 && $evname_words_[1] eq "Revised" ) {
            $revised_event_name_to_datumid_{ $evname_words_[0] } = $tt_words_[0];
            if ( $#tt_words_ >= 2 ) {
              $revised_event_name_to_scale_{ $evname_words_[0] } = $tt_words_[2];
            } else {
              $revised_event_name_to_scale_{ $evname_words_[0] } = 1;
            }
          }
          else {
            $event_name_to_datumid_{ $evname_ } = $tt_words_[0];
            $event_name_to_evid_{ $evname_ } = $evid_;
            if ( $#tt_words_ >= 2 ) {
              $event_name_to_scale_{ $evname_ } = $tt_words_[2];
            } else {
              $event_name_to_scale_{ $evname_ } = 1;
            }
          }
        }
      }
    }
  }
}

sub GetEventIdFromEventName
{
  my $evname_ = shift;
  return $event_name_to_evid_{ $evname_ };
}

sub GetDatumIdFromEventName
{
  my $evname_ = shift;
  return $event_name_to_datumid_{ $evname_ };
}

sub GenerateEventData
{
  my $evdat_file_ = shift;
  my $db_ = $use_fxsbbg_;

  my $evdat_file_tmp_ = $evdat_file_.".tmp";

  my $exec_cmd_ = "";
  if ( $event_names_[0] eq "EIA Crude Oil Stocks change" ) {
    $exec_cmd_ = $AFLASHSCRIPTS_DIR."/generate_event_dat_crude.R $evdat_file_tmp_ $event_time_";
  } else {
    $exec_cmd_ = $AFLASHSCRIPTS_DIR."/generate_event_dat.R $db_ $evdat_file_tmp_ $event_time_ ".join(" ", map { "\"$_\"" } @event_names_);
  }
  print $exec_cmd_."\n";
  `$exec_cmd_`;

  my @evlines_ = `tail -n+2 $evdat_file_tmp_`; chomp ( @evlines_ );
  my @dates_ = map { (split(',', $_))[0] } @evlines_;
  my %date_to_time_ = map { (split(',', $_))[0] => (split(',', $_))[1] } @evlines_;
  @dates_ = grep { $_ > 20150801 } @dates_;

  my @skip_dates_ = ( );
  if ( defined $skip_dates_file_ ) {
    my @skip_dates_ = `cat $skip_dates_file_`; chomp ( @skip_dates_ );
  }

  my %date_to_latency_ = ( );
  my %date_to_yield_ = ( );
  my %date_to_bidcover_ = ( );
  my %date_to_primary_dealer_ = ( );
  my %date_to_indirect_bidder_ = ( );
  foreach my $date_ ( @dates_ ) {
    my $yy_ = substr $date_, 0, 4;
    my $mm_ = substr $date_, 4, 2;
    my $dd_ = substr $date_, 6, 2;

    my $mdsfile_ = "/NAS1/data/AFLASHLoggedData/NY4/$yy_/$mm_/$dd_/AFL_$date_.gz";
    $mdsfile_ = `ls /media/shared/ephemeral*/s3_cache/$mdsfile_ | head -1`; chomp ( $mdsfile_ );

    my @values = `~/infracore_install/bin/mds_log_reader AFLASH $mdsfile_ 2>/dev/null | grep -A6 -B2 "Category: $event_id_" | awk '{print \$1, \$NF}'`;
    chomp ( @values );
    if ( $#values < 0 ) {
      push ( @skip_dates_, $date_ );
      next;
    }

    print $mdsfile_."\n".join("\n",@values)."\n";
    my @ev_values_ = grep { $_ =~ /Field/ } @values;
    @ev_values_ = map { (split(' ', $_))[-1] } @ev_values_;
    print join(" ", @ev_values_)."\n";

    my $time_ = (grep { $_ =~ /Time/ } @values )[0];
    $time_ = (split(' ', $time_))[-1];

    $date_to_latency_{ $date_ } = $time_ - GetUnixtimeFromUTC( $date_, $date_to_time_{ $date_ } );

    $date_to_yield_{ $date_ } = $ev_values_[0];
    $date_to_bidcover_{ $date_ } = $ev_values_[1];
    $date_to_primary_dealer_{ $date_ } = $ev_values_[2];
    $date_to_indirect_bidder_{ $date_ } = $ev_values_[3];
  }

  my %skip_dates_map_ = map { $_ => 1 } @skip_dates_;
  @dates_ = grep { ! defined $skip_dates_map_{ $_ } } @dates_;

  open DHANDLE, "> $evdat_file_" or PrintStacktraceAndDie ( "Could not open $evdat_file_ for writing" );

  print DHANDLE "Date,Time,Latency,HighYield,BidtoCoverRatio,Primary_Dealer,Indirect_Bidder\n"; 
  foreach ( @dates_ ) {
    print DHANDLE $_.",".$date_to_time_{$_}.",".$date_to_latency_{$_}.",".$date_to_yield_{$_}.",".$date_to_bidcover_{$_}.",".$date_to_primary_dealer_{$_}.",".$date_to_indirect_bidder_{$_}."\n";
  }
  close DHANDLE;
}

sub FindProductBetas
{
  my @time_tokens_ = split('_', $event_time_);
  if ( $#time_tokens_ > 1 ) {
    PrintStacktraceAndDie ( "The event time $event_time_ has incorrect format" );
  }

  my $tz_ = "";
  my $numeric_time_ = $time_tokens_[0];
  if ( $#time_tokens_ > 0 ) {
    $tz_ = $time_tokens_[0]."_";
    $numeric_time_ = $time_tokens_[1];
  }

  my $time_minutes_ = int($numeric_time_/100)*60 + $numeric_time_%100;
  my $time_min_bef_ = $time_minutes_ - 10;
  my $time_min_aft_ = $time_minutes_ + 30;
  my $time_bef_ = $tz_.( int($time_min_bef_/60)*100 + $time_min_bef_%60 );
  my $time_aft_ = $tz_.( int($time_min_aft_/60)*100 + $time_min_aft_%60 );

  my %shc_to_pxchange_ = ( );
  foreach my $shc_ ( @shortcode_list_ ) {
    my $gendatg_cmd_ = $HOME_DIR."/us_auction/findpxch.sh $shc_ $evdat_file_ $event_time_ $time_bef_  $time_aft_ ".$working_dir_." 2 5 10 20";
    print $gendatg_cmd_."\n";
#`$gendatg_cmd_ 1>/dev/null 2>&1`;

    $shc_to_pxchange_{ $shc_ } = $shc_to_working_dir_{ $shc_ }."/pxchange.dat";

    if ( ! exists $shc_to_pxchange_{ $shc_ } ) {
     PrintStacktraceAndDie ( $shc_to_pxchange_{ $shc_ }." could not be generated" );
    }

   if ( $compute_p2y_ && $shc_ eq "ZN_0" ) {
     my $p2y_file_ = "/spare/local/tradeinfo/p2y/ZN_p2y";
     my $ticksize_ = "0.015625";

     if ( -f $p2y_file_ ) {
       my @yield_lines_ = `tail -n+2 $p2y_file_`; chomp ( @yield_lines_ );
       my %p2y_ = map { ((split(' ',$_))[0] / $ticksize_) => (split(' ',$_))[1] } @yield_lines_;

       my @pxch_lines_ = `tail -n+2 $shc_to_pxchange_{ $shc_ }`; chomp ( @pxch_lines_ );
       my %date2price_ = map { (split(',',$_))[0] => (split(',',$_))[-1] } @pxch_lines_;

       open DHANDLE, "< $evdat_file_" or PrintStacktraceAndDie ( "Could not open $evdat_file_ for reading" );
       my @dlines_ = <DHANDLE>; chomp ( @dlines_ );
       close DHANDLE;

       open DHANDLE, "> $evdat_file_" or PrintStacktraceAndDie ( "Could not open $evdat_file_ for writing" );
       foreach my $line_ ( @dlines_ ) {
         my $date_ = (split(",", $line_))[0];
         if ( $date_ eq "Date" ) { 
           $line_ .= ",ZNYield";
         }
         else {
           $line_ .= ",";
           if ( defined $date2price_{ $date_ } ) {
             my $price_ = int(0.5 + ($date2price_{ $date_ } / $ticksize_));
             if ( defined $p2y_{ $price_ } ) {
               $line_ .= $p2y_{ $price_ };
             }
           }
         }
         print DHANDLE $line_."\n";
       }
       close DHANDLE;
     }
   }
  }

  my @evid_list_ = @datumid_vec_;
  my @evscale_list_ = map { 1 } @evid_list_;

  my @percentiles_ = (0.75, $aggressive_threshold_);
  my $zn_findscale_cmd_ = "/home/hagarwal/us_auction/findScale.R $evdat_file_ ZN_0 ".$shc_to_pxchange_{ "ZN_0" }." ".join(",", @percentiles_);
  print $zn_findscale_cmd_."\n";
  $event_mean_ = `$zn_findscale_cmd_ 2>/dev/null | grep EventMean | awk '{print \$2;}'`; chomp ( $event_mean_ );

  foreach my $shc_ ( @shortcode_list_ ) {
    my $findscale_cmd_ = "/home/hagarwal/us_auction/findScale.R $evdat_file_ $shc_ ".$shc_to_pxchange_{ $shc_ }." ".join(",", @percentiles_)." $event_mean_";
    print $findscale_cmd_."\n";
    my @fscale_outs_ = `$findscale_cmd_ 2>/dev/null`; chomp ( @fscale_outs_ );

    print "$shc_\n".join("\n", @fscale_outs_)."\n\n";

    foreach my $fline_ ( @fscale_outs_ ) {
      my @fwords_ = split /\s+/, $fline_;

      given ( $fwords_[0] ) {
        when ( "beta2:" ) {
          my @beta_list_ = map { $fwords_[$_] * 1.0 / $evscale_list_[$_-1] } 1..($#fwords_-1);
          $shc_dur_to_beta_{ $shc_ }{ 2 } = [ map { $evid_list_[$_].":".$beta_list_[$_] } 0..$#beta_list_ ];
          $shc_dur_to_adj_rsq_{ $shc_ }{ 2 }= $fwords_[$#fwords_];
        }
        when ( "beta5:" ) {
          my @beta_list_ = map { $fwords_[$_] * 1.0 / $evscale_list_[$_-1] } 1..($#fwords_-1);
          $shc_dur_to_beta_{ $shc_ }{ 5 } = [ map { $evid_list_[$_].":".$beta_list_[$_] } 0..$#beta_list_ ];
          $shc_dur_to_adj_rsq_{ $shc_ }{ 5 }= $fwords_[$#fwords_];
        }
        when ( "beta10:" ) {
          my @beta_list_ = map { $fwords_[$_] * 1.0 / $evscale_list_[$_-1] } 1..($#fwords_-1);
          $shc_dur_to_beta_{ $shc_ }{ 10 } = [ map { $evid_list_[$_].":".$beta_list_[$_] } 0..$#beta_list_ ];
          $shc_dur_to_adj_rsq_{ $shc_ }{ 10 }= $fwords_[$#fwords_];
        }
        when ( "lm_corr:" ) {
          foreach my $dur_idx_ ( 0..$#durs_ ) {
            $shc_dur_to_corr_{ $shc_ }{ $durs_[ $dur_idx_ ] } = $fwords_[ $dur_idx_+1 ];
          }
        }
        when ( /^percentile/ ) {
          my $tpercentile_ = $fwords_[0];
          $tpercentile_ =~ s/percentile//;
          if ( $tpercentile_ == 0.75 ) {
            foreach my $dur_idx_ ( 0..$#durs_ ) {
              if ( ! defined $shc_dur_to_75percentile_{ $shc_ }{ $durs_[ $dur_idx_ ] } ) {
                $shc_dur_to_75percentile_{ $shc_ }{ $durs_[ $dur_idx_ ] } = $fwords_[ $dur_idx_+1 ];
              }
            }
          }
          elsif ( $tpercentile_ == $aggressive_threshold_ ) {
            foreach my $dur_idx_ ( 0..$#durs_ ) {
              if ( ! defined $shc_dur_to_agg_threshold_{ $shc_ }{ $durs_[ $dur_idx_ ] } ) {
                $shc_dur_to_agg_threshold_{ $shc_ }{ $durs_[ $dur_idx_ ] } = $fwords_[ $dur_idx_+1 ];
              }
            }
          }
        }
      }
    }
  }

  SanitizeProductsOnCorr ( );
}

sub SanitizeProductsOnCorr
{
  my @filtered_shclist_ = ( );
  foreach my $shc_ ( @shortcode_list_ ) {
    my @corrs_ = values %{ $shc_dur_to_corr_{ $shc_ } };
    my @rsq_ = values %{ $shc_dur_to_adj_rsq_{ $shc_ } };
    my @pxch_ = values %{ $shc_dur_to_75percentile_{ $shc_ } };

    if ( ! grep { abs($corrs_[$_]) > 0.0 && $rsq_[$_] > 0.0 } 0..$#corrs_ ) {
      print "Shortcode $shc_ has very low correlation and poor lm fits. Ignoring $shc_ for further processing\n";
    }
    elsif ( ! grep { abs($_) > 1.0 } @pxch_ ) {
      print "Shortcode $shc_ has very small price movements. Ignoring $shc_ for further processing\n";
    }
    else {
      push ( @filtered_shclist_, $shc_ );
    }
  }
  @shortcode_list_ = @filtered_shclist_;
}

sub MakeUTSList
{
  my $utslist_ = shift;

  open UTSListHandle, "< $uts_mur_list_" or PrintStacktraceAndDie( "Could Not open $uts_mur_list_ for reading\n" );
  my @utslist_contents_ = <UTSListHandle>; chomp ( @utslist_contents_ );
  close UTSListHandle;

  my @t_utslist_ = ( );
  foreach my $utsline_ ( @utslist_contents_ ) {
    my @utswords_ = split( " ", $utsline_ );
    my $shc_ = $utswords_[0];
    $shc_to_uts_{ $shc_ } = $utswords_[1];
    $shc_to_mur_{ $shc_ } = $utswords_[2];
    if ( FindItemFromVec ( $shc_, @shortcode_list_ ) ) {
      push ( @t_utslist_, $utsline_ );
    }
  }
  
  open UTSListHandle, "> $utslist_" or PrintStacktraceAndDie( "Could Not open $utslist_ for writing" );
  print UTSListHandle join("\n", @t_utslist_)."\n";
  close UTSListHandle;
}

sub PrepStrats
{
  my $paramsample_ = $AFLASHSCRIPTS_DIR."/paramfile_sample";
  if ( ! -f $paramsample_ ) {
    PrintStacktraceAndDie ( "Paramfile Sample $paramsample_ does not exist" );
  }

  my @time_tokens_ = split('_', $event_time_);
  if ( $#time_tokens_ > 1 ) {
    PrintStacktraceAndDie ( "The event time $event_time_ has incorrect format" );
  }

  my $tz_ = "";
  my $numeric_time_ = $time_tokens_[0];
  if ( $#time_tokens_ > 0 ) {
    $tz_ = $time_tokens_[0]."_";
    $numeric_time_ = $time_tokens_[1];
  }
  my $time_minutes_ = int($numeric_time_/100)*60 + $numeric_time_%100;
  my $time_min_bef_ = $time_minutes_ - 15;
  my $time_bef_ = $tz_.( int($time_min_bef_/60)*100 + $time_min_bef_%60 );
  
  foreach my $dur_ ( @sim_durs_ ) {
    my $dur_mins_ = $dur_;
    $dur_mins_ = 5 if $dur_ eq "dd";

    my $time_min_aft_ = $time_minutes_ + $dur_mins_;
    $time_min_aft_ += 5 if $dur_ eq "dd";
       
    my $time_aft_ = $tz_.( int($time_min_aft_/60)*100 + $time_min_aft_%60 );

    foreach my $shc_ ( @shortcode_list_ ) {
      my $paramfile_ = $shc_to_working_dir_{ $shc_ }."/param_".$shc_."_af_agg_".$dur_;
      my $modelfile_ = $shc_to_working_dir_{ $shc_ }."/model_".$shc_."_empty";
      my $stratfile_ = $shc_to_working_dir_{ $shc_ }."/w_af_exec_".$dur_;

      open PARAMHANDLE, "< $paramsample_" or PrintStacktraceAndDie( "Could not open $paramsample_ for reading" );
      my @paramlines_ = <PARAMHANDLE>; chomp ( @paramlines_ );
      close PARAMHANDLE;

      my $uts_ = $shc_to_uts_{ $shc_ };
      my $mur_ = $shc_to_mur_{ $shc_ };
      my $maxpxch_ = $shc_dur_to_75percentile_{ $shc_ }{ $dur_mins_ };
      my $thresh_ = $shc_dur_to_agg_threshold_{ $shc_ }{ $dur_mins_ };
      print "$shc_ $uts_ $mur_ maxpxch: $maxpxch_ thresh: $thresh_\n";

      my @dest_plines_ = ( );

      foreach my $pline_ ( @paramlines_ ) {
        my @pwords_ = split(" ", $pline_ );

        given ( $pwords_[1] ) {
          when ( "UNIT_TRADE_SIZE" ) { $pwords_[2] = $uts_; }
          when ( "MAX_UNIT_RATIO" ) { $pwords_[2] = $mur_; }
          when ( "AF_EVENT_ID" ) { $pwords_[2] = $event_id_; }
          when ( "AF_EVENT_MAX_UTS_PXCH" ) { $pwords_[2] = $maxpxch_; }
          when ( "AGGRESSIVE" ) { $pwords_[2] = $thresh_; }
        }

        push ( @dest_plines_, join(" ", @pwords_) );
      }

      push ( @dest_plines_, "PARAMVALUE AF_EVENT_GETFLAT_PXCH_MARGIN ".($getflat_scale_ * $maxpxch_) );
      push ( @dest_plines_, "PARAMVALUE AF_SCALE_BETA ".join(",", @{$shc_dur_to_beta_{ $shc_ }{ $dur_mins_  }} ) );
      if ( $dur_ eq "dd" ) {
        push ( @dest_plines_, "PARAMVALUE AF_EVENT_DRAWDOWN_SECS 150" );
      }

      if ( UseZNTrend( $shc_ ) ) {
        push ( @dest_plines_, "PARAMVALUE AF_SOURCE_SHC ZN_0" );
      }

      open PARAMHANDLE, "> $paramfile_" or PrintStacktraceAndDie( "Could not open $paramfile_ for writing" );
      print PARAMHANDLE join("\n", @dest_plines_)."\n";
      close PARAMHANDLE;

      open MHANDLE, "> $modelfile_" or PrintStacktraceAndDie( "Could not open $modelfile_ for writing" );
      print MHANDLE "MODELINIT DEPBASE $shc_ Midprice Midprice\nMODELMATH LINEAR CHANGE\nINDICATORSTART\nINDICATOREND\n";
      close MHANDLE;

      open SHANDLE, "> $stratfile_" or PrintStacktraceAndDie( "Could not open $stratfile_ for writing" );
      print SHANDLE "STRATEGYLINE $shc_ EventBiasAggressiveTrading $modelfile_ $paramfile_ $time_bef_ $time_aft_ 1111\n";
      close SHANDLE;

      $shc_dur_to_stratfile_{ $shc_ }{ $dur_ } = $stratfile_;
    }
  }
}

sub UseZNTrend
{
  my $shc_ = shift;

  my $zntr_file_ = $AFLASHSCRIPTS_DIR."/zn_products";
  open ZNHANDLE, "< $zntr_file_" or PrintStacktraceAndDie( "Could not open $zntr_file_ for reading");
  my @zntr_shclist_ = <ZNHANDLE>; chomp( @zntr_shclist_ );
  close ZNHANDLE;

  if ( FindItemFromVec( $shc_, @zntr_shclist_ ) ) { return 1; }
  return 0;
}

sub RunStrats
{
  my @dates_ = `awk -F, '{print \$1}' $evdat_file_`; chomp( @dates_ );
  @dates_ = grep { ValidDate ( $_ ) && $_ > 20130101 } @dates_;  # consider the dates after 20130101

    my $datefile_ = $working_dir_."/dates_2yrs";
  open DHANDLE, "> $datefile_" or PrintStacktraceAndDie( "Could not open $datefile_ for writing" );
  print DHANDLE join("\n", @dates_)."\n";
  close DHANDLE;

  my $runsim_script_ = $AFLASHSCRIPTS_DIR."/run_simulations_to_result_dir.pl";
  my $results_dir_ = $working_dir_."/results_aggress";

  foreach my $shc_ ( @shortcode_list_ ) {
    my $sttlist_file_ = $shc_to_working_dir_{ $shc_ }."/stratlist_agg";
    open STTHANDLE, "> $sttlist_file_" or PrintStacktraceAndDie( "Could not open $sttlist_file_ for writing" );
    foreach my $dur_ ( keys %{ $shc_dur_to_stratfile_{ $shc_ } } ) {
      print STTHANDLE $shc_dur_to_stratfile_{ $shc_ }{ $dur_ }."\n";
    }
    close STTHANDLE;

    my $exec_cmd_ = $runsim_script_." $shc_ $sttlist_file_ $datefile_ ALL $results_dir_";

    print $exec_cmd_."\n";

    my $logfile_ = $shc_to_working_dir_{ $shc_ }."/runlog";

    `$exec_cmd_ 1>$logfile_ 2>&1`;
  }
} 
