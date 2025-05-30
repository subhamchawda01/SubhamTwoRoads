#!/usr/bin/perl

# \file ModelScripts/pick_strats_and_install.pl
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
use Math::Complex; # sqrt
use FileHandle;
use POSIX;
use sigtrap qw(handler signal_handler normal-signals error-signals);

# TODO : Figure out how to factor in diversity weights.

sub LoadConfigFile;
sub InstallProductionStratList;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_REMOTE_DIR="/home/dvctrader/LiveModels";
my $MODELING_STIR_STRATS_DIR=$MODELING_BASE_DIR."/stir_strats"; # this directory is used to store the chosen strategy files
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $LOCAL_PRODUCTION_STRAT_LIST_DIR = $HOME_DIR."/production_strat_list";
my $local_production_strat_list_file_ = "";

my $REMOTE_PRODUCTION_STRAT_LIST_DIR = "/home/dvctrader/production_strat_list";
my $remote_production_strat_list_file_ = "";

my @intermediate_files_ = ( );

require "$GENPERLLIB_DIR/sqrt_sign.pl"; # SqrtSign
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_insample_date.pl"; # GetInsampleDate

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr

require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams

require "$GENPERLLIB_DIR/is_weird_sim_day_for_shortcode.pl"; # IsWeirdSimDayForShortcode

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst

require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile

require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

require "$GENPERLLIB_DIR/get_basepx_strat_first_model.pl"; # GetBasepxStratFirstModel

require "$GENPERLLIB_DIR/strat_utils.pl"; # GetRegimeInd

# start
my $USAGE="$0 SHORTCODE TIMEPERIOD CONFIGFILE";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }

my $mail_body_ = "";

my $shortcode_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];
my $config_file_ = $ARGV [ 2 ];

my $exchange_ = "";

my $verbose_ = 0;
my $install_ = 1;
my $delete_intermediate_files_ = 1;
my $send_mail_ = 1;

my %shc_to_min_order_size_ = ();
my %shc_to_uts_ = ( );
my %shc_to_param_uts_ = () ; # uts in the modelling params
my %id_to_strat_base_ = ( );
my %id_to_remote_strat_ = ( );
my @strats_base_ = ( );
my %strat_name_to_subset_index_ = ( );
my %id_to_prod_to_max_loss_per_unit_size_ = () ;
my @strats_path_;
my @strats_im_path_;
my $max_loss_ = -1;
my $max_opentrade_loss_ = -1;
my $max_global_risk_ = -1;

my $email_address_ = "";
my @exchanges_to_remove_ = ( );
my @shortcodes_to_remove_ = ( );

my $install_location_ = "";
my $prod_query_start_id_ = 0;
my $exec_start_hhmm_ = 0;
my $exec_end_hhmm_ = 0;
my $onload_trade_exec_ = 0;
my $vma_trade_exec_ = 0;
my $affinity_trade_exec_ = 0;
my $use_sharpe_pnl_check_ = 0;
my $use_optimal_max_loss_ = 0 ;
my $read_OML_settings_ = 0 ;
my $OML_lookback_days_ = 0 ;
my $OML_hit_ratio_ = 0 ;
my $OML_number_top_loss_ = 0 ;
my $risk_scale_ = 1 ; 
my $uts_scale_ = 0.5 ;
my $risk_tags_ = "GLOBAL";  # Tags for risk monitor computations

my @remote_strat_full_path_list_ = ( );

my @skip_dates_vector_ = ( );
my $skip_dates_file_ = "/spare/local/".$USER."/skip_dates_file_".$shortcode_ ;

my $remote_lockfile_ = "/home/dvctrader/pick_strat_lock";
my $is_remote_lock_created_ = 0;

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

LoadConfigFile ( $config_file_ );
SanityCheckConfigParams ( );

InstallAll ( );

RemoveIntermediateFiles ( );

SendReportMail ( );

sub InstallAll
{
  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  $mail_body_ = $mail_body_." # InstallStrats\n\n";

  ClearProductionStratList ( );
  
  my $install_string_ = "";
  foreach my $id_  ( keys %id_to_strat_base_ )
  {
    InstallStirStrat ( $id_ );

    $install_string_ = $install_string_."\n\t INSTALLED : query id: ".$id_." , stir_stratname: ".$id_to_strat_base_{$id_}." , remote_name: ".$id_to_remote_strat_{$id_}."\n";
    print "INSTALLED : query id: ".$id_." , stir_stratname: ".$id_to_strat_base_{$id_}." , remote_name: ".$id_to_remote_strat_{$id_}."\n\n";
  }
  
  $mail_body_ = $mail_body_.$install_string_;
  $mail_body_ = $mail_body_." # InstallCrontab\n\n";

  foreach my $id_ ( keys %id_to_remote_strat_ )
  {
    my $remote_strat_ = $id_to_remote_strat_ {$id_};

    $mail_body_ = $mail_body_."\t\t [ $id_ ] ".$remote_strat_."\n";

    AddToProductionStratList ( $remote_strat_ , $install_location_ , $id_ );
  }

  InstallProductionStratList ( );
}

