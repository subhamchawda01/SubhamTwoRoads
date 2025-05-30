#!/usr/bin/perl
#
# \file ModelScripts/generate_source_weights.pl
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

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO = "basetrade";
my $DESTINATION_PATH_=$HOME_DIR."/portfolios";

my $HOST=`hostname -s`;
chomp ( $HOST );
if ( index ( $HOST , "ip-10" ) >= 0 )
{
  $DESTINATION_PATH_="/mnt/sdf/portfolios/";
}

`mkdir -p $DESTINATION_PATH_`;

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $INSTALL_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $MIN_START_DATE=20110101 ;

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_insample_date.pl"; # GetInsampleDate

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize


my $USAGE="$0 configfile ";

if ( $#ARGV < 0 ) {print $USAGE."\n"; exit ( 0 ); }



my @source_shc_ = () ;
my $dep_shc_ = () ;
my $start_date_yyyymmdd_ = $MIN_START_DATE ;
my $end_date_yyyymmdd_ = $MIN_START_DATE ;
my $PORT_NAME_="";
my $timeout_usec_ = 1000;
my $start_time_ = 0001;
my $end_time_ = 2359 ;
my $price_type_ = "Midprice";
my $duration_ = 1;
my $input_args_ = "$timeout_usec_ 0 0 " ;
my $timezone_ = "PreGMT12";

LoadConfigFile ( $ARGV[0] ) ;
GetTimeZone ( ) ;

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $SPARE_HOME."/GSWP/".$dep_shc_."/".$unique_gsm_id_ ;
my $price_data_dir_  = $work_dir_."/PriceData/";
`mkdir -p $work_dir_`;
`mkdir -p $price_data_dir_`;

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);
my $delete_intermediate_files_ = "";
my $returns_based_="1";

my @date_vec_ = () ;
my $reg_data_file_name_ = $price_data_dir_."/reg_data_file.txt";
my @reg_coeff_vec_ = () ;
my %shc_min_price_increment_ = () ;

my $result_file_ = $DESTINATION_PATH_."/portfolio_data_tr_ridge_".$timezone_.".txt";
if ( not ExistsWithSize ( $result_file_ ) ) { `>$result_file_`;}

CollectDates () ;
GenerateSingleSourceResults () ;

