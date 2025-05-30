#!/usr/bin/perl

# \file ModelScripts/pick_strats_and_install.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#	   Suite No 162, Evoma, #14, Bhattarhalli,
#	   Old Madras Road, Near Garden City College,
#	   KR Puram, Bangalore 560049, India
#	   +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min sum/; # for max
use List::MoreUtils qw(uniq); # for uniq
use Mojolicious::Lite;
use Mojo::JSON qw(decode_json encode_json);
use Math::Complex; # sqrt
use FileHandle;
use Scalar::Util qw(looks_like_number);
use POSIX;
use Term::ANSIColor;
use sigtrap qw(handler signal_handler normal-signals error-signals);

use Term::ANSIColor 1.04 qw(uncolor);
my $names = uncolor('01;31');

sub LoadMasterStratFileMRT;
sub MaxLossSanityCheck;
sub GetRiskAllocScriptOutput;
sub GetShortcodePair;
sub EditPickStratConfig;
sub EditPickStratConfigWrapper;
sub GetShortcodeStratMapping;
sub SendErrorMailifAutomatedAndDie;
sub PrintErrorAndDie;
sub SendReportMail;

my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $PICKSTRATS_DIR = "/spare/local/pickstrats_logs";
my $PICKSTRAT_TEMP_DIR = $PICKSTRATS_DIR."/temp_dir";
my $CONFIGLIST_DIR = $HOME_DIR."/modelling/pick_strats_config/MRT";
my $STRATLIST_DIR = $HOME_DIR."/modelling/mrt_strats";
my $RISKALLOC_OUTPUT_ = "/spare/local/MeanRevertPort/DailyPort/MRT_Portfolio_";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
`mkdir -p $PICKSTRAT_TEMP_DIR`;

my $USAGE="$0 STRATLIST [pick_strats_date]";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $stratlist_file_ = $ARGV [ 0 ];
my $pickstrats_date_ = `date +%Y%m%d`; chomp ( $pickstrats_date_ );
if ( $#ARGV > 0 ) { $pickstrats_date_ = $ARGV [ 1 ]; }
my $curr_hh_ = `date +%H%M`; chomp ( $curr_hh_ );
if ( $curr_hh_ >= 2200 )
{
  $pickstrats_date_ = CalcNextDate ( $pickstrats_date_ );
}
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );
my $bounds_algo_ = "2";
my $round_off_algo_ = "1";
my $email_address_ = "";

#my $max_loss_pnl_cutoff_ = -1;
my $min_max_loss_per_strat_ = -1;
my $max_max_loss_per_strat_ = -1;

my %strat_name_to_optimal_max_loss_ = ();
my %strat_name_to_uts_ = ();
my %shortcode_to_strat_ = ();

my $mail_body_ = "";
my $final_strats_mail_body_ = "";

LoadMasterStratFileMRT($stratlist_file_);
GetRiskAllocScriptOutput();
GetShortcodeStratMapping();
EditPickStratConfigWrapper();
SendReportMail();

sub LoadMasterStratFileMRT
{
  print "Loading Config File ...\n";

  my ( $t_stratlist_file_ ) = @_;

  open ( CONFIG_FILE, "<", $t_stratlist_file_ ) or PrintStacktraceAndDie ( "Could not open config file $t_stratlist_file_" );
  my @stratlist_file_lines_ = <CONFIG_FILE>;
  chomp ( @stratlist_file_lines_ );
  close ( CONFIG_FILE );

  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  $mail_body_ = $mail_body_." > CONFIG_FILE=".$t_stratlist_file_."\n";

  my $current_param_ = "";

  foreach my $stratlist_file_lines_ ( @stratlist_file_lines_ )
  {
    if ( index ( $stratlist_file_lines_ , "#" ) == 0 ) # not ignoring lines with # not at the beginning
    {
      next;
    }

    my @t_words_ = split ( ' ' , $stratlist_file_lines_ );

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
        when ( "ROUND_OFF_ALGO" )
        {
          $round_off_algo_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > ROUND_OFF_ALGO=".$t_words_ [ 0 ]."\n";
        }
        when ( "BOUNDS_ALGO" )
        {
          $bounds_algo_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > BOUNDS_ALGO=".$t_words_ [ 0 ]."\n";
        }
#        when ( "MAX_LOSS_PNL_CUTOFF" )
#        {
#          $max_loss_pnl_cutoff_ = $t_words_ [ 0 ];
#          $mail_body_ = $mail_body_." \t > MAX_LOSS_PNL_CUTOFF=".$t_words_ [ 0 ]."\n";
#        }
        when ( "MIN_MAX_LOSS_PER_STRAT" )
        {
          $min_max_loss_per_strat_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > MIN_MAX_LOSS_PER_STRAT=".$min_max_loss_per_strat_."\n";
        }
        when ( "MAX_MAX_LOSS_PER_STRAT" )
        {
          $max_max_loss_per_strat_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > MAX_MAX_LOSS_PER_STRAT=".$max_max_loss_per_strat_."\n";
        }
        when ( "DATE" )
        {
          my $t_pickstrats_date_ = $t_words_ [ 0 ];
          if ( $t_pickstrats_date_ ne "TODAY" ) {
            $pickstrats_date_ = GetIsoDateFromStrMin1 ( $t_pickstrats_date_ );
          }
        }
        when ( "EMAIL_ADDRESS" )
        {
          $email_address_ = $t_words_ [ 0 ];
        }
      }
    }
  }
  print "Config File Done\n"
}