sub LoadConfigFile
{
  my ( $t_config_file_ ) = @_;

  open ( CONFIG_FILE , "<" , $t_config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $t_config_file_" );
  my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
  close ( CONFIG_FILE );

  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  $mail_body_ = $mail_body_." > CONFIG_FILE=".$t_config_file_."\n";

  my $current_param_ = "";
  foreach my $config_file_lines_ ( @config_file_lines_ )
  {
    if ( index ( $config_file_lines_ , "#" ) == 0 ) # not ignoring lines with # not at the beginning
    {
      next;
    }

    my @t_words_ = split ( ' ' , $config_file_lines_ );

    if ( $#t_words_ < 0 )
    {
      $current_param_ = "";
      next;
    }

    if ( ! $current_param_ )
    {
      $current_param_ = $t_words_ [ 0 ];
      next;
    }
    else
    {
      given ( $current_param_ )
      {
        when ( "SHORTCODE" )
        {
          my $t_shortcode_ = $t_words_ [ 0 ];
          if ( $t_shortcode_ ne $shortcode_ )
          {
            PrintStacktraceAndDie ( "$t_shortcode_ in config file != $shortcode_" );
          }
          $mail_body_ = $mail_body_." \t > SHORTCODE=".$t_shortcode_."\n";
        }

        when ( "EXCHANGE" )
        {
          $exchange_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > EXCHANGE=".$exchange_."\n";
        }


        when ( "UNIT_TRADE_SIZE" )
        {
          my $shc_ = $t_words_ [ 0 ];
          my $uts_ = $t_words_ [ 1 ];

          $shc_to_uts_ {$shc_} = $uts_;
          $mail_body_ = $mail_body_." \t > UTS for ".$shc_." = ".$uts_."\n";
        }

        when ( "EMAIL_ADDRESS" )
        {
          $email_address_ = $t_words_ [ 0 ];
        }

        when ( "INSTALL" )
        {
          $install_ = $t_words_ [0];
          $mail_body_ = $mail_body_." \t > TO_INSTALL = ".$install_."\n";
        }

        when ( "INSTALL_LOCATION" )
        {
          $install_location_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > INSTALL_PICKED_STRATS=".$install_location_."\n";    		
        }

        when ( "RISK_TAGS" )
        {
          $risk_tags_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > RISK_TAGS=".$risk_tags_."\n";
        }

        when ( "PROD_QUERY_START_ID" )
        {
          $prod_query_start_id_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > PROD_QUERY_START_ID=".$prod_query_start_id_."\n";
        }

        when ( "MAX_LOSS_PER_STRAT" )
        {
          $max_loss_ = $t_words_[ 0 ];
          $mail_body_ = $mail_body_." \t > MAX_LOSS=".$max_loss_."\n";
        }

        when ( "MAX_OPENTRADE_LOSS" )
        {
          $max_opentrade_loss_ = $t_words_[ 0 ];
          $mail_body_ = $mail_body_." \t > MAX_OPENTRADE_LOSS=".$max_loss_."\n";
        }

        when ( "MAX_GLOBAL_RISK" )
        {
          $max_global_risk_ = $t_words_[ 0 ];
          $mail_body_ = $mail_body_." \t > MAX_GLOBAL_RISK=".$max_loss_."\n";
        }

        when ( "EXEC_START_HHMM" )
        {
          $exec_start_hhmm_ = GetUTCHHMMStr ( $t_words_ [ 0 ], $yyyymmdd_ ); 
          if ( $exec_start_hhmm_ < 10 )
          { # 0007 -> 7 , causing substrs to fail when installing cron.
            $exec_start_hhmm_ = "000".$exec_start_hhmm_;
          }
          elsif ( $exec_start_hhmm_ < 100 )
          { # 0010 -> 10 , causing substrs to fail when installing cron.
            $exec_start_hhmm_ = "00".$exec_start_hhmm_;
          }
          elsif ( $exec_start_hhmm_ < 1000 )
          { # 0710 -> 710 , causing substrs to fail when installing cron.
            $exec_start_hhmm_ = "0".$exec_start_hhmm_;
          }


          $mail_body_ = $mail_body_." \t > EXEC_START_HHMM=".$exec_start_hhmm_."\n";    
        }

        when ( "EXEC_END_HHMM" )
        {
          $exec_end_hhmm_ = GetUTCHHMMStr ( $t_words_ [ 0 ], $yyyymmdd_ );

          if ( $exec_end_hhmm_ < 10 )
          { # 0710 -> 710 , causing substrs to fail when installing cron.
            $exec_end_hhmm_ = "000".$exec_end_hhmm_;
          }
          elsif ( $exec_end_hhmm_ < 100 )
          { # 0710 -> 710 , causing substrs to fail when installing cron.
            $exec_end_hhmm_ = "00".$exec_end_hhmm_;
          }
          elsif ( $exec_end_hhmm_ < 1000 )
          { # 0710 -> 710 , causing substrs to fail when installing cron.
            $exec_end_hhmm_ = "0".$exec_end_hhmm_;
          }

          $mail_body_ = $mail_body_." \t > EXEC_END_HHMM=".$exec_end_hhmm_."\n";    
        }

        when ( "ONLOAD_TRADE_EXEC" )
        {
          $onload_trade_exec_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > ONLOAD_TRADE_EXEC=".$onload_trade_exec_."\n";
        }

        when ( "VMA_TRADE_EXEC" )
        {
          $vma_trade_exec_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > VMA_TRADE_EXEC=".$vma_trade_exec_."\n";
        }

        when ( "AFFINITY_TRADE_EXEC" )
        {
          $affinity_trade_exec_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > AFFINITY_TRADE_EXEC=".$affinity_trade_exec_."\n";
        }

        when ( "STRATS_TO_KEEP" )
        {
          my $strat_ = $t_words_[0];
          $mail_body_ = $mail_body_." \t > STRAT=".$strat_."\n";
          push ( @strats_base_ , $strat_ );
        }
        when ( "USE_OPTIMAL_MAX_LOSS" )
        {
        	if ( $#t_words_ >= 0 )
        	{
        		$use_optimal_max_loss_ = $t_words_[0];
        	}
        	$mail_body_ = $mail_body_."\t > USE_OPTIMAL_MAX_LOSS ".$use_optimal_max_loss_."\n";
        }
         when ( "OPTIMAL_MAX_LOSS_SETTINGS" )
        { 
# This if present would override the default settings for optimal max_loss calculations.
          $read_OML_settings_ = 1 ;
          $OML_lookback_days_ = $t_words_ [ 0 ] ;
          $OML_hit_ratio_ = min ( 0.2, $t_words_ [ 1 ] ) ; #greater than 20% is not sane
          $OML_number_top_loss_ = $t_words_ [ 2 ] ;
          $mail_body_ = $mail_body_."\t > OPTIMAL_MAX_LOSS_SETTING $config_file_lines_\n";
        }
        when ("RISK_SCALE")
        {
        	if ( $#t_words_ >= 0 )
        	{
        		$risk_scale_ = $t_words_[0];
        	} 
        }
        when ( "UTS_SCALE" )
        {
            if ( $#t_words_ >= 0 )
            {
                $uts_scale_ = $t_words_[0];
            }
        }
      }
    }
  }
  return;
}

sub SanityCheckConfigParams
{
  if ( $install_ )
  {
    my $lock_exists_ = `ssh $install_location_ "if [ -f $remote_lockfile_ ]; then echo 1; else echo 0; fi"`; chomp($lock_exists_);
    if($lock_exists_)
    {
      PrintStacktraceAndDie("lockfile $remote_lockfile_ exists at $install_location_\nEither already a pick_strats is running or somebody messed up in last pick_strats for this server.\n" );
    }
    print "creating lock $install_location_:$remote_lockfile_. Make sure it is deleted after the run.\n";
    `ssh $install_location_ "touch $remote_lockfile_"`;
    $is_remote_lock_created_ = 1;
  }

  $local_production_strat_list_file_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$shortcode_.".".$timeperiod_.".".basename ( $config_file_ );
  push ( @intermediate_files_ , $local_production_strat_list_file_ );

  $remote_production_strat_list_file_ = $REMOTE_PRODUCTION_STRAT_LIST_DIR."/".$shortcode_.".".$timeperiod_.".".basename ( $config_file_ );

  my $query_id_ = $prod_query_start_id_ ;
  foreach my $strat_ ( @strats_base_ )
  {
    $id_to_strat_base_ { $query_id_ } = $strat_ ;
    $query_id_ ++;
  }

  if ( $verbose_ ) { print " query id: ".$_.", strat base: ".$id_to_strat_base_{$_}."\n" for keys %id_to_strat_base_; }
}

sub InstallStirStrat
{
  my $query_id_ = $_[0];
  my $strat_base_ = $id_to_strat_base_{$query_id_};
  
  my $strat_path_ = `ls $MODELING_STIR_STRATS_DIR/$shortcode_/*/$strat_base_ 2> /dev/null`; chomp ( $strat_path_ );
  if ( $strat_path_ eq "" )
  {
    $strat_path_ = `ls $MODELING_STIR_STRATS_DIR/$shortcode_/$strat_base_ 2> /dev/null`; chomp ( $strat_path_ );
  }
  push ( @strats_path_, $strat_path_ );
  if ( $verbose_ ) { print "stratpath: ".$strat_path_."\n"; }
  
  if ( ! ( -f $strat_path_ ) )
  {
    print "The strategy file $strat_path_ does not exist in the pool: $MODELING_STIR_STRATS_DIR/$shortcode_ \n";
    exit(1);
  }

  my %files_to_install_;

  my $strat_im_path_ = `awk '{print \$2}' $strat_path_`; chomp($strat_im_path_);
  push ( @strats_im_path_ , $strat_im_path_ );
  my $strat_im_base_ = `basename $strat_im_path_`; chomp ( $strat_im_base_ );

  if ( ! ( -f $strat_im_path_ ) )
  {
    print "The strategy_im file $strat_im_path_ does not exist in the pool \n";
    exit(1);
  }

  my $tmp_strat_path_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$strat_base_."_".$query_id_;
  push ( @intermediate_files_ , $tmp_strat_path_ );
  my $remote_strat_name_ = $MODELING_REMOTE_DIR."/stir_strats/".$shortcode_."/".$strat_base_."_".$query_id_;
  $files_to_install_ { $tmp_strat_path_ } = $remote_strat_name_ ;

  my $tmp_strat_im_path_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$strat_im_base_."_".$query_id_;
  push ( @intermediate_files_ , $tmp_strat_im_path_ );
  my $remote_strat_im_name_ = $MODELING_REMOTE_DIR."/strats/".$shortcode_."/".$strat_im_base_."_".$query_id_;
  $files_to_install_ { $tmp_strat_im_path_ } = $remote_strat_im_name_ ; 
  
  `awk -vqid=$query_id_ '\$3 = qid' $strat_path_ > $tmp_strat_path_`;
  `sed -i s,$strat_im_path_,$remote_strat_im_name_, $tmp_strat_path_`;

  open ( STRATFIL , "<" , $strat_im_path_ ) or PrintStacktraceAndDie ( "Could not open $strat_im_path_\n" );
  open ( TMPSTRATFIL , ">" , $tmp_strat_im_path_ ) or PrintStacktraceAndDie ( "Could not open $tmp_strat_im_path_\n" );
  my @lines_ = <STRATFIL> ; chomp ( @lines_ );
  close STRATFIL ;

  my $stir_desc_line_ = $lines_[0];
  my @stir_desc_words_ = split ( ' ' , $stir_desc_line_ );
  my $commonparamfile_ = $stir_desc_words_ [3];
  my $commonparam_base_ = `basename $commonparamfile_`; chomp ( $commonparam_base_ );
  my $tmp_commonparamfile_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$commonparam_base_."_".$query_id_;
  push ( @intermediate_files_ , $tmp_commonparamfile_ );
  my $remote_commonparamfile_ = $MODELING_REMOTE_DIR."/params/".$shortcode_."/".$commonparam_base_."_".$query_id_;
  $files_to_install_ { $tmp_commonparamfile_ } = $remote_commonparamfile_ ;

  `cp $commonparamfile_ $tmp_commonparamfile_`;
  open ( COMMONPARAM , "<" , $commonparamfile_ ) or PrintStacktraceAndDie ( "Could not open $commonparamfile_\n");
  open ( TMPCOMMONPARAM , ">" , $tmp_commonparamfile_ ) or PrintStacktraceAndDie ( "Could not open $tmp_commonparamfile_\n");
  my @cplines_ = <COMMONPARAM>; chomp ( @cplines_ );
  close COMMONPARAM;
  foreach my $cpline_ ( @cplines_ )
  {
    my @c_words_ = split ( ' ', $cpline_ );
    if ( $#c_words_ < 0 ) {
      next;
    }
    if ( $c_words_[0] eq "PARAMVALUE" )
    {
      if ( $c_words_[1] eq "MAX_LOSS" && $max_loss_ >= 0 )
      {
        if ( $use_optimal_max_loss_ == 1)
        {
        	my $FIND_OPTIMAL_MAX_LOSS  = $MODELSCRIPTS_DIR."/find_optimal_max_loss.pl ";
        	my $exec_cmd_ = $FIND_OPTIMAL_MAX_LOSS." $shortcode_ $timeperiod_ $OML_lookback_days_ $OML_hit_ratio_ $OML_number_top_loss_ $strat_base_ $yyyymmdd_ $skip_dates_file_ 2>/dev/null";
        	my @exec_output_ = `$exec_cmd_`;
        	foreach my $max_loss_line_ ( @exec_output_ )
        	{
        		if ( index ( $max_loss_line_ , "=>" ) >= 0 || index ( $max_loss_line_ , "MAX_LOSS" ) >= 0 )
        		{
        			next;
        		}
        		my @max_loss_words_ = split ( ' ' , $max_loss_line_ );
        		if ( $#max_loss_words_ >= 2 )
        		{
        			my $optimal_max_loss_ = $max_loss_words_ [ 0 ];
        			my $num_max_loss_hits_ = $max_loss_words_ [ 2 ];
        			if ( $num_max_loss_hits_ <= $OML_lookback_days_ * $OML_hit_ratio_ ) # fist max_loss_line may not follow the hit_ratio constraint in some cases, adding a sanity check here 
        			{
        				$max_loss_  = $optimal_max_loss_ * $risk_scale_;
        				last;
        			}
        		}
        	}
        }
        $mail_body_ = $mail_body_."Combined MaxLoss: $max_loss_ $shortcode_ $strat_base_ \n";
        $mail_body_ = $mail_body_."-----------------------------------------------------------------\n";
        print "Combined MaxLoss: $max_loss_ $shortcode_ $strat_base_ \n";
        $c_words_[2] = $max_loss_;
      }
      elsif ( $c_words_[1] eq "MAX_GLOBAL_RISK" && $max_global_risk_ >= 0 )
      {
        $c_words_[2] = $max_global_risk_;
      }
      elsif ( $c_words_[1] eq "MAX_OPENTRADE_LOSS" && $max_opentrade_loss_ >= 0 )
      {
        $c_words_[2] = $max_opentrade_loss_;
      }
      elsif ( $c_words_[1] eq "STRUCTURE_FILE" && ( -f $c_words_[2] ) )
      {
        my $w_base_ = `basename $c_words_[2]`; chomp ( $w_base_ );
        my $remote_file_ = $MODELING_REMOTE_DIR."/params/".$shortcode_."/".$w_base_."_".$c_words_[1]."_".$query_id_;
        $files_to_install_ { $c_words_[2] } = $remote_file_;
        $c_words_[2] = $remote_file_;
      }
      print TMPCOMMONPARAM join ( ' ' , @c_words_ )."\n";
    }
  }
  close TMPCOMMONPARAM;

  $stir_desc_words_ [3] = $remote_commonparamfile_;
  $stir_desc_words_ [6] = $query_id_;
  print TMPSTRATFIL join( ' ' , @stir_desc_words_ )."\n";

  foreach my $line_ ( @lines_ )
  {
    my @t_words_ = split ( ' ' , $line_ );
    if ( $#t_words_ < 0 ) { next ; } 
    
    if ( $t_words_[0] eq "STRATEGYLINE" )
    {
      my $c_shc_ = $t_words_[1];
      my $param_path_ = $t_words_[3];
      my $model_path_ = $t_words_[2];
      
      my $exec_cmd_ = "$LIVE_BIN_DIR/get_contract_specs $c_shc_ $yyyymmdd_ LOTSIZE | awk '{print \$2}'";
      my $lot_size_ = `$exec_cmd_`; chomp ( $lot_size_ ) ;
      if ( not $lot_size_ ) 
      {
          $lot_size_ = 1;
      }
      
      $shc_to_min_order_size_{$c_shc_} = $lot_size_ ;
      
      open MODELLING_PARAM, " < $param_path_ " or PrintStacktraceAndDie ( " Could not open modelling param $param_path_ for reading\n" ) ;
      my @param_lines_ = <MODELLING_PARAM>;
      close MODELLING_PARAM;
      for my $param_line_ ( @param_lines_ ) 
      {
          if ( index ( $param_line_, "UNIT_TRADE_SIZE" ) >= 0 ) 
          {
              my @param_words_ = split ( ' ', $param_line_ ) ;
              if ( $#param_words_  >= 2 ) 
              {
                  $shc_to_param_uts_ {$c_shc_} = $param_words_[2];
                  last ;
              }
          }
      }
      
      if ( not exists $shc_to_uts_ {$c_shc_} and exists $shc_to_param_uts_ {$c_shc_} )
      {
          $shc_to_uts_{$c_shc_} = max ( $lot_size_,  (int (int ($uts_scale_ * $shc_to_param_uts_{$c_shc_})/$lot_size_ ) ) * $lot_size_ ) ;
      } 
      elsif ( not exists $shc_to_uts_ {$c_shc_} )
      {
          print STDERR " Could not find UTS for $c_shc_ in pick-strats config or in modelling/params \n";
          $shc_to_uts_{$c_shc_} = $lot_size_;
      }
  
      my $param_base_ = `basename $param_path_`; chomp ($param_base_);
      my $tmp_param_path_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$param_base_.".".$shc_to_uts_{$c_shc_}."_".$query_id_;
      push ( @intermediate_files_ , $tmp_param_path_ );
      my $remote_param_path_ = $MODELING_REMOTE_DIR."/params/".$c_shc_."/".$param_base_.".".$shc_to_uts_{$c_shc_}."_".$query_id_;
      $files_to_install_ {$tmp_param_path_} = $remote_param_path_;

      UpdateParam ( $query_id_ , $c_shc_ , $param_path_, $tmp_param_path_ );

      my $model_base_ = `basename $model_path_`; chomp ($model_base_);
      my $remote_model_path_ = $MODELING_REMOTE_DIR."/models/".$shortcode_."/".$model_base_."_".$query_id_;
      $files_to_install_ {$model_path_} = $remote_model_path_;

      $t_words_[2]= $remote_model_path_;
      $t_words_[3] = $remote_param_path_;
    
      print TMPSTRATFIL join( ' ' , @t_words_)."\n";
    }
  }
  close TMPSTRATFIL;

  if ( $install_ )
  {
    foreach my $local_file_ ( keys %files_to_install_ )
    {
      my $remote_file_ = $files_to_install_ {$local_file_};
      if ( $verbose_ ) { print " Installing $local_file_ : $remote_file_\n"; }
      my $file_dir_ = `dirname $remote_file_`; chomp ( $file_dir_ );
      `ssh $install_location_ -l dvctrader 'mkdir -p $file_dir_'`;
      `scp $local_file_ dvctrader\@"$install_location_":"$remote_file_"`;
    }
  }
  $mail_body_ = $mail_body_."-----------------------------------------------------------------\n";
  $mail_body_ = $mail_body_."-----------------------------------------------------------------\n";
  $id_to_remote_strat_ {$query_id_} =  $remote_strat_name_ ; 
}

sub UpdateParam  # ScaleParamFile #2338
{
  my $query_id_ = shift;
  my $shc_ = shift;
  my $param_file_ = shift;
  my $tmp_param_file_ = shift;

  my $new_uts_ = $shc_to_uts_ { $shc_ };
  my $t_strat_name_ = $id_to_strat_base_{$query_id_};
        	
  open ( PARAM_FILE_, "<", $param_file_ ) or PrintStacktraceAndDie ( "Could not open $param_file_" );
  my @param_contents_ = <PARAM_FILE_>; chomp ( @param_contents_ );
  close PARAM_FILE_;

  open ( TMP_PARAM_FILE_, ">", $tmp_param_file_ ) or PrintStacktraceAndDie ( "Could not open $tmp_param_file_" );

  my $orig_uts_ = -1;
  my @my_uts_line_ = grep ( /UNIT_TRADE_SIZE/, @param_contents_ ) ;
  @my_uts_line_ = grep ( !/FACTOR/, @my_uts_line_ );

  if ( $#my_uts_line_ == 0 ) {
    my @t_words = split( / / , $my_uts_line_[0] );
    $orig_uts_ = $t_words[2] ; chomp($orig_uts_);
  }
  else {
    print "There is either no UnitTradeSize line or multiple lines. @my_uts_line_. $#my_uts_line_\nCannot proceed!!!." ;
    exit;
  }
    
  foreach my $param_file_line_ ( @param_contents_ )
  {
    my @param_line_words_ = split ( ' ' , $param_file_line_ );

    given ( $param_line_words_ [ 1 ] )
    {
      when ( "UNIT_TRADE_SIZE" )
      {
        $param_line_words_ [ 2 ] = $new_uts_;
      }

      when ( ["MAX_POSITION", "WORST_CASE_POSITION", "MAX_OPENTRADE_LOSS", "MAX_SHORT_TERM_LOSS"] )
      {
        chomp($param_line_words_ [ 2 ]);
        $param_line_words_ [ 2 ] = ( ( $param_line_words_ [ 2 ] * $new_uts_ ) / $orig_uts_ );
      }
      when ( "MAX_LOSS")
      {
      	my $t_optimal_max_loss_ =  ( ( $param_line_words_ [ 2 ] * $new_uts_ ) / $orig_uts_ ); 
        if ( $use_optimal_max_loss_ == 1 )
        {
        	my $FIND_OPTIMAL_MAX_LOSS  = $MODELSCRIPTS_DIR."/find_optimal_max_loss.pl ";
        	
        	my $exec_cmd_ = $FIND_OPTIMAL_MAX_LOSS." $shc_ $timeperiod_ $OML_lookback_days_ $OML_hit_ratio_ $OML_number_top_loss_ $t_strat_name_ $yyyymmdd_ $skip_dates_file_ 2>/dev/null";
        	my @exec_output_ = `$exec_cmd_`;
        	foreach my $max_loss_line_ ( @exec_output_ )
        	{
        		if ( index ( $max_loss_line_ , "=>" ) >= 0 || index ( $max_loss_line_ , "MAX_LOSS" ) >= 0 )
        		{
        			next;
        		}
        		my @max_loss_words_ = split ( ' ' , $max_loss_line_ );
        		if ( $#max_loss_words_ >= 2 )
        		{
        			my $max_loss_ = $max_loss_words_ [ 0 ];
        			my $num_max_loss_hits_ = $max_loss_words_ [ 2 ];
        			if ( $num_max_loss_hits_ <= $OML_lookback_days_ * $OML_hit_ratio_ ) # fist max_loss_line may not follow the hit_ratio constraint in some cases, adding a sanity check here 
        			{
        				$t_optimal_max_loss_ = $max_loss_ * $new_uts_;
        				last;
        			}
        		}
        	}
        }
        $mail_body_ = $mail_body_.sprintf ( "Product $shc_ MaxLoss: %02f $t_strat_name_ \n", $t_optimal_max_loss_) ;
        $param_line_words_ [ 2 ] = $t_optimal_max_loss_; 
        printf ( "Product $shc_ MaxLoss: %02f $t_strat_name_ \n", $t_optimal_max_loss_) ;
        print  "Product $shc_ MaxLoss: $t_optimal_max_loss_  $t_strat_name_ \n" ;
      }
      when ( [ "ZEROPOS_LIMITS", "HIGHPOS_LIMITS" ] )
      {
        chomp($param_line_words_ [ 2 ]);
        if ( $orig_uts_ > 1 &&
            $param_line_words_ [ 2 ] >= $orig_uts_ )
        { # this is a case where the zeropos-limits is >= than the uts.
# most likely the zeropos-limits was meant to be an absolute specification.
# this also needs to be scaled when installed in production.
          $param_line_words_ [ 2 ] = ( ( $param_line_words_ [ 2 ] * $new_uts_ ) / $orig_uts_ );
        }
      }

      default
      {
      }
    }
    print TMP_PARAM_FILE_ join ( ' ' , @param_line_words_ )."\n";
  }
  close TMP_PARAM_FILE_;
}

sub InstallProductionStratList
{

    open ( LOCAL_PRODUCTION_FILE , ">>" , $local_production_strat_list_file_ ) or PrintStacktraceAndDie ( "Could not open $local_production_strat_list_file_" );

    print LOCAL_PRODUCTION_FILE "\n##################################################################################################\n";

    # Backup existing crontab on destination server.
    BackupExistingProductionInstall ( );

    FixProductionCrontab ( );

    # Sync generated crontab to prod. machine and install it.
    # Do this again , to propogate combined strategy file changes.
    if ( $install_ ) { InstallProductionCrontab ( ); }

    return;
}

sub BackupExistingProductionInstall
{
  my $backup_remote_production_strat_list_file_ = $remote_production_strat_list_file_.".".$yyyymmdd_.".".$hhmmss_;

  my $exec_cmd_ = "ssh $install_location_ -l dvctrader 'mkdir -p $REMOTE_PRODUCTION_STRAT_LIST_DIR'";
  `$exec_cmd_`;

  $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l \> $backup_remote_production_strat_list_file_'";
  `$exec_cmd_`;

  return;
}

sub ClearProductionStratList
{
  if ( ! -d $LOCAL_PRODUCTION_STRAT_LIST_DIR )
  {
    `mkdir -p $LOCAL_PRODUCTION_STRAT_LIST_DIR`;
  }

  if ( ExistsWithSize ( $local_production_strat_list_file_ ) )
  {
    PrintStacktraceAndDie ( "$local_production_strat_list_file_ already exists , proceeding could be dangerous. Please remove and re-run" );
  }

  open ( LOCAL_PRODUCTION_FILE , ">" , $local_production_strat_list_file_ ) or PrintStacktraceAndDie ( "Could not open $local_production_strat_list_file_" );

  print LOCAL_PRODUCTION_FILE "##################################################################################################\n";
# Print a tiny header into the crontab , maybe it will help
# understand this install at trading time.
  print LOCAL_PRODUCTION_FILE "## [ $shortcode_ $exchange_ ] [ ";

  foreach my $query_id_ ( keys %id_to_strat_base_ )
  {
    print LOCAL_PRODUCTION_FILE " ( ".$query_id_;
    foreach my $c_shc_ ( keys %shc_to_uts_ )
    {
      print LOCAL_PRODUCTION_FILE " ".$c_shc_.": ".$shc_to_uts_{$c_shc_};
    }
    print LOCAL_PRODUCTION_FILE " , MAXLOSS: ".$max_loss_." )";
  }


  print LOCAL_PRODUCTION_FILE "[ $email_address_ $yyyymmdd_ : $hhmmss_ ] ##\n";

  print LOCAL_PRODUCTION_FILE "##################################################################################################\n";

  close ( LOCAL_PRODUCTION_FILE );

  return;
}

sub AddToProductionStratList
{
  my ( $remote_full_path_strat_name_ , $t_install_location_ , $t_dest_id_ ) = @_;

  push ( @remote_strat_full_path_list_ , $remote_full_path_strat_name_ );

  my $ONLOAD_START_REAL_TRADING = "/home/dvctrader/LiveExec/ModelScripts/onload_start_real_trading.sh";
  my $STOP_REAL_TRADING = "/home/dvctrader/LiveExec/ModelScripts/stop_real_trading.sh";
  my $QUERYOUTPUT_FILE = "/spare/local/logs/tradelogs/queryoutput/sms.".$t_dest_id_;

  open ( LOCAL_PRODUCTION_FILE , ">>" , $local_production_strat_list_file_ ) or PrintStacktraceAndDie ( "Could not open $local_production_strat_list_file_" );

  my $onload_switch_token_ = "OFF";
  if ( $onload_trade_exec_ ) { $onload_switch_token_ = "ON"; }

  my $vma_switch_token_ = "OFF";
  if ( $vma_trade_exec_ ) { $vma_switch_token_ = "ON"; }

  my $affinity_switch_token_ = "NF";
  if ( $affinity_trade_exec_ ) { $affinity_switch_token_ = "AF"; }

  print LOCAL_PRODUCTION_FILE "\n";

# Tagging the US strat installs with "US_MORN_DAY"
# and the EU strat installs with "EU_MORN_DAY"
# This will allow EU and US to co-exist.
  my $time_period_comment_tag_ = "# ".$timeperiod_;

  print LOCAL_PRODUCTION_FILE substr ( $exec_start_hhmm_ , 2 , 2 )." ".substr ( $exec_start_hhmm_ , 0 , 2 )." * * 1-5 ".$ONLOAD_START_REAL_TRADING." ".$exchange_." ".$t_dest_id_." ".$remote_full_path_strat_name_." ".$onload_switch_token_." ".$affinity_switch_token_." ".$vma_switch_token_." ".$risk_tags_." >> ".$QUERYOUTPUT_FILE." 2>&1 ".$time_period_comment_tag_."\n";
  print LOCAL_PRODUCTION_FILE substr ( $exec_end_hhmm_ , 2 , 2 )." ".substr ( $exec_end_hhmm_ , 0 , 2 )." * * 1-5 ".$STOP_REAL_TRADING." ".$exchange_." ".$t_dest_id_." ".$time_period_comment_tag_."\n";

  close ( LOCAL_PRODUCTION_FILE );

  return;
}

sub FixProductionCrontab
{
  my $timeperiod_prefix_ = substr ( $timeperiod_ , 0 , 2 );

# Only remove installs for same timeperiod.
  my $time_period_comment_tag_ = "# ".$timeperiod_;

  my $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l | grep $shortcode_ | grep start_real_trading | grep $exchange_ | grep \"$time_period_comment_tag_\"'";
  my @existing_queries_ = `$exec_cmd_`; chomp ( @existing_queries_ );

  $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l | grep stop_real_trading | grep $exchange_ | grep \"$time_period_comment_tag_\"'";
  my @existing_stops_ = `$exec_cmd_`; chomp ( @existing_stops_ );

# Match starts with stops , so we don't end up messing with
# cron jobs for different products.
  my %start_query_ids_ = ( );
  my %is_valid_stop_id_ = ( );
  
  my @shc_filtered_existing_queries_ = ();
  foreach my $existing_query_line_ ( @existing_queries_ )
  {
    my @existing_query_words_ = split ( ' ' , $existing_query_line_ );
    my $t_query_id_ = 0;

    for ( my $i = 0 ; $i <= $#existing_query_words_ ; $i ++ )
    {
      if ( $existing_query_words_ [ $i ] eq $exchange_ and 
      index ( $existing_query_words_[$i+2], $shortcode_ ) >= 0 ) #check for shortcode too
      {
          push ( @shc_filtered_existing_queries_, $existing_query_line_ ) ;
        $t_query_id_ = $existing_query_words_ [ $i + 1 ];
        last;
      }
    }

    $start_query_ids_ { $t_query_id_ } = 1;
  }
  
  @existing_queries_ = @shc_filtered_existing_queries_;
  
  my @stop_lines_to_remove_ = ( );

  foreach my $stop_query_line_ ( @existing_stops_ )
  {
    my @existing_stop_words_ = split ( ' ' , $stop_query_line_ );
    my $t_query_id_ = 0;

    for ( my $i = 0 ; $i <= $#existing_stop_words_ ; $i ++ )
    {
      if ( $existing_stop_words_ [ $i ] eq $exchange_ )
      {
        $t_query_id_ = $existing_stop_words_ [ $i + 1 ];
        last;
      }
    }

    if ( exists ( $start_query_ids_ { $t_query_id_ } ) )
    {
# There isn't a start corresponding to this query id that matches
# the shortcode-exchange pair we are about to install.
# This is either a stray stop or a stop for a different shortcode and / or exchange.
# Preserve it.
      push ( @stop_lines_to_remove_ , $stop_query_line_ );
    }
  }

# Need to remove all starts for this shortcode and exchange ( all in  @existing_queries_ )
# Need to remove only the stops which were for this shortcode & exchange ( all in @stop_lines_to_remove_ )

  $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l'";
  my @crontab_lines_ = `$exec_cmd_`; chomp ( @crontab_lines_ );

  open ( LOCAL_PRODUCTION_FILE , "<" , $local_production_strat_list_file_ ) or PrintStacktraceAndDie ( "Could not open $local_production_strat_list_file_" );
  my @local_production_lines_ = <LOCAL_PRODUCTION_FILE>;
  close ( LOCAL_PRODUCTION_FILE );

  open ( LOCAL_PRODUCTION_FILE , ">" , $local_production_strat_list_file_ ) or PrintStacktraceAndDie ( "Could not open $local_production_strat_list_file_" );

# Skip lines from a pervious install.
  my @lines_to_skip_ = ( );
  push ( @lines_to_skip_ , "################################################################################################" );
  push ( @lines_to_skip_ , "## [ $shortcode_ $exchange_ ] [" );

  foreach my $crontab_line_ ( @crontab_lines_ )
  {
    if ( FindItemFromVec ( $crontab_line_ , @existing_queries_ ) ||
        FindItemFromVec ( $crontab_line_ , @stop_lines_to_remove_  ) )
    {
      next;
    }

    my $to_skip_ = 0;
    foreach my $t_line_to_skip_ ( @lines_to_skip_ )
    {
      if ( index ( $crontab_line_ , $t_line_to_skip_ ) >= 0 )
      {
        $to_skip_ = 1;
      }
    }
    if ( $to_skip_ )
    {
      next;
    }

    chomp ( $crontab_line_ );

    if ( $crontab_line_ )
    {
      print LOCAL_PRODUCTION_FILE $crontab_line_."\n\n";
    }
  }

# Now append the generated crontab for this shortcode.
  print LOCAL_PRODUCTION_FILE @local_production_lines_;

  close ( LOCAL_PRODUCTION_FILE );

  return;
}

sub InstallProductionCrontab
{
  my $exec_cmd_ = "rsync -avz --delete $local_production_strat_list_file_ dvctrader\@$install_location_:$remote_production_strat_list_file_";
  `$exec_cmd_`;

  $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab $remote_production_strat_list_file_'";
  `$exec_cmd_`;

  return;
}

sub RemoveIntermediateFiles
{
  foreach my $intermediate_file_ ( @intermediate_files_ )
  {
    if ( $delete_intermediate_files_ ) 
    {
      my $exec_cmd_ = "rm -f $intermediate_file_";
#       print $exec_cmd_."\n";
      `$exec_cmd_`;
    }
    elsif ( $verbose_ ) {
      print $intermediate_file_."\n";
    }
  }

  return;
}

sub SendReportMail
{
  if ( ! $email_address_ || ! $send_mail_ )
  {
    print STDOUT $mail_body_;

    return;
  }
  else
  {
    if ( $email_address_ && $mail_body_ )
    {
      open ( MAIL , "|/usr/sbin/sendmail -t" );

      print MAIL "To: $email_address_\n";
      print MAIL "From: $email_address_\n";
      print MAIL "Subject: pick_strats_and_install ( $config_file_ ) $yyyymmdd_ $hhmmss_ \n\n";

      print MAIL $mail_body_ ;

      close(MAIL);
    }
  }
}

sub signal_handler
{
  die "Caught a signal $!\n";
}

sub END
{
  if($is_remote_lock_created_ && $install_location_ ne "")
  {
    `ssh $install_location_ "rm -f $remote_lockfile_"`;
    print "removed lock  $install_location_:$remote_lockfile_\n";
  }
}

