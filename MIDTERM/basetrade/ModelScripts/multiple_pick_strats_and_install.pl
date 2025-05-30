#!/usr/bin/perl
#
## \file ModelScripts/multiple_pick_strats_and_install.pl
##
## \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
##  Address:
##    Suite No 162, Evoma, #14, Bhattarhalli,
##    Old Madras Road, Near Garden City College,
##    KR Puram, Bangalore 560049, India
##    +91 80 4190 3551
#

use strict;
use warnings;
#use Perl::Critic;
#use warnings::unused;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use FileHandle;
use POSIX;
use sigtrap qw(handler signal_handler normal-signals error-signals);

sub LoadConfigFile;
sub SanityCheck;
sub InstallMultipleStrats;
sub RunPickStratAndDumpStrats;
sub SendReportMail;
sub ClearProductionStratList;
sub AddToProductionStratList;
sub BackupExistingProductionInstall;
sub FixProductionCrontab;
sub InstallProductionCrontab;
sub SendErrorMail;
sub SendErrorMailAndDie;
sub SendErrorMailifAutomatedAndDie;
sub CreateRunningLocks;
sub RemoveRunningLocks;
sub CreateDoneLocks;
sub MovePickedStratsToPool;
sub GetOmixFileFromStrat;
sub MakeStratsFiles;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $BASETRADE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
my $LOCAL_PRODUCTION_STRAT_LIST_DIR = $HOME_DIR."/production_strat_list";
my $REMOTE_PRODUCTION_STRAT_LIST_DIR = "/home/dvctrader/production_strat_list";
my $lock_manager_script = $HOME_DIR."/".$REPO."/scripts/pick_strats_lock.sh";

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/install_strategy_production.pl"; # CopyFileCreateDir
require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

my $USAGE="$0 CONFIGFILE TIMEPERIOD";


if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $mail_body_ = "";

my $global_config_file_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1  ];

my $local_production_strat_combined_list_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$timeperiod_.".".basename ( $global_config_file_ )."_combined_strat_file_"; 
my $local_production_strat_list_file_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$timeperiod_.".".basename ( $global_config_file_ )."_strat_list_file_"; 
my $remote_production_strat_list_file_ = $REMOTE_PRODUCTION_STRAT_LIST_DIR."/".$timeperiod_.".".basename ( $global_config_file_ )."strat_list_file";
my $exchange_ = "";
my $strat_name_ = "";
my @config_files_ = ();
my @shortcodes_ = ();
my $install_location_ = "";
my $exec_start_hhmm_ = 0;
my $exec_end_hhmm_ = 0;
my $onload_trade_exec_ = 0;
my $vma_trade_exec_ = 0;
my $affinity_trade_exec_ = 0;
my $num_strats_file_ = 0;
my $use_combined_across_prods_ = 0;

my %shc_to_size_to_run_ = ( );
my %shc_to_notional_size_to_run_ = ( );
my %shc_to_num_strats_ = ( );

my $prod_query_start_id_ = "";
my $email_address_ = "";

my $pickstrats_date_ = `date +%Y%m%d`; chomp ( $pickstrats_date_ );
my $curr_hh_ = `date +%H%M`; chomp ( $curr_hh_ );
if ( $curr_hh_ >= 2200 )
{
    $pickstrats_date_ = CalcNextDate ( $pickstrats_date_ );
}
my $curr_date_ = $pickstrats_date_;
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

my $remote_lockfile_ = "/home/dvctrader/pick_strat_lock";
my $is_remote_lock_created_ = 0;
my @lock_keys_ = ( );

LoadConfigFile ( $global_config_file_ );

SanityCheck ( );

InstallCombinedProducts ( );  

SendReportMail ( );

#Installed successfully => Mark this query_start_id as done for today (to avoid multiple installs for same id)
CreateDoneLocks ( );


