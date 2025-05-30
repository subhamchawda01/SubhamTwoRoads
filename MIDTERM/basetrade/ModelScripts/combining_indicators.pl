#!/usr/bin/perl

use feature "switch";
use List::Util qw(first max maxstr min minstr reduce shuffle sum);

my $USAGE="$0 CONFIGFILE";

my $config_file_ = $ARGV[0];

my $USER = $ENV{'USER'};
my $SPARE_HOME="/spare/local/".$USER."/";
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib/";
my $exec_file_ = "~/basetrade_install/scripts/get_regdata_multiple.R";
my $combine_indicator_dir_ = $SPARE_HOME."CombineIndicators";
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $combine_indicator_dir_."/".$unique_gsm_id_."/";
my $main_log_file_ = $work_dir_."/main_log_file.txt";

my $ilist_filename_ = "ind_ilist";
my $base_price_ = "MktSizeWPrice";
my $target_price_ = "MktSizeWPrice";
my $ilist_dir_ = "dir_ilist";
my @pred_duration_ = ();
my @pred_algo_ = ();
my @filter_algo_ = ();
my $last_trading_date_ = "20151120";
my $num_prev_days_ = 5;
my $pdt_shortcode_ = "FGBM_0";
my $end_time_ = "EST_800";
my $start_time_ = "CET_900";
my $user_correlation_sign_ = 0;
my $min_increase_threshold_ = 0.3;
my $min_correlation_threshold_ = 0.03;
my $constrained_beta_ = 1;
my $datagen_string_ = "1000 0 0 0";

my $indicator_dir_ = $HOME_DIR.'/modelling/composite_indicatorwork/';

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

require "$GENPERLLIB_DIR"."get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( ! ( -d $work_dir_ ) )
{
	`mkdir -p $work_dir_` ;
}
print "Working Dir: $work_dir_ \n";
open MAINLOGFILEHANDLE, "> $main_log_file_ " or die "Could not create the main log file: $main_log_file_ \n";
print MAINLOGFILEHANDLE "LOG FILE STARTED";
sub SendMail;
sub ReadConfigFile;

if(! -e $config_file_)
{
    print "config_file_ $config_file_ does not exist\n";
    exit(0);
}

ReadConfigFile();
SanityCheckInstructionFile();

$ilist_file_path_ = $work_dir_ . $ilist_filename_;

open ILISTFILEHANDLE, "> $ilist_file_path_ " or die  "$0 Could not open ilist file : $ilist_file_path_ for writing\n" ;
open CONDTIONALVARHANDLE, "< $conditional_var_file" or die  "$0 Could not open condtional_var file : $conditional_var_file for reading\n" ;
open INDICATORONEHANDLE, "< $indicator_file_one" or die "$0 Could not open indicator 1 file : $indicator_file_one for reading\n";
open INDICATORTWOHANDLE, "< $indicator_file_two" or die "$0 Could not open indicator 2 file : $indicator_file_two for reading\n" ;


### Reading CV, Ind1, Ind2 and writing the ilist ###########

