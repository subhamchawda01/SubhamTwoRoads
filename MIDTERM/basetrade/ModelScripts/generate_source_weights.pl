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
my $timezone_ = "PreGMT12";

LoadConfigFile ( $ARGV[0] ) ;

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

my $result_file_ = $DESTINATION_PATH_."/portfolio_data_".$timezone_.".txt";
if ( not ExistsWithSize ( $result_file_ ) ) { `>$result_file_`;}

CollectDates () ;
GenerateSingleSourceResults () ;

print STDERR "port1: $PORT_NAME_ $dep_shc_\n";
if ( not $PORT_NAME_)
{
    foreach my $shc_ ( @source_shc_ )
    {
	$PORT_NAME_ = $PORT_NAME_.substr ( $shc_, 0, -2 ) ;
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
	$PORT_NAME_ = $PORT_NAME_.substr ( $shc_, 0, -2 ) ;
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
  exit ;
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

    foreach my $tradingdate_ ( @date_vec_ )
    {
	my $this_day_out_file_ = $price_data_dir_."/price_data_$tradingdate_";
	my %shc_price_vec_ = () ;
        my %last_price_ = ();
        my $last_print_time_ = 0.0;
        my %last_time_vec_ = () ;
        my %last_price_vec_ = () ;
        my %last_price_diff_ = () ;

        my $ex_cmd_ = "$INSTALL_BIN_DIR/get_min_price_increment ".$dep_shc_." ".$tradingdate_;
        my $tick_size_ = `$ex_cmd_`; chomp ( $tick_size_);
        $shc_min_price_increment_ { $dep_shc_ } = $tick_size_ ;
        foreach my $shc_ ( @source_shc_ )
        {
          my $ex_cmd_ = "$INSTALL_BIN_DIR/get_min_price_increment ".$shc_." ".$tradingdate_;
          $tick_size_ = `$ex_cmd_`; chomp ( $tick_size_);
          $shc_min_price_increment_ { $shc_ } = $tick_size_ ;

        }
	my $indep_shc_str_= join ( ' ', @source_shc_ ) ;
	my $exec_cmd_ = $INSTALL_BIN_DIR."/generate_price_data SHORTCODE $dep_shc_ $indep_shc_str_ -1 PORT -1 $tradingdate_ $price_type_ $start_time_ $end_time_ $timeout_usec_ 1>$this_day_out_file_";
	`$exec_cmd_ `;

	$shc_price_vec_{$dep_shc_} = 0.0;
        $last_price_diff_ { $dep_shc_ } = 0.0 ;
        $last_price_vec_ { $dep_shc_ }= 0.0 ;
        $last_time_vec_ { $dep_shc_} = 0.0 ;
	foreach my $shc_ ( @source_shc_ )
	{
	    $shc_price_vec_{$shc_} = 0.0 ;
            $last_price_diff_{ $shc_} = 0.0 ;
            $last_price_vec_ { $shc_ } = 0.0 ;
            $last_time_vec_ { $shc_ }= 0.0 ;
	}
	
#large file

	my $start_printing_ = "" ;
	my $timed_price_data_filename_ = $price_data_dir_."/timed_price_data_".$tradingdate_;
	open ( PRICE_DIFF_DATA, "> $timed_price_data_filename_" ) or PrintStacktraceAndDie ( "Could not open file $timed_price_data_filename_ for writing\n" ) ;

	open ( PRICE_DATA_FILE, "< $this_day_out_file_" ) or PrintStacktraceAndDie ( " Could not open the file $this_day_out_file_\n" ) ;

        my %filehandles_ = () ;
        open ( $filehandles_ { $dep_shc_ }, "< $this_day_out_file_" ) or PrintStacktraceAndDie ( " Could not open file for reading $this_day_out_file_\n" ) ;

        foreach my $shortcode_ ( @source_shc_ )
        {
          open ( $filehandles_{$shortcode_}, "< $this_day_out_file_" ) or PrintStacktraceAndDie ( " Could not open file for reading $this_day_out_file_\n" ) ;
        }
        

	while ( my $line_ = <PRICE_DATA_FILE> )
	{
	    chomp ($line_ ) ;
	    my @line_words_ = split ( ' ', $line_);
            if ( $#line_words_ < 2 )
            {
              next ;
            }
#the fromat is $time, shc, price

	    $shc_price_vec_{$line_words_[1]} = $line_words_[2];
	    if ( not $start_printing_ )
	    {
		$start_printing_ = "true";
		foreach my $shc_ ( @source_shc_ ) { if ( $shc_price_vec_{$shc_} == 0.0 ) { $start_printing_ = "";  } }
                if ( $shc_price_vec_{$dep_shc_} == 0.0 ) { $start_printing_ = "";}
                next;
            }

           if ( $start_printing_ && $last_print_time_== 0.0 )
           {
             foreach my $shc_ ( @source_shc_ )
             {
               $last_price_ {$shc_} = $shc_price_vec_ { $shc_ };
               $last_price_vec_{ $shc_} = $shc_price_vec_ { $shc_};
             } 
             $last_price_ { $dep_shc_} = $shc_price_vec_ { $dep_shc_ } ;
             $last_price_vec_ { $dep_shc_ }= $shc_price_vec_ { $dep_shc_ };
             $last_print_time_ = $line_words_ [0] ;
             print " Setting LastPrit time : " . $last_print_time_." ".$last_price_{ $line_words_[1]}."\n";
             foreach my $sh_ ( @source_shc_ )
             {
               print $last_price_vec_{$sh_}." ".$last_price_diff_{$sh_}." " ;
             }
             print $last_price_vec_{$dep_shc_ }." ".$last_price_diff_{$dep_shc_}." \n"; 
             next ;
           }

           if ( $last_time_vec_{ $line_words_[1] } + $duration_ > $line_words_[0]  )
           {
             print " Skipping ". $last_price_diff_{ $line_words_ [1] }." ".$last_price_vec_{$line_words_[1] }." ".$last_time_vec_{ $line_words_[1] }." ".$duration_." ".$line_words_[0]." ".$line_words_[1]."\n";
# do nothning
           }
           else
           { 
             my $this_file_handle_ = $filehandles_ { $line_words_[1] } ;
             while ( my $this_line_ = <$this_file_handle_> )
             {
               my @this_line_words_ = split (' ', $this_line_ ) ;
               if ($this_line_words_[1] ne $line_words_[1] ) 
               {
                 next;
               }
               print " INFO ". $last_price_diff_{ $line_words_ [1] }. " " .$shc_price_vec_ { $line_words_[1]  }." ".$last_price_vec_{$line_words_[1] }." ".$line_words_[1]."\n";
               if ( $last_price_vec_{$line_words_[1]} == 0.0 ) {print " ERROR".$line_words_[1]." = 0 ";}
               if ( $this_line_words_[ 0 ] + $duration_ > $line_words_ [0]  )
               {
                    # this time is higher
                    # use last value here
                 
                 $last_price_diff_{ $line_words_ [1] }= ( $shc_price_vec_ { $line_words_[1]  } - $last_price_vec_{$line_words_[1] } ) / $last_price_vec_{$line_words_[1] } ;
                 $last_price_vec_{$line_words_[1] } = $this_line_words_[2] ;
                 $last_time_vec_ { $line_words_[1] } = $this_line_words_[0] ;

                 last ;
               }
               else
               {
                 $last_price_diff_{ $line_words_ [1] }= ( $shc_price_vec_ { $line_words_[1]  } - $last_price_vec_{$line_words_[1] } ) /$last_price_vec_{$line_words_[1] } ;
                #$last_price_diff_{ $line_words_ [1] }= $shc_price_vec_ { $line_words_[1]  } - $this_line_words_[2] ;               
                 $last_price_vec_{$line_words_[1] } = $this_line_words_[2] ;
                 $last_time_vec_ { $line_words_[1] } = $this_line_words_[0] ;
               }
             }
           }

           if ($start_printing_ && ( 1000000*$line_words_[0] > 1000000*$last_print_time_ + $timeout_usec_ || $last_print_time_== 0.0 )  )
	    {
              print "PRINT: ".$shc_price_vec_ {$dep_shc_}." " .$last_print_time_ ." ".$line_words_[0]." ".$timeout_usec_." ".$line_words_[1]."\n";
		printf  PRICE_DIFF_DATA  "%.8f ", $last_price_diff_ { $dep_shc_ } ;
		printf "%.8f ", $last_price_diff_ { $dep_shc_ } ;
                $last_price_ {$dep_shc_} = $shc_price_vec_ { $dep_shc_ };
		foreach my $shc_ ( @source_shc_ ) 
		{
		    printf  PRICE_DIFF_DATA "%.8f ", $last_price_diff_ { $shc_}  ;
		printf  "%.8f ", $last_price_diff_ { $shc_ } ;
		    $last_price_ {$shc_} = $shc_price_vec_ { $shc_ };
		}
		print PRICE_DIFF_DATA "\n";
		print "\n";
		$last_print_time_ = $line_words_[0] ;
	    }

	}
	
	close ( PRICE_DIFF_DATA ) ;
	close ( PRICE_DATA_FILE ) ;

	if ( $delete_intermediate_files_ )
	{
	    print $main_log_file_handle_ " rm -rf $this_day_out_file_ \n";
	    `rm -rf $this_day_out_file_ `;
	}

	`cat $timed_price_data_filename_ >> $reg_data_file_name_`;
#$exec_cmd_ = $MODELSCRIPTS_DIR."/generate_price_diff.pl ".$timed_price_data_filename_." ".$timeout_usec_." ".$reg_data_file_name_ ;
#   print $main_log_file_handle_ $exec_cmd_."\n";
#   `$exec_cmd_`;

	if ( $delete_intermediate_files_ )
	{
	    print $main_log_file_handle_ "rm -rf $timed_price_data_filename_\n";
	    `rm -rf $timed_price_data_filename_`;
	}
#$reg_data_file_name_ = ""
    }
}

sub RunRegression 
{
    my $reg_out_file_name_ = $work_dir_."/reg_out_file.txt";
    my $exec_cmd_ = $MODELSCRIPTS_DIR."/build_linear_model.R ".$reg_data_file_name_." ".$reg_out_file_name_."\n";
    
    print $main_log_file_handle_ "$exec_cmd_\n";
    `$exec_cmd_`;
    if ( $delete_intermediate_files_ )
    {
	print $main_log_file_handle_ " rm -rf $reg_data_file_name_ \n";
	`rm -rf $reg_data_file_name_ `;
    }
    open ( REG_OUT_FILE, "<", $reg_out_file_name_ ) or PrintStacktraceAndDie ( "Could not open the file $reg_out_file_name_ " ) ;
    my @reg_out_lines_ = <REG_OUT_FILE>; chomp ( @reg_out_lines_);
    if ( $#reg_out_lines_ != $#source_shc_ )
    {
	print $main_log_file_handle_ " number_of_weights != number of independent shc\n";
	print " number_of_weights != number of independent shc\n";
	exit ();
    }

    foreach my $line_ ( @reg_out_lines_  )
    {
      my @words_ = split (' ', $line_ );
      push ( @reg_coeff_vec_ , $words_[1] ) ;
    }
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
    foreach my $line_ ( @old_ )
    {
      my @line_words_ = split ( ' ', $line_ );
      if ( $#line_words_ < 2 ) { next;}
      if ( ( $line_words_[0] eq $dep_shc_ ) and ( $line_words_[1] eq substr ( $shc_, 0 , -2 ) ) )
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
      $PORT_NAME_ = substr ( $shc_, 0 , -2 );
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
      if ( $#line_words_ >= 3 &&( $line_words_[0] eq $dep_shc_ ) && ( $line_words_[1] eq substr ( $source_shc_[$count_], 0, -2 ) )  )
      {
        if ( $line_words_[3] * $reg_coeff_vec_[$count_] >= 0 ) # check if the signs are same
        {
          push ( @new_source_, $source_shc_[$count_] );
        }
        else
        {
          $sign_mis_match_ = "true"
        }
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
      $timezone_ = "PreGMT0 ";
    }
  }

  $timezone_ ;
}