sub LoadConfigFile
{
  print "Loading Config File ...\n";

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
        when ( "USE_COMBINED_ACROSS_PRODS" )
        {
          $use_combined_across_prods_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > USE_COMBINED_ACROSS_PRODS=".$use_combined_across_prods_."\n";
        }
        when ( "COMMON_STRAT_NAME" )
        {
          $strat_name_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > NAME=".$strat_name_."\n";
        }
        when ( "EXCHANGE" )
        {
          $exchange_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > EXCHANGE=".$exchange_."\n";
        }
        when ( "CONFIG_FILES" )
        {
          if( ($#t_words_ >= 1) )
          {
            push ( @config_files_ , $t_words_ [ 1 ] );
            push ( @shortcodes_ , $t_words_ [ 0 ] );
            $mail_body_ = $mail_body_." \t > SHORTCODE = ".$t_words_[ 0 ]." CONFIG = ".$t_words_ [ 1 ]."\n";
          }
        }
        when ( "EMAIL_ADDRESS" )
        {
          $email_address_ = $t_words_ [ 0 ];
        }

        when ( "INSTALL_LOCATION" )
        {
          $install_location_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > INSTALL_PICKED_STRATS=".$install_location_."\n";     
        }

        when ( "PROD_QUERY_START_ID" )
        {
          $prod_query_start_id_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > PROD_QUERY_START_ID=".$prod_query_start_id_."\n";
        }

        when ( "SIZES_TO_RUN" )
        {
          if( ($#t_words_ >= 1) )
          {
            $shc_to_size_to_run_ { $t_words_ [ 0 ] } = $t_words_ [ 1 ];
            $mail_body_ = $mail_body_." \t > PROD_QUERY_START_ID = ".$t_words_ [ 0 ].": ".$t_words_ [ 1 ]."\n";
          }
        }

        when ( "NOTIONAL_SIZES_TO_RUN" )
        {
          if( ($#t_words_ >= 1) )
          {
            $shc_to_notional_size_to_run_ { $t_words_ [ 0 ] } = $t_words_ [ 1 ];
            $mail_body_ = $mail_body_." \t > PROD_QUERY_START_ID = ".$t_words_ [ 0 ].": ".$t_words_ [ 1 ]."\n";
          }
        }


        when ( "NUM_STRATS_TO_RUN" )
        {
          if( ($#t_words_ >= 1) )
          {
            $shc_to_num_strats_ { $t_words_ [ 0 ] } = $t_words_ [ 1 ];
            $mail_body_ = $mail_body_." \t > PROD_QUERY_START_ID = ".$t_words_ [ 0 ].": ".$t_words_ [ 1 ]."\n";
          }
        }

        when ( "DATE" )
        {
          my $t_pickstrats_date_ = $t_words_ [ 0 ];
          if ( $t_pickstrats_date_ ne "TODAY" ) {
            $pickstrats_date_ = GetIsoDateFromStrMin1 ( $t_pickstrats_date_ );
          }
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
        when ( "EXEC_START_HHMM" )
        {
          $exec_start_hhmm_ = GetUTCHHMMStr ( $t_words_ [ 0 ], $pickstrats_date_ ); 
          if ( $exec_start_hhmm_ < 10 )
          { 
# 0007 -> 7 , causing substrs to fail when installing cron.
            $exec_start_hhmm_ = "000".$exec_start_hhmm_;
          }
          elsif ( $exec_start_hhmm_ < 100 )
          { 
# 0010 -> 10 , causing substrs to fail when installing cron.
            $exec_start_hhmm_ = "00".$exec_start_hhmm_;
          }
          elsif ( $exec_start_hhmm_ < 1000 )
          { 
# 0710 -> 710 , causing substrs to fail when installing cron.
            $exec_start_hhmm_ = "0".$exec_start_hhmm_;
          }
          $mail_body_ = $mail_body_." \t > EXEC_START_HHMM=".$exec_start_hhmm_."\n";  
        }

        when ( "EXEC_END_HHMM" )
        {
          $exec_end_hhmm_ = GetUTCHHMMStr ( $t_words_ [ 0 ], $pickstrats_date_ );
          if ( $exec_end_hhmm_ < 10 )
          { 
# 0710 -> 710 , causing substrs to fail when installing cron.
            $exec_end_hhmm_ = "000".$exec_end_hhmm_;
          }
          elsif ( $exec_end_hhmm_ < 100 )
          { 
# 0710 -> 710 , causing substrs to fail when installing cron.
            $exec_end_hhmm_ = "00".$exec_end_hhmm_;
          }
          elsif ( $exec_end_hhmm_ < 1000 )
          { 
# 0710 -> 710 , causing substrs to fail when installing cron.
            $exec_end_hhmm_ = "0".$exec_end_hhmm_;
          }
          $mail_body_ = $mail_body_." \t > EXEC_END_HHMM=".$exec_end_hhmm_."\n";  
        }
      }
    }
  }
  $mail_body_ = $mail_body_." \t > DATE=".$pickstrats_date_."\n";

  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  
  return;
}

sub SanityCheck
{
  print "Sanity Checking ...\n";

  if ( $use_combined_across_prods_ ) {
    if ( $strat_name_ eq "" ) {
      PrintErrorAndDie ( "USE_COMBINED_ACROSS_PRODS is TRUE but Combined Product Name not provided" );
    }
    if ( $prod_query_start_id_ eq "" ) {
      PrintErrorAndDie ( "USE_COMBINED_ACROSS_PRODS is TRUE but PROD_QUERY_START_ID not provided" );
    }
  }

  for ( my $t_picked_ = 0 ; $t_picked_ <= $#shortcodes_ ; $t_picked_ ++ ) 
  { 
    my $t_config_file_ = $config_files_ [ $t_picked_];
    my $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$t_config_file_." INSTALL_LOCATION";
    my $t_install_location_ = `$exec_cmd_`; chomp ( $t_install_location_ );
    if($t_install_location_ ne $install_location_)
    {
      PrintErrorAndDie ( "Install location in $t_config_file_ does not matches with the current install location : $install_location_\n" );
    }
  } 

  return;
}


sub InstallCombinedProducts
{
  print "Installing ...\n"; 
  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n"; 
  $mail_body_ = $mail_body_." # InstallStrats\n\n"; 
 
  $mail_body_ = $mail_body_."\n\n\t INSTALLED :\n"; 
 
  if ( $use_combined_across_prods_ ) { 
#create running locks (to avoid multiple simultaneous runs across servers)
    CreateRunningLocks ( );

    ClearProductionStratList ( ); 
  }
 
  for ( my $t_picked_ = 0 ; $t_picked_ <= $#shortcodes_ ; $t_picked_ ++ ) 
  { 
    my $t_shortcode_ = $shortcodes_ [ $t_picked_ ];
    my $t_config_file_ = $config_files_ [ $t_picked_];
    RunPickStratAndDumpStrats ( $t_shortcode_, $t_config_file_ ,$timeperiod_  );
  } 

  if ( $use_combined_across_prods_ ) {
# Need to maintain a list of all query ids , should we need to combine 
# multiple strat lines into a single strat file. 
    MakeStratsFiles ( );

    {
      my $create_lock_numtrials_ = 10;
      my $create_lock_timeout_secs_ = 30;

      my $lock_exists_ = `ssh $install_location_ "if [ -f $remote_lockfile_ ]; then echo 1; else echo 0; fi"`; chomp($lock_exists_);

      while ( $lock_exists_ && $create_lock_numtrials_ > 0 ) {
        print "lockfile $remote_lockfile_ exists at $install_location_..\n Retrying after $create_lock_timeout_secs_ seconds..\n";
        sleep $create_lock_timeout_secs_;

        $lock_exists_ = `ssh $install_location_ "if [ -f $remote_lockfile_ ]; then echo 1; else echo 0; fi"`; chomp($lock_exists_);
        $create_lock_numtrials_--;
      }

      if ( $lock_exists_ ) {
        SendErrorMailAndDie("lockfile $remote_lockfile_ exists at $install_location_\nEither already a pick_strats is running or 
            somebody messed up in last pick_strats for this server.\n" );
      }

      print "creating lock $install_location_:$remote_lockfile_. Make sure it is deleted after the run.\n";
      `ssh $install_location_ "touch $remote_lockfile_"`;
      $is_remote_lock_created_ = 1;
    }

    for ( my $t_picked_ = 0 ; $t_picked_ < $num_strats_file_ ; $t_picked_ ++ ) 
    { 
      my $full_strat_name_ = $local_production_strat_combined_list_."".$t_picked_."_";
      AddToProductionStratList ( $strat_name_ , $install_location_ , $prod_query_start_id_ + $t_picked_ , $full_strat_name_ ); 
    } 

    BackupExistingProductionInstall ( );

    FixProductionCrontab ( );

    InstallProductionCrontab ( );

    `rm $local_production_strat_list_file_`;
  }

  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n"; 
  return; 
}

sub RunPickStratAndDumpStrats
{
  my ( $t_shortcode_, $t_config_file_, $t_timeperiod_ ) = @_;

  if ( exists $shc_to_size_to_run_{ $t_shortcode_ } ) {
    my $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$t_config_file_." TOTAL_SIZE_TO_RUN ".$shc_to_size_to_run_{ $t_shortcode_ };
    print $exec_cmd_."\n";
    `$exec_cmd_`;
  }

  if ( exists $shc_to_notional_size_to_run_{ $t_shortcode_ } ) {
    my $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$t_config_file_." TOTAL_NOTIONAL_SIZE_TO_RUN ".$shc_to_notional_size_to_run_{ $t_shortcode_ };
    print $exec_cmd_."\n";
    `$exec_cmd_`;
  }

  if ( exists $shc_to_num_strats_{ $t_shortcode_ } ) {
    my $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$t_config_file_." NUM_STRATS_TO_INSTALL ".$shc_to_num_strats_{ $t_shortcode_ };
    print $exec_cmd_."\n";
    `$exec_cmd_`;
  }

  if ( ! $use_combined_across_prods_ ) {
    my $exec_cmd_ = $MODELSCRIPTS_DIR."/pick_strats_and_install.pl ".$t_shortcode_." ".$t_timeperiod_." ".$t_config_file_." 0 0";
    print $exec_cmd_."\n";
    my $exec_output_ = `$exec_cmd_`; chomp ( $exec_output_ );
  }
  else {
    my $exec_cmd_ = $MODELSCRIPTS_DIR."/pick_strats_and_install.pl ".$t_shortcode_." ".$t_timeperiod_." ".$t_config_file_." 0 0 0 0 ";
    print $exec_cmd_."\n";
    my @exec_out_vec_ = `$exec_cmd_`; chomp ( @exec_out_vec_ );
#print join("\n", @exec_out_vec_)."\n";
    @exec_out_vec_ = grep {$_ =~ /SINGLE_STRAT_FILE_NAME/ } @exec_out_vec_;

    if ( $#exec_out_vec_ >= 0 && $exec_out_vec_[0] eq "" ) {
      SendErrorMailAndDie ( "Pickstrats for $t_config_file_ could not run successfully.." );
    }

    my @tokens_ = split ' ', $exec_out_vec_[0] ;

    my $local_single_strat_file_ = $tokens_[1];
    $exec_cmd_ = "cat ".$local_single_strat_file_." >> ".$local_production_strat_combined_list_;
    print $exec_cmd_."\n";
    `$exec_cmd_`;
    $exec_cmd_ = "rm ".$local_single_strat_file_;
    `$exec_cmd_`;
  }
  return;  
}

sub SendReportMail
{
  if ( ! $email_address_ )
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
      print MAIL "Subject: pick_strats_and_install ( $global_config_file_ ) $pickstrats_date_ $hhmmss_ \n\n";
      print MAIL $mail_body_ ;
      close(MAIL);
    }
  }
}

sub ClearProductionStratList
{
  if ( ! -d $LOCAL_PRODUCTION_STRAT_LIST_DIR )
  {
    `mkdir -p $LOCAL_PRODUCTION_STRAT_LIST_DIR`;
  }
 
  if ( ExistsWithSize ( $local_production_strat_list_file_ ) )
  {
    my $remove_cmd_ = "rm ".$local_production_strat_list_file_;
    `$remove_cmd_`;
  }

  if ( glob ( "$local_production_strat_combined_list_*" ) )
  {
    my $remove_cmd_ = "rm ".$local_production_strat_combined_list_."*";
    `$remove_cmd_`;
  }
 
  open ( LOCAL_PRODUCTION_FILE , ">" , $local_production_strat_list_file_ ) or SendErrorMailifAutomatedAndDie ( "Could not open $local_production_strat_list_file_" );
  print LOCAL_PRODUCTION_FILE "##################################################################################################\n";
# Print a tiny header into the crontab , maybe it will help
# understand this install at trading time.
  print LOCAL_PRODUCTION_FILE "### [ Combined Strategy $exchange_ ]";
  print LOCAL_PRODUCTION_FILE "[ $email_address_ $pickstrats_date_ : $hhmmss_ ] ##\n";
  print LOCAL_PRODUCTION_FILE "##################################################################################################\n";
  close ( LOCAL_PRODUCTION_FILE );
  return;
}

sub AddToProductionStratList
{
  my ( $t_strat_name_ , $t_install_location_ , $t_dest_id_ , $full_strat_name_) = @_;

  my $remote_full_path_strat_name_ = "/home/dvctrader/LiveModels/strats/Combined/".$timeperiod_."/".$t_strat_name_;

  $remote_full_path_strat_name_ = CopyFileCreateDir ( $full_strat_name_, $remote_full_path_strat_name_, "dvctrader", $t_install_location_ );

  my $exec_cmd_ = "rm ".$full_strat_name_ ;
  `$exec_cmd_`;

  my $ONLOAD_START_REAL_TRADING = "/home/dvctrader/LiveExec/ModelScripts/onload_start_real_trading.sh";
  my $STOP_REAL_TRADING = "/home/dvctrader/LiveExec/ModelScripts/stop_real_trading.sh";
  my $QUERYOUTPUT_FILE = "/spare/local/logs/tradelogs/queryoutput/sms.".$t_dest_id_;

  open ( LOCAL_PRODUCTION_FILE , ">>" , $local_production_strat_list_file_ ) or SendErrorMailifAutomatedAndDie ( "Could not open $local_production_strat_list_file_" );

  my $onload_switch_token_ = "OFF";

  my $vma_switch_token_ = "OFF";
  if ( $vma_trade_exec_ ) { $vma_switch_token_ = "ON"; }

  my $affinity_switch_token_ = "NF";
  if ( $affinity_trade_exec_ ) { $affinity_switch_token_ = "AF"; }

  print LOCAL_PRODUCTION_FILE "\n";

# Tagging the US strat instlocal_production_strat_list_file_alls with "US_MORN_DAY"
# and the EU strat installs with "EU_MORN_DAY"
# This will allow EU and US to co-exist.
  my $time_period_comment_tag_ = "# ".$timeperiod_;

  print LOCAL_PRODUCTION_FILE substr ( $exec_start_hhmm_ , 2 , 2 )." ".substr ( $exec_start_hhmm_ , 0 , 2 )." * * 1-5 ".$ONLOAD_START_REAL_TRADING." ".$exchange_." ".$t_dest_id_." ".$remote_full_path_strat_name_." ".$onload_switch_token_." ".$affinity_switch_token_." ".$vma_switch_token_." >> ".$QUERYOUTPUT_FILE." 2>&1 ".$time_period_comment_tag_."\n";
  print LOCAL_PRODUCTION_FILE substr ( $exec_end_hhmm_ , 2 , 2 )." ".substr ( $exec_end_hhmm_ , 0 , 2 )." * * 1-5 ".$STOP_REAL_TRADING." ".$exchange_." ".$t_dest_id_." 1>/dev/null 2>/dev/null $time_period_comment_tag_\n";

  close ( LOCAL_PRODUCTION_FILE );

  return;
}


sub BackupExistingProductionInstall
{
  my $backup_remote_production_strat_list_file_ = $remote_production_strat_list_file_.".".$pickstrats_date_.".".$hhmmss_;
  my $exec_cmd_ = "ssh $install_location_ -l dvctrader 'mkdir -p $REMOTE_PRODUCTION_STRAT_LIST_DIR'";
  `$exec_cmd_`;
  $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l \> $backup_remote_production_strat_list_file_'";
  `$exec_cmd_`;
  return;
}

sub FixProductionCrontab
{
  my $timeperiod_prefix_ = substr ( $timeperiod_ , 0 , 2 );

# Only remove installs for same timeperiod.
  my $time_period_comment_tag_ = "# ".$timeperiod_;

  my $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l | grep $strat_name_ | grep start_real_trading | grep $exchange_ | grep \"$time_period_comment_tag_\"'";
  my @existing_queries_ = `$exec_cmd_`; chomp ( @existing_queries_ );

  $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l | grep stop_real_trading | grep $exchange_ | grep \"$time_period_comment_tag_\"'";
  my @existing_stops_ = `$exec_cmd_`; chomp ( @existing_stops_ );

# Match starts with stops , so we don't end up messing with
# cron jobs for different products.
  my %start_query_ids_ = ( );
  my %is_valid_stop_id_ = ( );

  foreach my $existing_query_line_ ( @existing_queries_ )
  {
    my @existing_query_words_ = split ( ' ' , $existing_query_line_ );
    my $t_query_id_ = 0;

    for ( my $i = 0 ; $i <= $#existing_query_words_ ; $i ++ )
    {
      if ( $existing_query_words_ [ $i ] eq $exchange_ )
      {
        $t_query_id_ = $existing_query_words_ [ $i + 1 ];
        last;
      }
    }

    $start_query_ids_ { $t_query_id_ } = 1;
  }

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

  open ( LOCAL_PRODUCTION_FILE , "<" , $local_production_strat_list_file_ ) or SendErrorMailifAutomatedAndDie ( "Could not open $local_production_strat_list_file_" );
  my @local_production_lines_ = <LOCAL_PRODUCTION_FILE>;
  close ( LOCAL_PRODUCTION_FILE );

  open ( LOCAL_PRODUCTION_FILE , ">" , $local_production_strat_list_file_ ) or SendErrorMailifAutomatedAndDie ( "Could not open $local_production_strat_list_file_" );

# Skip lines from a pervious install.
  my @lines_to_skip_ = ( );
  push ( @lines_to_skip_ , "################################################################################################" );
  push ( @lines_to_skip_ , "### [ Combined Strategy $exchange_ ]" );

  foreach my $crontab_line_ ( @crontab_lines_ )
  {
    #Remove existing installs for the same query ID (required here to filter matching installs with different tags)
    if ( ( index ( $crontab_line_ , "start_real_trading" ) >= 0 ) || ( index ( $crontab_line_ , "stop_real_trading" ) >= 0 ) ) {
      my @crontab_line_words_ = split ( ' ' , $crontab_line_ );
      my $t_query_id_ = 0;

      for ( my $cron_ctr = 0 ; $cron_ctr <= $#crontab_line_words_ ; $cron_ctr ++ )
      {
        if ( $crontab_line_words_ [ $cron_ctr ] eq $exchange_ )
        {
          $t_query_id_ = $crontab_line_words_ [ $cron_ctr + 1 ];
          last;
        }
      }

      if ( ( $t_query_id_ == $prod_query_start_id_ ) ) {
        #Duplicate query id already installed => Remove this
        next;
      }
    }

    #Remove existing installs for the same shortcode and tag
    if ( FindItemFromVec ( $crontab_line_ , @existing_queries_ ) || FindItemFromVec ( $crontab_line_ , @stop_lines_to_remove_  ) )
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

sub MakeStratsFiles
{
  open ( SFILE_HANDLE , "< $local_production_strat_combined_list_ " ) or PrintStacktraceAndDie ( " Could not open file $local_production_strat_combined_list_ for reading \n" );
  my %omix_file_to_strat_name_ = ( );
  while ( my $sline_ = <SFILE_HANDLE> )
  {
    my $omix_file_  = GetOmixFileFromStrat($sline_);
    if (! (exists $omix_file_to_strat_name_{$omix_file_}  ))
    {
      my $t_strat_file_ = $local_production_strat_combined_list_."".$num_strats_file_."_";
      open (TEMP_HANDLE , " > $t_strat_file_");
      $omix_file_to_strat_name_{$omix_file_} = $t_strat_file_ ;
      $num_strats_file_ = $num_strats_file_ + 1;    
      print TEMP_HANDLE $sline_;
      close (TEMP_HANDLE);
    }
    else
    {
      my $t_strat_name_ = $omix_file_to_strat_name_{$omix_file_};
      my $exec_cmd_ = "sed -i '\$a $sline_' $t_strat_name_";
      `$exec_cmd_`;  
    }
  }
  close(SFILE_HANDLE);
  `rm $local_production_strat_combined_list_`;
}

sub GetOmixFileFromStrat 
{
  my $strat_line_ = shift;
  my @strat_words_ = split ( ' ', $strat_line_ );
  my $MFILE_ = $strat_words_[3];
  my $omix_file_ = "/spare/local/tradeinfo/OfflineInfo/offline_mix_mms.txt";

  if ( $MFILE_ )
  {
    if ( -e $MFILE_ )
    {
      open ( MFILE_HANDLE, "< $MFILE_" ) or PrintStacktraceAndDie ( " Could not open file $MFILE_ for reading \n" );
      while ( my $mline_ = <MFILE_HANDLE> )
      {
        my @words_ = split ( ' ', $mline_ );
        if ( ( $#words_ >= 1 ) &&
        ( $words_[0] eq "OFFLINEMIXMMS_FILE" ) )
        { # MODELINIT DEPBASE ZT_0 MktSizeWPrice MktSizeWPrice
          $omix_file_=$words_[1];
          last; # first line
        }
     }
     close ( MFILE_HANDLE );
    }
  }
 
  return $omix_file_;
}

sub CreateRunningLocks
{
  push ( @lock_keys_, $prod_query_start_id_."multiple" );
  push ( @lock_keys_, basename( $local_production_strat_combined_list_ ) );

  {
    #create running lock file (to avoid multiple simultaneous installs for same query id)
    #Create locks forcibly
    my $lock_mode="START";
    #Iterate over each query id
    foreach my $strat_counter ( 0..$#lock_keys_ )
    {
      my $temp_query_id_ = $lock_keys_[ $strat_counter ];
      `$lock_manager_script $temp_query_id_ $lock_mode 2>/dev/null`;
      if ( $? ne "0" && $lock_mode eq "START_FORCE" ) {
        #Could not create lock dir even after removing existing one
        PrintErrorAndDie ( "Cannot create running lock file for query id ".$temp_query_id_." even after removing the existing one. Probably permission issue. Aborting install!\n");
      }
      elsif ( $? ne "0" ) {
#Lock creation failed
        print "Cannot create running lock file for query id ".$temp_query_id_.".\n";
        print "\nLooks like a duplicate install for query id ".$temp_query_id_.". Are you sure you want to go ahead? Press Enter to continue, CTRL+C to exit installation: ";
          
        my $user_input = <STDIN>;
          
        #User didn't press CTLR+C=>decided to go ahead=>start creating locks forcefully(by removing existing ones)
        $lock_mode="START_FORCE";
        $strat_counter --; #Try this query id again (as lock creation for this failed)
      }
    }
  }
}

sub SendErrorMail
{
  my $error_string_ = "@_";
  open ( MAIL , "|/usr/sbin/sendmail -t" );
  print MAIL "To: $email_address_\n";
  print MAIL "From: $email_address_\n";
  print MAIL "Subject: Error- multiple_pick_strats_and_install ( $global_config_file_ ) $pickstrats_date_ $hhmmss_ \n\n";
  print MAIL $error_string_ ;
  close(MAIL);

  PrintStacktrace ( $error_string_ );
}

sub SendErrorMailAndDie
{
  my $error_string_ = "@_";
  open ( MAIL , "|/usr/sbin/sendmail -t" );
  print MAIL "To: $email_address_\n";
  print MAIL "From: $email_address_\n";
  print MAIL "Subject: Error- multiple_pick_strats_and_install ( $global_config_file_ ) $pickstrats_date_ $hhmmss_ \n\n";
  print MAIL $error_string_ ;
  close(MAIL);

  PrintStacktraceAndDie ( $error_string_ );
}

sub SendErrorMailifAutomatedAndDie
{
  my $error_string_ = "@_";
  open ( MAIL , "|/usr/sbin/sendmail -t" );
  print MAIL "To: $email_address_\n";
  print MAIL "From: $email_address_\n";
  print MAIL "Subject: Error- multiple_pick_strats_and_install ( $global_config_file_ ) $pickstrats_date_ $hhmmss_ \n\n";
  print MAIL $error_string_ ;
  close(MAIL);
  PrintStacktraceAndDie ( $error_string_ );
}

#remove running locks for given query ids
sub RemoveRunningLocks
{
#Remove running lock before exiting
  foreach my $strat_counter ( 0..$#lock_keys_ )
  {
    my $temp_query_id_ = $lock_keys_[ $strat_counter ];
    `$lock_manager_script $temp_query_id_ STOP 2>/dev/null`;
  }
}

#To avoid reusing same query id again on same day
sub CreateDoneLocks
{
#Installed successfully => Mark this query_start_id as done for today (to avoid multiple installs for same id)
  foreach my $strat_counter ( 0..$#lock_keys_ )
  {
    my $temp_query_id_ = $lock_keys_[ $strat_counter ];
    `$lock_manager_script $temp_query_id_ COMPLETE 2>/dev/null`;
    if ( $? ne "0" ) {
#Operation failed
      SendErrorMail( "Could not create done lock for ".$prod_query_start_id_." ; exit code: ".$? );
    }
  }
}

sub signal_handler
{
  die "Caught a signal $!\n";
  RemoveRunningLocks ( );
}

sub END
{
  RemoveRunningLocks ( );
  if ( $is_remote_lock_created_ && $install_location_ ne "" )
  {
    `ssh $install_location_ "rm -f $remote_lockfile_"`;
    print "removed lock  $install_location_:$remote_lockfile_\n";
  }
}
