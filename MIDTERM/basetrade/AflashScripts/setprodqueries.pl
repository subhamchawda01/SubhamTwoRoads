#!/usr/bin/perl

# \file ModelScripts/setprodqueries.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use POSIX qw/ceil/;
use FileHandle;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME = "/spare/local/".$USER."/";

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
my $AFLASHSCRIPTS_DIR = $HOME_DIR."/".$REPO."/AflashScripts";
my $AFLASH_TRADEINFO_DIR = "/spare/local/tradeinfo/Alphaflash";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $TEMP_DIR = "/spare/local/temp";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr
require "$GENPERLLIB_DIR/get_unique_list.pl"; #GetUniqueList
require "$GENPERLLIB_DIR/read_shc_machine_mapping.pl"; # GetMachineForProduct

my $USAGE="$0 config-file event_time [time_to_close(in minutes)] [INSTALL=1] ";

if ( $#ARGV < 1 ) { 
  print $USAGE."\n";
  exit(0);
}

my $config_file_ = $ARGV[0];
my $ev_time_ = $ARGV[1];
my $ev_ttc_ = $ARGV[2];
my $to_install_ = 1;
$to_install_ = $ARGV[3] if $#ARGV > 2;
my $ev_id_;

my $date_ = `date +'%Y%m%d'`; chomp ( $date_ );

my @shc_list_ = ( );
my %shc_to_uts_ = ( );
my %shc_to_mur_ = ( );
my %shc_to_wcase_ = ( );
my %shc_to_maxpxch_ = ( );
my %shc_to_agg_thresh_ = ( );
my %shc_to_getflat_margin_ = ( );
my %shc_to_ttc_ = ( );
my %shc_to_maxloss_ = ( );
my %shc_to_dd_secs_ = ( );
my %shc_to_query_id_ = ( );
my %shc_to_serv_id_ = ( );
my %shc_to_beta_ = ( );
my %prod_to_sources_ = ( );
my %sources_map_ = ( );
my $read_af_mult_ = 0;

my $af_risk_scaling_ = 0;
my $dd_maxpxch_factor_ = -1;
my $dd_predpxch_factor_ = -1;

my @staggered_getflat_mins_ = ( );

my $maxloss_to_allow_ = 25000;
my $compute_maxloss_ = 1;
my $getflat_pxch_scale_ = 1.0;
my $dd_seconds_ = -1;
my $global_source_;

my $total_maxloss_ = 0;

LoadConfigFile ( );

LoadSources ( );

my %source_losses_ = ();
foreach my $shc_ ( keys %shc_to_maxloss_ ) {
  my $source_ = $shc_;
  if ( exists $sources_map_{ $shc_ } ) {
    $source_ = $sources_map_{ $shc_ };
  }

  if ( ! defined $source_losses_{ $source_ } ) {
    $source_losses_{ $source_ } = 0;
  }
  $source_losses_{ $source_ } += $shc_to_maxloss_{ $shc_ };
}

print "Source Losses: \n";
print $_.": ".$source_losses_{ $_ }."\n" foreach keys %source_losses_;

if ( $to_install_ ) {
  InstallConfigFile ( );
}

