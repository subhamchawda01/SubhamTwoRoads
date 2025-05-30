#!/usr/bin/perl

# \file scripts/sanity_check_global_results.pl
#
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 162, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes as input :

## This script is the top level script to generate global results through call_run_sim_overnight_perdir_longer (using call_run_simulations_2.pl)
## Maintain queue structure for all  [prods X durations]
## Give priority to scripts running for shorter duration while running few longer duration scripts as well
## Remove call_run_sim_overnight_ [both recent longer_1 longer_2 perdir]


use strict;
use warnings;
use File::Basename;
use Fcntl qw (:flock);
use File::Path qw(mkpath);
use Scalar::Util qw(looks_like_number);

sub RemoveLinesFromFile;

my $USER = $ENV { 'USER' };
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/is_server_out_of_disk_space.pl"; # IsServerOutOfDiskSpace
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # ValidDate

my $HOSTNAME=`hostname`;
my $SCRIPTNAME="$0";
my $GLOBALRESULTS_DIR_ = $HOME_DIR."/ec2_globalresults/";
my $NUM_COLUMS_IN_RESULTS_FILE = 21; #Since we dont need to verify alll columes we can have it less then number of colums actual number is 22
if ( index ( $HOSTNAME, "10-0-") >= 0 )
{
  $GLOBALRESULTS_DIR_ = "/mnt/sdf/ec2_globalresults/";
}
my $results_file_cmd_ = "ls ".$GLOBALRESULTS_DIR_."/*/*/*/*/results_database.txt";

my @results_file_list_ = `$results_file_cmd_`; chomp ( @results_file_list_);
my $invalid_line_found__ = "";
my @invalid_lines_ = ();
foreach my $result_file_ ( @results_file_list_ )
{
  my @this_file_result_lines_to_remove_ = ();
  open RESULTS_FILE_HANDLE , " < $result_file_ " or PrintStacktraceAndDie ( " Could not open $result_file_ for reading .. \n" );
  while ( my $result_line_  = <RESULTS_FILE_HANDLE> )
  {
    my $invalid_line_found_ = "";
    my @results_line_words_ = split ( ' ', $result_line_ );
    if ( $#results_line_words_ < $NUM_COLUMS_IN_RESULTS_FILE  && $#results_line_words_ >= 0  ) 
    { 
      $invalid_line_found_ = "true";
      $invalid_line_found__ = "true";
      push ( @invalid_lines_, " FILE: ".$result_file_ );
      push ( @invalid_lines_, " LINE: ".$result_line_ );
      push ( @this_file_result_lines_to_remove_, $result_line_ );
      next;
    }

    if ( ( $results_line_words_[0] eq "" ) or  ( looks_like_number (  $results_line_words_[ 0 ] ) ) )
    {
      $invalid_line_found_ = "true";
    }
    if ( ( $results_line_words_ [1] eq ""  ) or  ( ! ValidDate ( $results_line_words_ [1] ) ) ) 
    {
      $invalid_line_found_ = "true";
    }
    for ( my $i_ = 2; $i_ < $#results_line_words_; $i_++ )
    {
      if ( ( $results_line_words_ [ $i_ ] eq "" ) or  ( ! looks_like_number ( $results_line_words_ [ $i_ ] ) ) )
      {
        $invalid_line_found_ = "true";
      }
    }
    if ( $invalid_line_found_ )
    {
      push ( @invalid_lines_, "\n FILE: ".$result_file_ );
      push ( @invalid_lines_, "\n LINE: ".$result_line_ );
      push ( @this_file_result_lines_to_remove_, $result_line_ );
      $invalid_line_found__ = "true"
    }
  }
  RemoveLinesFromFile( $result_file_, @this_file_result_lines_to_remove_ );
}

if ( $invalid_line_found__ )
{
  my $email_id_ = 'diwakar@tworoads.co.in';
  open(MAIL, "|/usr/sbin/sendmail -t");
  my $hostname_=`hostname`;
  print MAIL "To: $email_id_\n";
  print MAIL "From: $email_id_\n";
  print MAIL "Subject: GlobalResults SanityCheck $hostname_\n";
  print MAIL join ( ' ', @invalid_lines_ ); 
  close ( MAIL );
}
exit;

sub RemoveLinesFromFile
{
  my ( $result_file_, @this_file_result_lines_to_remove_ ) = @_;
  open RESULTS_FILE_READ_HANDLE , " < $result_file_ " or PrintStacktraceAndDie ( " Could not open $result_file_ for reading .. \n" );
  my @results_file_cont_ = <RESULTS_FILE_READ_HANDLE>;
  my @to_write_lines_ = ( );
  foreach my $my_line_ ( @results_file_cont_ )
  {
    $my_line_ =~ s/^\s+|\s+$//g;
   if ( FindItemFromVec ( $my_line_ , @this_file_result_lines_to_remove_ ) )
   {
     next;
   }
   else
   {
     push ( @to_write_lines_, $my_line_ );
   }
  }
  close ( RESULTS_FILE_READ_HANDLE );

  open RESULTS_FILE_WRITE_HANDLE,  " > $result_file_ " or PrintStacktraceAndDie ( " Could not open $result_file_ for writing .. \n" );
  print RESULTS_FILE_WRITE_HANDLE join ("\n", @to_write_lines_ );
  close ( RESULTS_FILE_WRITE_HANDLE );
}