print MAINLOGFILEHANDLE "STARTING TO CREATE ILIST USING CONDITIONAL VARS, INDICATOR 1, INDICATOR 2 FILES\n";
my $num_ind1_ = 0;
my $num_ind2_ = 0;
my $num_cv_ = 0;
my $header_ilist_ = "MODELINIT DEPBASE " . $pdt_shortcode_  ." " . $base_price_ ." ". $target_price_."
MODELMATH LINEAR CHANGE
INDICATORSTART\n";
my $footer_ilist = 'INDICATOREND';
print ILISTFILEHANDLE $header_ilist_;
print MAINLOGFILEHANDLE $header_ilist_;
while ( (my $f1 = readline ( CONDTIONALVARHANDLE ) ) ) {
  if($f1){
    $f1 =~ s/^\s+|\s+$//g;
	    if ($f1 =~ /^\s*($|#)/){ 
 	           print MAINLOGFILEHANDLE "BLANK LINE ENCOUNTERED";
            }
	    else{
        $f1 =~ s/\#.*//;
		    print ILISTFILEHANDLE $f1."\n";
	    	print MAINLOGFILEHANDLE $f1."\n";
	            $num_cv_ = $num_cv_ + 1;
	    }
    }
}
while ( (my $f2 = readline( INDICATORONEHANDLE ) ) ) {
  if($f2){
    $f2 =~ s/^\s+|\s+$//g;
	    if ($f2 =~ /^\s*($|#)/){
		    print MAINLOGFILEHANDLE "BLANK LINE ENCOUNTERED";
	    }
	    else{
        $f2 =~ s/\#.*//; 
		    print ILISTFILEHANDLE $f2."\n";
		    print MAINLOGFILEHANDLE $f2."\n";
        	    $num_ind1_ = $num_ind1_ + 1;
	    }
    }
}
while ( (my $f3 = readline ( INDICATORTWOHANDLE ) ) ) {
  if($f3){
    $f3 =~ s/^\s+|\s+$//g;
	    if ($f3 =~ /^\s*($|#)/){ 
	            print MAINLOGFILEHANDLE "BLANK LINE ENCOUNTERED";
            }
	    else{
        $f3 =~ s/\#.*//;
   	    print MAINLOGFILEHANDLE $f3."\n";
		    print ILISTFILEHANDLE $f3."\n";
        	    $num_ind2_ = $num_ind2_ + 1;
	    }
    }
}
print ILISTFILEHANDLE $footer_ilist;
print MAINLOGFILEHANDLE $footer_ilist; 
close CONDITIONALVARHANDLE;
close INDICATORONEHANDLE;
close INDICATORTWOHANDLE;
close ILISTFILEHANDLE;
print MAINLOGFILEHANDLE "ILIST CREATED\n";
my $pred_duration_list = $pred_duration_[0];
for( my $i=1; $i < @pred_duration_; $i++ )
{
        $pred_duration_list = $pred_duration_list. ',' . $pred_duration_[$i];
}
my $pred_algo_list = $pred_algo_[0];
for( my $i=1; $i < @pred_algo_; $i++ )
{
        $pred_algo_list = $pred_algo_list. ',' . $pred_algo_[$i];
}
my $filter_algo_list = $filter_algo_[0];
for( my $i=1; $i < @filter_algo_; $i++ )
{
        $filter_algo_list = $filter_algo_list. ',' . $filter_algo_[$i];
}

#### Generating regdata ################
my $last = $datagen_string_. ' '. $pred_duration_list . ' '. $pred_algo_list .' '. $filter_algo_list . ' '.$work_dir_. "/". $ilist_dir_. "/";
my $generate_data_ = $exec_file_. ' ' . $pdt_shortcode_ . ' ' . $ilist_file_path_ . ' ' . $last_trading_date_ . ' ' . $num_prev_days_ . ' ' . $start_time_ . ' ' . $end_time_ . ' ' . $last; 
my $regdata_output_ = `$generate_data_`;
my $error_string_ = "Error";
if (index($regdata_output_, $error_string_) != -1) {
  my @regdata_out_arr = split('Error:', $regdata_output_);
  print "Error: $regdata_out_arr[1]";
  SendErrorMailAndDie("Error: $regdata_out_arr[1] \nWorkDir: $work_dir_");
}
print MAINLOGFILEHANDLE "REG DATA GENERATED\n";

#### Generating correaltion for the combination of all PNCs ##### 
$Rcommand_ = 'Rscript ~/basetrade/ModelScripts/combining_indicators_corr.R '. $work_dir_ . "/". $ilist_dir_ . ' '. $num_cv_ . ' ' . $num_ind1_ . ' ' . $num_ind2_. ' ' . $work_dir_ . ' '. $user_correlation_sign_ . ' ' . $indicator_file_one . ' '. $indicator_file_two . ' ' . $constrained_beta_;
my $reg_command_output_ = `$Rcommand_`;
print MAINLOGFILEHANDLE $reg_command_output_;
if (index($reg_command_output_, $error_string_) != -1) {
  my @regdata_out_arr = split('Error:', $reg_command_output_);
  print "Error: $regdata_out_arr[1]";
  SendErrorMailAndDie("Error: $regdata_out_arr[1] \nWorkDir: $work_dir_");
}
print MAINLOGFILEHANDLE "CORRELATION DATA GENERATED\n";

#### Filtering Data ##### 
my $running_date_yyyymmdd_ = GetIsoDateFromStrMin1 ( "TODAY" );
$RfilterCommand_ = 'Rscript ~/basetrade/ModelScripts/combining_indicators_filter.R '. $conditional_var_file . ' ' . $indicator_file_one . ' ' . $indicator_file_two . ' ' . $work_dir_ . ' '. $min_correlation_threshold_ . ' ' . $min_increase_threshold_ . ' ' . $running_date_yyyymmdd_ . ' ' . $last_trading_date_ . ' ' . $num_prev_days_ . ' ' . $pred_duration_list . ' ' . $pred_algo_list . ' ' . $filter_algo_list . ' '. $user_correlation_sign_ ;
system( $RfilterCommand_ );
print MAINLOGFILEHANDLE "DATA FILTERED\n";

### Mailing results ####
$email_file_ = $work_dir_ . 'email_contents';
open my $mail_body_, $email_file_ or die;
while( my $f = readline($mail_body_) ){
	$body_content = $body_content.$f;
}
SendMail($body_content);
print MAINLOGFILEHANDLE "EMAIL SENT\n";

`rm -r $work_dir_$ilist_dir_`;
`rm $email_file_`;

if ( ! ( -d $indicator_dir_ ) )
{
   print "indicator_dir_ $indicator_dir_ does not exist\n";
   exit(0);
}

$pdt_dir_ = $indicator_dir_.$pdt_shortcode_.'/';

if ( ! ( -d $pdt_dir_ ) )
{
        `mkdir -p $pdt_dir_` ;
}

$composite_indicator_file_ = $pdt_dir_.'composite_indicator_list';
$composite_input_file_ = $work_dir_.'combination_indicators';
if ( $num_prev_days_ > 90 )
{
  `cat $composite_indicator_file_ >> $composite_input_file_`;
  `mv $composite_input_file_ $composite_indicator_file_`;
}

### function SendMail
sub SendMail
{
  my ($t_mail_body_) = @_;
  if ( ( $mail_address_ ) &&
      ( $t_mail_body_ ) )
  {
    open(MAIL, "|/usr/sbin/sendmail -t");

    my $hostname_=`hostname`;
    print MAIL "To: $mail_address_\n";
    print MAIL "From: $mail_address_\n";
    print MAIL "Subject: Combine indicators for ( $config_file_ ) $hostname_\n\n";
    print MAIL $t_mail_body_ ;

    close(MAIL);

    print MAINLOGFILEHANDLE "Mail Sent to $mail_address_\n\n$t_mail_body_\n";
  }

}


sub ReadConfigFile
{
  open CONFIGFILEHANDLE, "< $config_file_ " or PrintStacktraceAndDie ( "$0 Could not open config file : $config_file_ for reading\n" );
  my $current_instruction_="";
  my $current_instruction_set_ = 0;

  while ( my $thisline_ = <CONFIGFILEHANDLE> ){
    chomp($thisline_);
    my @thisline_words_  = split(' ', $thisline_);
    if($#thisline_words_ < 0)
    {
      $current_instruction_="";
      $current_instruction_set_ = 0;
      next;
    }
    if(substr ( $thisline_words_[0], 0, 1) eq '#')  {next;}
    if($current_instruction_set_ == 0)
    {
      $current_instruction_ = $thisline_words_[0];
      $current_instruction_set_ = 1;
      next;
    }
    given( $current_instruction_ )
    {
      when("CONDITIONAL_VARIABLES")
      {
        $conditional_var_file = $thisline_words_[0];
      }
      when("INDICATOR_FILE_1")
      {
        $indicator_file_one = $thisline_words_[0];
      }
      when("INDICATOR_FILE_2")
      {
        $indicator_file_two = $thisline_words_[0];
      }
      when("PRODUCT_SHORTCODE")
      {
        $pdt_shortcode_ = $thisline_words_[0];
      }
      when("LAST_TRADING_DATE")
      {
        $last_trading_date_ = GetIsoDateFromStrMin1 ( $thisline_words_[0] );
      }
      when("NUM_PREV_DAYS")
      {
        $num_prev_days_ = $thisline_words_[0];
      }
      when("START_TIME")
      {
        $start_time_ = $thisline_words_[0];
      }
      when("END_TIME")
      {
        $end_time_ = $thisline_words_[0];
      }
      when("DATAGEN_STRING")
      {
        $datagen_string_ = $thisline_;
      }
      when("PRED_DURATION")
      {
        push ( @pred_duration_, $thisline_words_[0] );
      }
      when("PRED_ALGO")
      {
        push ( @pred_algo_, $thisline_words_[0] );
      }
      when("FILTER_ALGO")
      {
        push ( @filter_algo_, $thisline_words_[0] );
      }
      when("MAIL_ADDRESS")
      {
        if ($num_prev_days_ > 10) {
          $mail_address_ = "nseall@tworoads.co.in";
        } else {
          $mail_address_ = $thisline_words_[0];
        }
      }
      when("BASE_PRICE_TYPE")
      {
        $base_price_ = $thisline_words_[0];
      }
      when("TARGET_PRICE_TYPE")
      {
        $target_price_ = $thisline_words_[0];
      }
      when("USER_CORRELATION_SIGN")
      {
        $user_correlation_sign_ = $thisline_words_[0];
      }
      when("MIN_INCREASE_THRESHOLD")
      {
        $min_increase_threshold_ = $thisline_words_[0];
      }
      when("MIN_CORRELATION_THRESHOLD")
      {
        $min_correlation_threshold_ = $thisline_words_[0];
      }
      when("CONSTRAINED_BETA")
      {
        $constrained_beta_ = $thisline_words_[0];
      }
      default
      {
      }
    }
  }
}

sub SanityCheckInstructionFile 
{
  print MAINLOGFILEHANDLE "SanityCheckInstructionFile\n";

  if ( ! ( $pdt_shortcode_ ) )
  {
    print MAINLOGFILEHANDLE "SHORTCODE missing\n";
    SendErrorMailAndDie("SHORTCODE missing\nWorkDir: $work_dir_");
    exit ( 0 );
  }
  if ( ! -f $conditional_var_file ) {
        SendErrorMailAndDie ( "Bad Config $config_file_ No such file: $conditional_var_file \nWork Dir: $work_dir_" );
  }

  if ( ! -f $indicator_file_one ) {
        SendErrorMailAndDie ( "Bad Config $config_file_ No such file: $indicator_file_one \nWork Dir: $work_dir_" );
  }

  if ( ! -f $indicator_file_two ) {
        SendErrorMailAndDie ( "Bad Config $config_file_ No such file: $indicator_file_two \nWork Dir: $work_dir_" );
  }

  if ( ! ( $last_trading_date_ ) )
  {
    print MAINLOGFILEHANDLE "DATAGEN LAST TRADING DATE missing\n";
    SendErrorMailAndDie ("DATAGEN LAST TRADING DATE missing \nWorkDir: $work_dir_");
    exit ( 0 );
  }

  if ( ! ( ValidDate ( $last_trading_date_ ) ) )
  {
    print MAINLOGFILEHANDLE "DATAGEN LAST TRADING DATE not Valid\n";
    SendErrorMailAndDie ( "DATAGEN LAST TRADING DATE not Valid\nWorkDir: $work_dir_" );
    exit ( 0 );
  }

  if ( ! ( $start_time_ ) )
  {
    print MAINLOGFILEHANDLE "DATAGEN_START_TIME missing\n";
    SendErrorMailAndDie("DATAGEN_START_TIME missing\nWorkDir: $work_dir_");
    exit ( 0 );
  }

  if ( ! ( $end_time_ ) )
  {
    print MAINLOGFILEHANDLE "DATAGEN_END_TIME missing\n";
    SendErrorMailAndDie ("DATAGEN_END_TIME missing\nWorkDir: $work_dir_");
    exit ( 0 );
  }
  my @datagen_params = split(' ', $datagen_string_);
  if( $datagen_params[0] !~ /^\d+$/ )
  {
    print MAINLOGFILEHANDLE "ERROR : Datagen param_0 $datagen_params[0] not implemented\n";
    print "ERROR : Datagen param_0 $datagen_params[0] not implemented\n";
    SendErrorMailAndDie ("Datagen param_0 $datagen_params[0] not implemented\nWorkDir: $work_dir_");
  }
  if( $datagen_params[1] !~ /^(\d+)|(c1)|(c2)|(c3)|(e1)|0$/ )
  {
    print MAINLOGFILEHANDLE "ERROR : Datagen param_1 $datagen_params[1] not implemented\n";
    print "ERROR : Datagen param_1 $datagen_params[1] not implemented\n";
    SendErrorMailAndDie ("Datagen param_1 $datagen_params[1] not implemented\nWorkDir: $work_dir_");
  }
  if( $datagen_params[2] !~ /^(\d+)|(t1)|(ts1)|0$/ )
  {
    print MAINLOGFILEHANDLE "ERROR : Datagen param_2 $datagen_params[2] not implemented\n";
    print "ERROR : Datagen param_2 $datagen_params[2] not implemented\n";
    SendErrorMailAndDie ("Datagen param_2 $datagen_params[2] not implemented\nWorkDir: $work_dir_");
  }
  if( $datagen_params[3] !~ /0|1/ )
  {
    print MAINLOGFILEHANDLE "ERROR : Datagen param_3 $datagen_params[3] not implemented\n";
    print "ERROR : Datagen param_3 $datagen_params[3] not implemented\n";
    SendErrorMailAndDie ("Datagen param_3 $datagen_params[3] not implemented\nWorkDir: $work_dir_");
  }
  my @valid_filter_list_ = ("f0", "fst.5", "fst1", "fsl2", "fsl3", "fsg.5", "fsg1", "fsg2", "fsr1_5", "fsr.5_3" , "flogit" );
  foreach my $this_filter_ ( @filter_algo_ )
  {
    my $is_valid = grep /^$this_filter_$/, @valid_filter_list_ ;
    if($is_valid == 0)
    {
      print MAINLOGFILEHANDLE "ERROR : filtration $this_filter_ not implemented\n";
      print "ERROR : filtration $this_filter_ not implemented\n";
      SendErrorMailAndDie ("Filtration $this_filter_ not implemented\nWorkDir: $work_dir_");
    }
  }
}
sub SendErrorMailAndDie
{
  my $error_string_ = "@_";
  $hostname_ = `hostname`;
  open ( MAIL , "|/usr/sbin/sendmail -t" );
  print MAIL "To: $mail_address_\n";
  print MAIL "From: $mail_address_\n";
  print MAIL "Subject: Error- Combine Indicators ( $config_file_ ) $hostname_\n\n";
  print MAIL "$error_string_";
  close(MAIL);
  `rm -r $work_dir_$ilist_dir_`;
  `rm $email_file_`;

  PrintErrorAndDie ( $error_string_ );
}
sub PrintErrorAndDie
{
  my $error_string_ = "@_";

  print STDERR "\nERROR: ".$error_string_."\n";

  die;
}