sub LoadConfigFile
{
  print "Loading Config File ...\n";

  open ( CONFIG_FILE , "<" , $config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $config_file_" );
  my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
  close ( CONFIG_FILE );

  my $current_shc_ = "";
  foreach my $cline_ ( @config_file_lines_ )
  {
    if ( $cline_ =~ /^#/ ) {  # not ignoring lines with # not at the beginning
      next;
    }
    my @t_words_ = split ( ' ' , $cline_ );

    if ( $#t_words_ < 0 ) {
      $current_shc_ = "";
      next;
    }

    if ( $#t_words_ < 1 ) {
      next;
    }

    my $param_ = $t_words_[0];

    if ( $current_shc_ eq "" ) {
      if ( $param_ eq "SHORTCODE" ) {
        $current_shc_ = $t_words_[1];
        push ( @shc_list_, $current_shc_ );
      }
      elsif ( $param_ eq "EVENT_ID" ) {
        $ev_id_ = $t_words_[1];
      }
      elsif ( $param_ eq "GETFLAT_SCALE" ) {
        $getflat_pxch_scale_ = $t_words_[1];
      }
      elsif ( $param_ eq "AF_EVENT_DRAWDOWN_SECS" ) {
        $dd_seconds_ = $t_words_[1];
      }
      elsif ( $param_ eq "USE_SOURCE_GLOBAL" ) {
        $global_source_ = $t_words_[1];
      }
      elsif ( $param_ eq "RISK_SCALING" ) {
        $af_risk_scaling_ = $t_words_[1];
      }
      elsif ( $param_ eq "DD_MAXPXCH_FACTOR" ) {
        $dd_maxpxch_factor_ = $t_words_[1];
      }
      elsif ( $param_ eq "DD_PREDPXCH_FACTOR" ) {
        $dd_predpxch_factor_ = $t_words_[1];
      }
      elsif ( $param_ eq "STAGGERED_GETFLAT_MINS" ) {
        @staggered_getflat_mins_ = @t_words_[1..$#t_words_];
      }
      else {
        PrintStacktraceAndDie ( "First Line of a new section should define SHORTCODE" );
      }
    }
    else {
      given ( $param_ ) {
        when ( "UTS" ) {
          $shc_to_uts_{ $current_shc_ } = $t_words_[1];
        }
        when ( "MUR" ) {
          $shc_to_mur_{ $current_shc_ } = $t_words_[1];
          $shc_to_wcase_{ $current_shc_ } = $t_words_[1];
        }
        when ( "PXCH_FOR_MAXORDERSIZE" ) {
          $shc_to_maxpxch_{ $current_shc_ } = $t_words_[1];
        }
        when ( "AGGRESSIVE_THRESHOLD" ) {
          $shc_to_agg_thresh_{ $current_shc_ } = $t_words_[1];
        }
        when ( "GETFLAT_MARGIN" ) {
          $shc_to_getflat_margin_{ $current_shc_ } = $t_words_[1];
        }
        when ( "MINUTES_TO_RUN" ) {
          $shc_to_ttc_{ $current_shc_ } = $t_words_[1];
        }
        when ( "MAXLOSS" ) {
          $shc_to_maxloss_{ $current_shc_ } = $t_words_[1];
        }
        when ( "DRAWDOWN_SECS" ) {
          $shc_to_dd_secs_{ $current_shc_ } = $t_words_[1];
        }
        when ( "AF_SCALE_BETA" ) {
          $shc_to_beta_{ $current_shc_ } = $t_words_[1];
        }
        when ( "USE_AF_MULT_LEVELS" ) {
          $read_af_mult_ = 1;
        }
        when ( "USE_SOURCE" ) {
          $prod_to_sources_{ $current_shc_ } = $t_words_[1];
        }
        when ( "SHORTCODE" ) {
          PrintStacktraceAndDie ( "SHORTCODE should always be First Line of a Section" );
        }
      }
    }
  }

  SanityCheck ( );
}


sub SanityCheck
{
  my @shc_to_skip_ = ( );

  if ( ! defined $ev_id_ ) {
    PrintStacktraceAndDie ( "EVENT_ID is not mentioned in the config-file" );
  }
  foreach my $shc_ ( @shc_list_ ) {
    if ( ! exists $shc_to_uts_{ $shc_ } ) {
      print "Error: UTS not defined for $shc_.. Skipping $shc_\n";
      push ( @shc_to_skip_, $shc_ );
      next;
    }
    if ( ! exists $shc_to_mur_{ $shc_ } ) {
      print "Error: MUR not defined for $shc_.. Skipping $shc_\n";
      push ( @shc_to_skip_, $shc_ );
      next;
    }
    if ( ! exists $shc_to_maxpxch_{ $shc_ } ) {
      print "Error: PXCH_FOR_MAXORDERSIZE not defined for $shc_.. Skipping $shc_\n";
      push ( @shc_to_skip_, $shc_ );
      next;
    }
    if ( ! exists $shc_to_getflat_margin_{ $shc_ } ) {
      print "Warning: GETFLAT_MARGIN not set for $shc_.. Setting it equal to $getflat_pxch_scale_ * PXCH_FOR_MAXORDERSIZE\n";
      $shc_to_getflat_margin_{ $shc_ } = $getflat_pxch_scale_ * $shc_to_maxpxch_{ $shc_ };
    }
    if ( ! exists $shc_to_agg_thresh_{ $shc_ } ) {
      print "Warning: AGGRESSIVE_THRESHOLD not set for $shc_.. Setting it to 0\n";
      $shc_to_agg_thresh_{ $shc_ } = 0;
    }
    if ( ! exists $shc_to_ttc_{ $shc_ } ) {
      print "Warning: MINUTES_TO_RUN not set for $shc_.. Setting it to the commandline-arg: $ev_ttc_\n";
      $shc_to_ttc_{ $shc_ } = $ev_ttc_;
    }
    if ( ! exists $shc_to_maxloss_{ $shc_ } ) {
      my $tick_size_ = `$BIN_DIR/get_min_price_increment $shc_ $date_ 2>/dev/null`;
      chomp ( $tick_size_ );
      my $n2d_ = `$BIN_DIR/get_numbers_to_dollars $shc_ $date_ 2>/dev/null`;
      chomp ( $n2d_ );

      if ( $tick_size_ eq "" ) {
        print "Error: min_price_increment could not be computed for shortcode $shc_.. Skipping $shc_\n";
        push ( @shc_to_skip_, $shc_ );
        next;
      }

      my $maxloss_computed_ = $shc_to_uts_{ $shc_ } * $shc_to_mur_{ $shc_ } * $shc_to_getflat_margin_{ $shc_ } * $n2d_ * $tick_size_ * 1.1;
      $maxloss_computed_ = ceil( $maxloss_computed_ / 100 ) * 100;
      $maxloss_computed_ = max(1000, $maxloss_computed_);

      $total_maxloss_ += $maxloss_computed_;

      if ( $compute_maxloss_ || !($shc_ =~ /^DI/) ) {
        print "Warning: MAXLOSS not set for $shc_.. Setting it to $maxloss_computed_\n";
        $shc_to_maxloss_{ $shc_ } = $maxloss_computed_;
      }
      else {
        print "Warning: MAXLOSS not set for $shc_.. Setting it to 5000\n";
        $shc_to_maxloss_{ $shc_ } = 5000;
      }
    }

    if ( $shc_to_maxloss_{ $shc_ } > $maxloss_to_allow_ ) {
      print "Error: MAXLOSS for $shc_ exceeds $maxloss_to_allow_.. Skipping $shc_\n";
      next;
    }

    print "\n"; 
  }

  print "Sum of Computed Maxlosses for All Products (except DIs): ".$total_maxloss_."\n\n";

  @shc_list_ = grep { ! FindItemFromVec( $_, @shc_to_skip_ ) } @shc_list_;

  print "Load ServId and QueryId Map\n";
  GetServIdQueryIdMaps ( );

  print "Checking the MaxOrderSize and Maxposition Limits for the products\n";
  CheckShcLimits ( );
}

sub CheckShcLimits
{
  my @serv_id_vec_ = GetUniqueList ( values %shc_to_serv_id_ );
#  print "All serv_id: \n".join("\n", @serv_id_vec_)."\n";

  my @skip_shc_vec_ = ( );
  my %shc_visited_ = ( );

  foreach my $serv_id_ ( @serv_id_vec_ ) {
   print "Loading the Limits for Serv: ".$serv_id_."\n"; 
    my @serv_shc_vec_ = grep { $shc_to_serv_id_{ $_ } eq $serv_id_ } keys %shc_to_serv_id_;
    
    my %shc_to_max_ordersize_ = ( );
    my %shc_to_max_position_ = ( );

    my %shcgrp_to_max_ordersize_ = ( );
    my %shcgrp_to_max_position_ = ( );
    my %shcgrp_to_shc_to_weight_ = ( );

    my $get_limits_cmd_ = "ssh dvcinfra\@$serv_id_ \'cat /home/pengine/prod/live_configs/\`hostname\`_addts.cfg\'";
    my @symbol_limit_lines_ = `$get_limits_cmd_ 2>/dev/null`; chomp( @symbol_limit_lines_ );
    @symbol_limit_lines_ = grep { !( $_ =~ /^#/) } @symbol_limit_lines_; 

    foreach my $limit_line_ ( @symbol_limit_lines_ ) {
      my @limit_words_ = split( " ", $limit_line_ );
      if ( $#limit_words_ < 6 ) {
        next;
      }
      if ( $limit_words_[2] eq "ADDTRADINGSYMBOL" ) {
        my $shc_ = $limit_words_[3];
        if ( FindItemFromVec ( $shc_, @serv_shc_vec_ ) ) {
          $shc_to_max_position_{ $shc_ } = $limit_words_[4];
          $shc_to_max_ordersize_{ $shc_ } = $limit_words_[5];
        }
      }
      elsif ( $limit_words_[2] eq "ADDCOMBINEDTRADINGSYMBOL" ) {
        my $shcgrp_ = $limit_words_[3];
        $shcgrp_to_max_position_{ $shcgrp_ } = $limit_words_[4];
        $shcgrp_to_max_ordersize_{ $shcgrp_ } = $limit_words_[5];

        my $shc_indx_ = 0;
        while ( $#limit_words_ > $shc_indx_ + 8 ) {
          my $shc_ = $limit_words_[ $shc_indx_ + 8 ];
          if ( FindItemFromVec ( $shc_, @serv_shc_vec_ ) ) {
            $shcgrp_to_shc_to_weight_{ $shcgrp_ }{ $shc_ } = $limit_words_[ $shc_indx_ + 9 ];
          }
          $shc_indx_ += 2;
        }
      }
    }

    foreach my $shc_ ( @serv_shc_vec_ ) {

      if ( exists $shc_to_max_ordersize_{ $shc_ } && exists $shc_to_max_position_{ $shc_ } ) {
        if ( $shc_to_uts_{ $shc_ } > $shc_to_max_ordersize_{ $shc_ } ) {
          print "ERROR: UTS for $shc_ (".$shc_to_uts_{ $shc_ }.") exceeds its MAX_ORDER_SIZE limit (".$shc_to_max_ordersize_{ $shc_ }.")\n";
          push ( @skip_shc_vec_, $shc_ );
        }
        my $max_position_ = $shc_to_uts_{ $shc_ } * $shc_to_mur_{ $shc_ };
        if ( $max_position_  > $shc_to_max_position_{ $shc_ } ) {
          print "ERROR: MaxPosition for $shc_ (".$max_position_.") exceeds its MAX_POSITION limit (".$shc_to_max_position_{ $shc_ }.")\n";
          push ( @skip_shc_vec_, $shc_ );
        }
        $shc_visited_{ $shc_ } = 1;
      }
      else {
        $shc_visited_{ $shc_ } = 0;
      }
    }

    foreach my $shcgrp_ ( keys %shcgrp_to_shc_to_weight_ ) {
      my $combined_maxpos_ = 0;

      foreach my $shc_ ( keys %{ $shcgrp_to_shc_to_weight_{ $shcgrp_ } } ) {
        $combined_maxpos_ += $shc_to_uts_{ $shc_ } * $shc_to_mur_{ $shc_ } * $shcgrp_to_shc_to_weight_{ $shcgrp_ }{ $shc_ };

        my $t_ordersize_ = $shc_to_uts_{ $shc_ } * $shcgrp_to_shc_to_weight_{ $shcgrp_ }{ $shc_ };
        if ( $t_ordersize_ > $shcgrp_to_max_ordersize_{ $shcgrp_ } ) {
          print "ERROR: UTS for $shc_ (".$t_ordersize_.") exceeds the ORDER_SIZE limit for $shcgrp_ (".$shcgrp_to_max_ordersize_{ $shcgrp_ }.")\n";
          push ( @skip_shc_vec_, $shc_ );
        }

        $shc_visited_{ $shc_ } = 1;
      } 
      
      if ( $combined_maxpos_ > $shcgrp_to_max_position_{ $shcgrp_ } ) {
        print "ERROR: Combined Maxposition for $shcgrp_ (".$combined_maxpos_.") exceeds its MAX_POSITION limit (".$shcgrp_to_max_position_{ $shcgrp_ }.")\n";
        foreach my $shc_ ( keys %{ $shcgrp_to_shc_to_weight_{ $shcgrp_ } } ) {
          push ( @skip_shc_vec_, $shc_ );
        }
      }
    }
  }

  if ( $#skip_shc_vec_ >= 0 ) {
  print "WARN: Limits could NOT be met for shortcodes: ".join(" ", @skip_shc_vec_)."\n";
  @shc_list_ = grep { ! FindItemFromVec( $_, @skip_shc_vec_ ) } @shc_list_;
  }

  my @shc_limit_unchecked_vec_ = grep { $shc_visited_{ $_ } == 0 } keys %shc_visited_;
  if ( $#shc_limit_unchecked_vec_ >= 0 ) {
    print "WARN: Limits could NOT be fetched for shortcodes: ".join(" ", @shc_limit_unchecked_vec_ )."\n";
  }
}
  
sub InstallConfigFile
{
  foreach my $shc_ ( @shc_list_ ) {
    my $serv_id_ = $shc_to_serv_id_{ $shc_ };
    my $query_id_ = $shc_to_query_id_{ $shc_ };

    my $paramsample_ = $AFLASHSCRIPTS_DIR."/paramfile_sample";

    my ($local_strat_name_, $local_model_file_, $local_param_file_ );
    my ($remote_strat_name_, $remote_model_file_, $remote_param_file_ );

    my $local_suffix_ = "_".$shc_."_af_".$ev_id_;

    $local_strat_name_ = $TEMP_DIR."/strat_".$local_suffix_;
    $local_model_file_ = $TEMP_DIR."/model_".$local_suffix_;
    $local_param_file_ = $TEMP_DIR."/param_".$local_suffix_;

    my $remote_path_ = "/home/dvctrader/af_strats/general/".$shc_;
    $remote_strat_name_ = $remote_path_."/w_af_exec_".$query_id_;
    $remote_model_file_ = $remote_path_."/model_".$shc_."_".$query_id_;
    $remote_param_file_ = $remote_path_."/param_".$shc_."_".$query_id_;

    my @time_tokens_ = split('_', $ev_time_);
    if ( $#time_tokens_ > 1 ) {
      PrintStacktraceAndDie ( "The event time $ev_time_ has incorrect format" );
    }

    my $tz_ = "";
    my $numeric_time_ = $time_tokens_[0];
    if ( $#time_tokens_ > 0 ) {
      $tz_ = $time_tokens_[0]."_";
      $numeric_time_ = $time_tokens_[1];
    }

    my $time_minutes_ = int($numeric_time_/100)*60 + $numeric_time_%100;
    my $time_min_bef_ = $time_minutes_ - 15;
    my $time_min_aft_ = $time_minutes_ + $shc_to_ttc_{ $shc_ };
    my $time_bef_ = $tz_.( int($time_min_bef_/60)*100 + $time_min_bef_%60 );
    my $time_aft_ = $tz_.( int($time_min_aft_/60)*100 + $time_min_aft_%60 );

    open FHANDLE, "> $local_strat_name_" or PrintStacktraceAndDie ("Could not open $local_strat_name_ for writing");
    print FHANDLE "STRATEGYLINE $shc_ EventBiasAggressiveTrading $remote_model_file_ $remote_param_file_ $time_bef_ $time_aft_ $query_id_\n";
    close FHANDLE;

    open FHANDLE, "> $local_model_file_" or PrintStacktraceAndDie ("Could not open $local_model_file_ for writing");
    print FHANDLE "MODELINIT DEPBASE $shc_ Midprice Midprice\nMODELMATH LINEAR CHANGE\nINDICATORSTART\nINDICATOREND\n";
    close FHANDLE;

    open PARAMHANDLE, "< $paramsample_" or PrintStacktraceAndDie( "Could not open $paramsample_ for reading" );
    my @paramlines_ = <PARAMHANDLE>; chomp ( @paramlines_ );
    close PARAMHANDLE;

    my @dest_plines_ = ( );

    my ($read_uts_, $read_mur_, $read_maxpxch_, $read_agg_, $read_getflat_pxch_, $read_evid_) = (0,0,0,0,0,0);

    foreach my $pline_ ( @paramlines_ ) {
      my @pwords_ = split(" ", $pline_ );

      given ( $pwords_[1] ) {
        when ( "UNIT_TRADE_SIZE" ) {
          $pwords_[2] = $shc_to_uts_{ $shc_ };
          $read_uts_ = 1;
        }
        when ( "MAX_UNIT_RATIO" ) { 
          $pwords_[2] = $shc_to_mur_{ $shc_ }; 
          $read_mur_ = 1;
        }
        when ( "WORST_CASE_UNIT_RATIO" ) {
          $pwords_[2] = $shc_to_wcase_{ $shc_ };
        }
        when ( "AF_EVENT_MAX_UTS_PXCH" ) {
          $pwords_[2] = $shc_to_maxpxch_{ $shc_ }; 
          $read_maxpxch_ = 1;
        }
        when ( "AGGRESSIVE" ) {
          $pwords_[2] = $shc_to_agg_thresh_{ $shc_ }; 
          $read_agg_ = 1;
        }
        when ( "AF_EVENT_GETFLAT_PXCH_MARGIN" ) {
          $pwords_[2] = $shc_to_getflat_margin_{ $shc_ };
          $read_getflat_pxch_ = 1;
        }
        when ( "AF_EVENT_ID" ) { 
          $pwords_[2] = $ev_id_; 
          $read_evid_ = 1;
        }
      }

      push ( @dest_plines_, join(" ", @pwords_) );
    }

    if ( ! $read_getflat_pxch_ ) {
      push ( @dest_plines_, "PARAMVALUE AF_EVENT_GETFLAT_PXCH_MARGIN ".$shc_to_getflat_margin_{ $shc_ } );
      $read_getflat_pxch_ = 1;
    }

    my $t_dd_seconds_ = -1;
    $t_dd_seconds_ = $dd_seconds_ if $dd_seconds_ > 0;
    $t_dd_seconds_ = $shc_to_dd_secs_{ $shc_ } if (defined $shc_to_dd_secs_{ $shc_ } && $shc_to_dd_secs_{ $shc_ } > 0);

    if ( $t_dd_seconds_ > 0 ) {
      push ( @dest_plines_, "PARAMVALUE AF_EVENT_DRAWDOWN_SECS ".$t_dd_seconds_ );
    }

    if ( defined $shc_to_beta_{ $shc_ } ) {
      push ( @dest_plines_, "PARAMVALUE AF_SCALE_BETA ".$shc_to_beta_{ $shc_ } );
    }

    if ( $af_risk_scaling_ > 0 ) {
      push ( @dest_plines_, "PARAMVALUE AF_RISK_SCALING ".$af_risk_scaling_ );
    }

    if ( $#staggered_getflat_mins_ >= 0 ) {
      push ( @dest_plines_, "PARAMVALUE AF_STAGGERED_GETFLAT_MINS ".join(" ", @staggered_getflat_mins_) );
    }

    if ( $dd_maxpxch_factor_ >= 0 && $dd_predpxch_factor_ >= 0 ) {
      push ( @dest_plines_, "PARAMVALUE AF_DD_MAXPXCH_FACTOR ".$dd_maxpxch_factor_ );
      push ( @dest_plines_, "PARAMVALUE AF_DD_PREDPXCH_FACTOR ".$dd_predpxch_factor_ );
    }

    if ( defined $prod_to_sources_{ $shc_ } ) {
      my $tsource_ = $prod_to_sources_{ $shc_ };
      if ( $tsource_ ne "None" && ExistShcBeta ( $tsource_ ) ) {
        push ( @dest_plines_, "PARAMVALUE AF_SOURCE_SHC $tsource_" );
      }
      else {
        print "Error: Beta Not present for source of $shc_ ($tsource_).. Skipping source for $shc_\n";
      }
    }

    if ( ! grep { $_ =~ /THROTTLE_MSGS_PER_SEC/ } @dest_plines_ ) {
      push ( @dest_plines_, "PARAMVALUE THROTTLE_MSGS_PER_SEC 100" );
    }

    if ( $read_af_mult_ > 0 ) {
      push ( @dest_plines_, "PARAMVALUE AF_MULT_LEVELS 1" );
    }

    if ( ! ($read_uts_ && $read_mur_ && $read_maxpxch_ && $read_agg_ && $read_getflat_pxch_ && $read_evid_) ) {
      print "Error: Paramfile NOT valid for shortcode $shc_.. Skipping $shc_ installation\n";
      next;
    }

    open PARAMHANDLE, "> $local_param_file_" or PrintStacktraceAndDie( "Could not open $local_param_file_ for writing" );
    print PARAMHANDLE join("\n", @dest_plines_)."\n";
    close PARAMHANDLE;

    my $exec_cmd_ = "ssh dvctrader\@$serv_id_ \"if test ! -d $remote_path_ ; then mkdir -p $remote_path_; fi\"";
#    print $exec_cmd_."\n";
    `$exec_cmd_`;

    $exec_cmd_ = "rsync -avz $local_param_file_ dvctrader\@$serv_id_:$remote_param_file_";
    print $exec_cmd_."\n";
    `$exec_cmd_`;

    $exec_cmd_ = "rsync -avz $local_model_file_ dvctrader\@$serv_id_:$remote_model_file_";
    print $exec_cmd_."\n";
    `$exec_cmd_`;

    $exec_cmd_ = "rsync -avz $local_strat_name_ dvctrader\@$serv_id_:$remote_strat_name_";
    print $exec_cmd_."\n";
    `$exec_cmd_`;

    `rm -f $local_param_file_ $local_model_file_ $local_strat_name_`;
  }
}

sub LoadSources
{
  my $sources_file_ = $AFLASHSCRIPTS_DIR."/sources.txt";
  open SHANDLE, "< $sources_file_" or PrintStacktraceAndDie( "Could not open $sources_file_ for reading" );
  my @slines_ = <SHANDLE>; chomp ( @slines_ );
  close SHANDLE;
  %sources_map_ = map { (split(/\s+/, $_))[0] => (split(/\s+/, $_))[1] } @slines_;

  foreach my $shc_ ( @shc_list_ ) {
    next if defined $prod_to_sources_{ $shc_ };

    if ( defined $global_source_ ) {
      $prod_to_sources_{ $shc_ } = $global_source_;
    }
    elsif ( defined $sources_map_{ $shc_ } ) {
      $prod_to_sources_{ $shc_ } = $sources_map_{ $shc_ };
    }
  }
}

sub ExistShcBeta
{
  my $tshc_ = shift;
  my $beta_file_ = $AFLASH_TRADEINFO_DIR."/af_events_scale_ids2.txt";

  my @beta_lines_ = `grep \^$tshc_ $beta_file_`; chomp ( @beta_lines_ );
  @beta_lines_ = grep { (split(/\s+/, $_))[1] eq $ev_id_ } @beta_lines_;

  if ( $#beta_lines_ < 0 ) { return 0; }
  else { return 1; }
}

sub GetServIdQueryIdMaps
{
  my $queryids_map_file_ = $AFLASHSCRIPTS_DIR."/queryids_map.txt";

  open FHANDLE, "< $queryids_map_file_" or PrintStacktraceAndDie ( "Could not open $queryids_map_file_ for reading" );
  my @config_lines_ = <FHANDLE>; chomp ( @config_lines_ );
  close FHANDLE;

  foreach my $line_ ( @config_lines_ ) {
    my @twords_ = split(" ", $line_ );
    if ( $#twords_ < 1 ) { next; }
    if ( FindItemFromVec( $twords_[0], @shc_list_ ) ) {
      my $shc_ = $twords_[0];
      my $shc_idx_ = $twords_[1];
      if ( $shc_idx_ > 99 ) {
        print "ERROR: shortcode index for $shc_ $shc_idx_, exceeds 99\n";
        exit( 1 );
      }
      $shc_to_query_id_{ $shc_ } = sprintf("36%03d%03d", $ev_id_, $shc_idx_);
      $shc_to_serv_id_{ $shc_ } = GetMachineForProduct( $shc_ );
    }
  }

  my @valid_shclist_ = ( );
  foreach my $shc_ ( @shc_list_ ) {
    if ( ! defined $shc_to_query_id_{ $shc_ } || $shc_to_query_id_{ $shc_ } eq "" ) {
      print "WARN: For $shc_, query_id could not be determined.. Skipping it\n";
      next;
    }
    elsif ( ! defined $shc_to_serv_id_{ $shc_ } || $shc_to_serv_id_{ $shc_ } eq "" ) {
      print "WARN: For $shc_, production server id could not be determined.. Skipping it\n";
      next;
    }

    push ( @valid_shclist_, $shc_ );
  }
  my @shc_to_skip_ = grep { ! FindItemFromVec ( $_, @valid_shclist_ ) } @shc_list_;
  @shc_list_ = @valid_shclist_;

  if ( $#shc_to_skip_ >= 0 ) {
    delete @shc_to_query_id_{ @shc_to_skip_ };
    delete @shc_to_serv_id_{ @shc_to_skip_ };
  }

}