sub MaxLossSanityCheck
{
  my ( $t_strat_ ) = @_;
  my $max_loss_ = $strat_name_to_optimal_max_loss_{$t_strat_};
#      my $avg_pnl_ = $max_loss_words_ [ 1 ];
#      my $t_max_loss_by_pnl_ = int($max_loss_/$avg_pnl_);
#
#      if ( $max_loss_pnl_cutoff_ > 0 &&
#         ( $avg_pnl_ <= 0 || $t_max_loss_by_pnl_ > $max_loss_pnl_cutoff_ ) )
#      {
#        $t_error_str_ = $t_error_str_."FAILED MAX LOSS PNL CUTOFF CHECK $t_max_loss_by_pnl_ > $max_loss_pnl_cutoff_\n";
#        next;
#      }

  if ( $max_loss_ > 1.2 * $max_max_loss_per_strat_ )
  {
    print "WARNING: FAILED MAX MAX LOSS PER STRAT CHECK $max_loss_ > 1.2 * $max_max_loss_per_strat_ for $t_strat_\n";
  }

  if ( $max_loss_ < 0.8 * $min_max_loss_per_strat_ )
  {
    print "WARNING: FAILED MIN MAX LOSS PER STRAT CHECK $max_loss_ < 0.8 * $min_max_loss_per_strat_ for $t_strat_\n";
  }
}

sub GetRiskAllocScriptOutput
{
  $RISKALLOC_OUTPUT_ = $RISKALLOC_OUTPUT_.$round_off_algo_."_".$bounds_algo_."_".$pickstrats_date_;
  open ( CONFIG_FILE, "<", $RISKALLOC_OUTPUT_ ) or PrintStacktraceAndDie ( "Could not open config file $RISKALLOC_OUTPUT_" );
  my @output_ = <CONFIG_FILE>; chomp ( @output_ );
  close ( CONFIG_FILE );

  foreach my $line_ ( @output_ )
  {
    my @t_words_ = split ( ',' , $line_ );
    if ($t_words_[0] eq "configid")
    {
      next;
    }
    my $t_strat_ = $t_words_[1];
    $strat_name_to_uts_{$t_strat_} = $t_words_[2];
    $strat_name_to_optimal_max_loss_{$t_strat_} = $t_words_[4];
    MaxLossSanityCheck($t_strat_);
  }
}