print STDERR "port1: $PORT_NAME_ $dep_shc_\n";
if ( not $PORT_NAME_)
{
    foreach my $shc_ ( @source_shc_ )
    {
      my $sym_ = $shc_ ;
      $sym_ =~ s/ //g;
      $sym_ =~ s/_//g;
      $PORT_NAME_ = $PORT_NAME_.$sym_ ;
    }
}
GenerateData () ;
RunRegression () ;
while ( CheckSignInResult () &&   $#source_shc_ >0 )
{
  $PORT_NAME_ = "";
  if ( not $PORT_NAME_)
  {
    foreach my $shc_ ( @source_shc_ )
    {
      my $sym_ = $shc_ ;
      $sym_ =~ s/ //g;
      $sym_ =~ s/_//g;
      $PORT_NAME_ = $PORT_NAME_.$sym_ ;
    }
  }
  @reg_coeff_vec_ = () ;
  GenerateData () ;
  RunRegression () ;
# keep eliminating the sources till either we have single soure or
# the signs of weights of individuals are same as they would have been if the were single source
}
if ( $#source_shc_ <= 0 )
{
  print $main_log_file_handle_ " No Independent shortcode satisfies matchin sign ";
  exit  ;
}

print STDERR "port: $PORT_NAME_ $dep_shc_\n";
WriteResultsToFile () ;

exit ;

sub CollectDates 
{
    my $date_ = $end_date_yyyymmdd_ ;
    while ( $date_ >= $start_date_yyyymmdd_ )
    {
	if ( IsDateHoliday ( $date_ ) || not ValidDate ( $date_ ) || NoDataDateForShortcode ( $date_, $dep_shc_ ) || IsProductHoliday ( $date_, $dep_shc_ ) )
	{
	    $date_ = CalcPrevWorkingDateMult ( $date_,  1 ) ;
	    next ;
	}
	push ( @date_vec_ , $date_);
        $date_ = CalcPrevWorkingDateMult ( $date_, 1 ) ;
    }
}


sub GenerateData
{
   print $main_log_file_handle_ " > $reg_data_file_name_ \n"; 
   `> $reg_data_file_name_ `;

   foreach my $tradingdate_ ( @date_vec_)
   {
     my $this_day_out_file_ = $price_data_dir_."/price_data_$tradingdate_";
     my $this_day_reg_data_file_ = $price_data_dir_."/reg_data_$tradingdate_";
     my $model_name_ = MakeIndicatorList ( );
     if ( $model_name_ )
     {
       my $exec_cmd_ = $INSTALL_BIN_DIR."/datagen $model_name_ $tradingdate_ $start_time_ $end_time_ 1 $this_day_out_file_ $input_args_ ADD_DBG_CODE -1 ";
       print $main_log_file_handle_ "$exec_cmd_\n";
       `$exec_cmd_`;
# we can directly use this file for regresstion after removing initial lines , time, tgt, base

       print $main_log_file_handle_ " cp $this_day_out_file_ $this_day_reg_data_file_ \n";
#`cp $this_day_out_file_ $this_day_reg_data_file_ `;

       `cut -d' ' -f5-  $this_day_out_file_ > $this_day_reg_data_file_ `;
       print $main_log_file_handle_ " cat $this_day_reg_data_file_ >> $reg_data_file_name_\n";
       `cat $this_day_reg_data_file_ >> $reg_data_file_name_`;
       if ( $delete_intermediate_files_ )
       {
         print $main_log_file_handle_ " rm -rf $this_day_reg_data_file_ \n rm -rf $this_day_out_file_\n";
         `rm -rf $this_day_reg_data_file_`;
         `rm -rf $this_day_out_file_`;
       }
     }

  }
    
}


sub RunRegression 
{
    my $reg_out_file_name_ = $work_dir_."/reg_out_file.txt"; `>$reg_out_file_name_ `;
    my $exec_cmd_ = $MODELSCRIPTS_DIR."/build_ridge_model.py ".$reg_data_file_name_." ".$reg_out_file_name_."\n";
    
    print $main_log_file_handle_ "$exec_cmd_\n";
    `$exec_cmd_`;
    if ( $delete_intermediate_files_ )
    {
	print $main_log_file_handle_ " rm -rf $reg_data_file_name_ \n";
	`rm -rf $reg_data_file_name_ `;
    }
    open ( REG_OUT_FILE, "<", $reg_out_file_name_ ) or PrintStacktraceAndDie ( "Could not open the file $reg_out_file_name_ " ) ;
    my @reg_out_lines_ = <REG_OUT_FILE>; chomp ( @reg_out_lines_);
    if ( $#reg_out_lines_ != $#source_shc_ || $#reg_out_lines_< 0  )
    {
	print $main_log_file_handle_ " number_of_weights != number of independent shc, $#reg_out_lines_ and $#source_shc_ \n";
	print " number_of_weights != number of independent shc\n";
    }


    my $coeff_ = 0;
    my $idx_ = 0;
    while ( $coeff_ <= $#source_shc_ || $idx_ <= $#reg_out_lines_ )
    {
      my $line_ = $reg_out_lines_[$idx_];
      {
        my @words_ = split (' ', $line_ );
        if ( $words_[0] eq  $coeff_ + 1)
        {
          my $coeff_ = sprintf "%.8f ", $words_[1] ;
          push ( @reg_coeff_vec_ ,  $coeff_ ) ;
        }
        else
        {
          push ( @reg_coeff_vec_, 0 ) ;
          $idx_--;
        }
      }
      $coeff_++; $idx_++;
    }

}

sub MakeIndicatorList 
{
#make indicator list with source as well as dependents

 my $temp_ilist_file_ = $work_dir_."/ilist_".join( '_', @source_shc_ ).".txt"; 
 my $ilist_line_ = "MODELINIT DEPBASE $dep_shc_ $price_type_ $price_type_ \nMODELMATH LINEAR CHANGE\nINDICATORSTART\n";
 my $dep_line_ = "INDICATOR 1.00 SimpleReturns ".$dep_shc_." ".$duration_." ".$price_type_."\n"; 
 $ilist_line_ = $ilist_line_.$dep_line_;
 foreach my $shc_ ( @source_shc_ )
 {
   $ilist_line_ = $ilist_line_."INDICATOR 1.00 SimpleReturns ".$shc_." ".$duration_." ".$price_type_."\n"
 }
 $ilist_line_ = $ilist_line_."INDICATOREND";
 open ( ILIST, "> $temp_ilist_file_ " ) or PrintStacktraceAndDie ( " Could not open file $temp_ilist_file_ to write \n") ;
 print ILIST $ilist_line_ ;
 close ( ILIST );
 $temp_ilist_file_ ;
}

sub WriteResultsToFile 
{
  open ( EXISTING_DATA, "< $result_file_" ) or PrintStacktraceAndDie ( " Could not open file $result_file_ to read");
  my @old_ = <EXISTING_DATA>;
  close ( EXISTING_DATA );

  my $new_line_ = "$dep_shc_ $PORT_NAME_ "; 
  my $count_ = 0;
  while ( $count_ <=  $#source_shc_ )
  {
    $new_line_ = $new_line_." ".$source_shc_[$count_]." ". $reg_coeff_vec_[$count_]." ";
    $count_ ++;
  }

  $count_ = 0;
  
  my $written_ = "";
  while ( $count_ <= $#old_ )
  {
    chomp ( $old_[$count_]);
    my @line_words_ = split (' ', $old_[$count_] ) ;
    if ( $#line_words_ < 2 ) { $count_++; next;}
    if ( ( $line_words_[0] eq $dep_shc_) && ( $line_words_[1] eq $PORT_NAME_ ) )
    {
      $old_[$count_]=$new_line_ ;
      $written_ = "tru";
    }
    $count_++;
  }

  if ( not $written_ ) { push ( @old_, $new_line_ ) ;}
  open ( PORT_DATA, "> $result_file_") or PrintStacktraceAndDie ( " could not open file $result_file_ to read");
  print PORT_DATA join ("\n", @old_ );
  close ( PORT_DATA );

}


sub GenerateSingleSourceResults 
{
  open ( EXISTING_DATA, "< $result_file_" ) or PrintStacktraceAndDie ( " Could not open file $result_file_ to read");
  my @old_ = <EXISTING_DATA>;
  close ( EXISTING_DATA );
# Check if we dont have single source threshold in the file m if not then try generating that

  my @temp_source_ = @source_shc_ ;
  foreach my $shc_ ( @temp_source_ )
  {
    my $exist_ = "";
    my $sym_ = $shc_ ;
    $sym_ =~ s/ //g;
    $sym_  =~ s/_//g;

    foreach my $line_ ( @old_ )
    {
      my @line_words_ = split ( ' ', $line_ );
      if ( $#line_words_ < 2 ) { next;}
      if ( ( $line_words_[0] eq $dep_shc_ ) and ( $line_words_[1] eq $sym_  ) )
      {      
        $exist_ = "true";
      }
    }

    if ( not $exist_ )
    {
#generate result here

      print STDERR " not exist for $shc_\n";
      @source_shc_ = () ;
      @reg_coeff_vec_ = () ;
      push ( @source_shc_, $shc_ );
      $PORT_NAME_ = $sym_;
      GenerateData () ;
      RunRegression () ;
      WriteResultsToFile () ;
    }
  }
  $PORT_NAME_ = "";
  @source_shc_ = @temp_source_ ;
}

sub CheckSignInResult 
{
  my $sign_mis_match_ = "";
  print $main_log_file_handle_ " CheckSignInResult: \n Current Coeffs: ";
  my $count_ = 0;
  while ( $count_ <= $#source_shc_ )
  {
    print $main_log_file_handle_  $source_shc_[$count_]." ".$reg_coeff_vec_[$count_]." ";
    $count_++;
  }
  print $main_log_file_handle_ "\n";

  open ( EXISTING_DATA, "< $result_file_" ) or PrintStacktraceAndDie ( " Could not open file $result_file_ to read");
  my @old_ = <EXISTING_DATA>;
  close ( EXISTING_DATA );
  $count_ = 0;
  my @new_source_ = () ;
  while ( $count_ <= $#source_shc_ )
  {
    foreach my $line_ ( @old_ )
    {
      my @line_words_ = split ( ' ', $line_ ) ;
      my $sym_ = $source_shc_[$count_];
      my $t_sym_ = $sym_ ;
      $t_sym_ =~ s/ //g;
      $t_sym_ =~ s/_//g;
      if ( ( $line_words_[0] eq $dep_shc_ ) && ( $line_words_[1] eq $t_sym_ ) )
      {
        if ( $line_words_[3] * $reg_coeff_vec_[$count_] > 0 ) # check if the signs are same
        {
          push ( @new_source_, $source_shc_[$count_] );
        }
        else
        {
          $sign_mis_match_ = "true"
        }
        last ;
      }
    }
    $count_++;
  }
  @source_shc_ = @new_source_;
  $sign_mis_match_;
}
sub LoadConfigFile
{
    my $config_file_ = shift ;
    open ( CONFIG_FILE , "<" , $config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $config_file_" );
    my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
    close ( CONFIG_FILE );
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
		when ( "DEPENDENT" )
		{
		    $dep_shc_ = $t_words_[0] ;
		}
		when ( "SOURCES" )
		{
		    my $count_ = 0 ;
		    while ( $count_ <= $#t_words_ )
		    {
			push ( @source_shc_, $t_words_[$count_] ) ;
                        $count_++;
		    }
		}
		when ( "START_DATE" )
		{
		    $start_date_yyyymmdd_ = $t_words_[0];
		}
		when ( "END_DATE" )
		{
		    $end_date_yyyymmdd_ = $t_words_[0];
		}
		when ( "TIMEOUT" )
		{
		    $timeout_usec_ = 1000*$t_words_[0] ;
		}
                when ( "DURATION" )
                {
                  $duration_ = $t_words_[0] ;
                }
		when ( "START_TIME" )
		{
		    $start_time_ = $t_words_[0]
		}
		when ( "END_TIME" )
		{
		    $end_time_ = $t_words_[0] ; 
		}
		when ( "PRICE_TYPE" )
		{
		    $price_type_ = $t_words_[0];
		}
                when ( "DELETE_INTERMEDIATE_FILES")
                {
                  $delete_intermediate_files_ = $t_words_[0];
                }
                when ( "RETURNS_BASED" )
                {
                  $returns_based_= ( $t_words_[0] != 0 )
                }            
                when ( "DATAGEN_ARGS" )
                {
                  $input_args_ = ( join ( ' ',@t_words_ ) ) ;
                }

	    }
	}
    }
}

sub GetTimeZone
{
# Currenlty dividing into are 4 timezones, UTC 0-6,6-12,12-18,18-00

  my $utc_start_time_ = GetUTCHHMMStr ( $start_time_, $start_date_yyyymmdd_ ) ;
  my $utc_end_time_ = GetUTCHHMMStr ( $end_time_, $start_date_yyyymmdd_ ) ;
  my $total_duration_ = $utc_end_time_  - $utc_start_time_;
  if ( $total_duration_ < 900 )
  {
    
    if ( $utc_end_time_ <= 100 )
    {
      $timezone_ = "PreGMT0";
    }
    elsif ( $utc_end_time_ <= 600 )
    {
      $timezone_ = "PreGMT6";
    }
    elsif ( $utc_end_time_ <= 1300 )
    {
      $timezone_ = "PreGMT12";
    }
    elsif ( $utc_end_time_ <= 1900 )
    { 
      $timezone_ = "PreGMT18" ;
    }
    elsif ( $utc_end_time_ <= 2359 )
    {
      $timezone_ = "PreGMT0";
    }
  }

  $timezone_ ;
}