sub GetShortcodePair
{
  my ( $strat_ ) = @_;
  my $PRINT_STRAT_NAME_FROM_BASE = $SCRIPTS_DIR."/print_strat_from_base.sh";

  my $exec_cmd_ = $PRINT_STRAT_NAME_FROM_BASE." $strat_";
  my @t_strat_name_ = `$exec_cmd_`; chomp ( @t_strat_name_ );

  if ( $#t_strat_name_ < 0 )
  {
    SendErrorMailifAutomatedAndDie ( "$exec_cmd_ returned ".@t_strat_name_ );
  }

  my $full_path_strat_name_ = ( split(" ", $t_strat_name_[0]) ) [ 0 ];
  my @temp_ = split('/', $full_path_strat_name_ );
  my $shortcode_ = $temp_[$#temp_-2];

  return $shortcode_ ;
}

sub GetShortcodeStratMapping
{
  foreach my $t_strat_ ( keys %strat_name_to_uts_)
  {
    my $shortcode_ = GetShortcodePair($t_strat_);
    push(@{$shortcode_to_strat_{$shortcode_}}, $t_strat_)
  }
}

sub EditPickStratConfig
{
  my ( $shortcode_ ) = @_;
  my ( $t_stratlist_file_ ) = $CONFIGLIST_DIR."/".$shortcode_.".MRT.txt";

  print "Editing Config File $t_stratlist_file_...\n";

  open ( CONFIG_FILE, "<", $t_stratlist_file_ ) or PrintStacktraceAndDie ( "Could not open config file $t_stratlist_file_" );
  my @stratlist_file_lines_ = <CONFIG_FILE>;
  chomp ( @stratlist_file_lines_ );
  close ( CONFIG_FILE );

  $final_strats_mail_body_ = $final_strats_mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  $final_strats_mail_body_ = $final_strats_mail_body_." > CONFIG_FILE=".$t_stratlist_file_."\n";

  my $current_param_ = "";
  my @new_configlist_file_lines_ = ();
  my @final_strats_for_shortcode_ = ();
  if ( exists $shortcode_to_strat_{$shortcode_})
  {
    @final_strats_for_shortcode_ = @{$shortcode_to_strat_{$shortcode_}};
  }

  foreach my $stratlist_file_lines_ ( @stratlist_file_lines_ )
  {
    if ( index ( $stratlist_file_lines_ , "#" ) == 0 ) # not ignoring lines with # not at the beginning
    {
      next;
    }

    my @t_words_ = split ( ' ' , $stratlist_file_lines_ );

    if ( $#t_words_ < 0 )
    {
      $current_param_ = "";
      push(@new_configlist_file_lines_, $current_param_);
      next;
    }

    if ( ! $current_param_ )
    {
      $current_param_ = $t_words_ [ 0 ];
      push(@new_configlist_file_lines_, $current_param_);
      given ( $current_param_ )
      {
        when ("STRATS_TO_KEEP")
        {
          foreach my $t_strat_ (@final_strats_for_shortcode_)
          {
            push ( @new_configlist_file_lines_, $t_strat_ );
            $final_strats_mail_body_ = $final_strats_mail_body_." \t > STRAT = $t_strat_ UTS = $strat_name_to_uts_{$t_strat_} MAXLOSS = $strat_name_to_optimal_max_loss_{$t_strat_}\n";
          }
        }
        when ("NUM_STRATS_TO_INSTALL")
        {
          push ( @new_configlist_file_lines_, $#final_strats_for_shortcode_ + 1 );
        }
        when ("TOTAL_SIZE_TO_RUN")
        {
          foreach my $t_strat_ (@final_strats_for_shortcode_)
          {
            push ( @new_configlist_file_lines_, $strat_name_to_uts_{$t_strat_} );
          }
        }
        when ("MAX_LOSS_PER_STRAT")
        {
          foreach my $t_strat_ (@final_strats_for_shortcode_)
          {
            push ( @new_configlist_file_lines_, $strat_name_to_optimal_max_loss_{$t_strat_} );
          }
        }
        when ("REMOTE_BASEFOLDER_PATH")
        {
          my $remote_basefolder_path_ = "/home/dvctrader/af_strats/mean_revert_strats/".$shortcode_."/";
          push ( @new_configlist_file_lines_, $remote_basefolder_path_ );
        }
      }
      next;
    }
    else
    {
      given ( $current_param_ )
      {
        when ("STRATS_TO_KEEP")
        {
          next
        }
        when ("NUM_STRATS_TO_INSTALL")
        {
          next
        }
        when ("TOTAL_SIZE_TO_RUN")
        {
          next
        }
        when ("MAX_LOSS_PER_STRAT")
        {
          next
        }
        when ("REMOTE_BASEFOLDER_PATH")
        {
          next
        }
        default
        {
            push( @new_configlist_file_lines_, $stratlist_file_lines_);
        }
      }
    }
  }

  open LISTHANDLE, "> $t_stratlist_file_" or SendErrorMailifAutomatedAndDie ( "Cannot open $t_stratlist_file_ for writing..\n" );
  print LISTHANDLE join("\n", @new_configlist_file_lines_);
  close LISTHANDLE;

  print "Config File Done\n"
}

sub EditPickStratConfigWrapper
{
  ## First find what all pairs are supposed to run from ~/modelling/mrt_strats/
  my @pairs_ = `ls $STRATLIST_DIR`; chomp @pairs_;

  foreach my $shortcode_ (@pairs_)
  {
    EditPickStratConfig($shortcode_);
  }
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
      print MAIL "Subject: MRT UTS Risk Allocation ( $stratlist_file_ ) $pickstrats_date_ $hhmmss_ \n\n";
      print MAIL $mail_body_ ;
      close(MAIL);
    }
    if ( $email_address_ && $final_strats_mail_body_ )
    {
      open ( MAIL , "|/usr/sbin/sendmail -t" );
      print MAIL "To: $email_address_\n";
      print MAIL "From: $email_address_\n";
      print MAIL "Subject: MRT Strats Running Today $pickstrats_date_ $hhmmss_ \n\n";
      print MAIL $final_strats_mail_body_ ;
      close(MAIL);
    }
  }
}

sub SendErrorMailifAutomatedAndDie
{
  my $error_string_ = "@_";
  open ( MAIL , "|/usr/sbin/sendmail -t" );
  print MAIL "To: $email_address_\n";
  print MAIL "From: $email_address_\n";
  print MAIL "Subject: Error- MRT Risk Allocation $pickstrats_date_ $hhmmss_ \n\n";
  print MAIL $error_string_ ;
  close(MAIL);

  PrintErrorAndDie ( $error_string_ );
}

sub PrintErrorAndDie
{
  my $error_string_ = "@_";

  print STDERR "\n", colored("ERROR: $error_string_", 'bold red'), "\n";

  die;
}

sub signal_handler
{
  die "Caught a signal $!\n";
}



