#!/usr/bin/perl

# \file ModelScripts/select_timed_data_rows_for_bad_periods.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes :
# TIMED_DATA_INPUT_FILENAME TIMED_DATA_OUTPUT_FILENAME SHORTCODE TRADINGDATE

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0 TIMED_DATA_INPUT_FILENAME TIMED_DATA_OUTPUT_FILENAME SHORTCODE YYYYMMDD BADSAMPLESFILE";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

my $timed_data_input_filename_ = $ARGV [ 0 ];
my $timed_data_output_filename_ = $ARGV [ 1 ];
my $shortcode_ = $ARGV [ 2 ];
my $yyyymmdd_ = $ARGV [ 3 ];
my $badsamples_file_ = $ARGV [ 4 ];

my ( $r_bad_periods_start_ , $r_bad_periods_end_ ) = GetBadSamplesForDate ( $badsamples_file_, $yyyymmdd_ );

my @bad_periods_start_ = @$r_bad_periods_start_;
my @bad_periods_end_ = @$r_bad_periods_end_;

if ($#bad_periods_start_ <= 0 || $#bad_periods_end_ <= 0 )
{
   my $exec_cmd_ = "cp $timed_data_input_filename_ $timed_data_output_filename_";
   `$exec_cmd_`;
   exit(0);
}

print "$#bad_periods_end_\n";

if ( $#bad_periods_start_ < 0 ) { exit ( 0 ); }

open ( OUTFILE , "> $timed_data_output_filename_" ) or PrintStacktraceAndDie ( "Could not open file $timed_data_output_filename_ for writing\n" );

open ( INFILE , "< $timed_data_input_filename_" ) or PrintStacktraceAndDie ( " Could not open $timed_data_input_filename_ for reading!\n" );

my $bad_period_index_ = 0;
while ( my $inline_ = <INFILE> )
{
    chomp ( $inline_ );

    my @timed_data_words_ = split ( ' ' , $inline_ );

    if ( $#timed_data_words_ < 3 ) { PrintStacktraceAndDie ( "Malformed timed_data_line_ : $inline_\n" ); }

    my $t_msecs_ = $timed_data_words_ [ 0 ];

    for ( ; $bad_period_index_ <= $#bad_periods_end_ && $t_msecs_ > $bad_periods_end_ [ $bad_period_index_ ] ; $bad_period_index_ ++ ) { }

    if ( $bad_period_index_ > $#bad_periods_end_ ) { last; }

    if ( $t_msecs_ >= $bad_periods_start_ [ $bad_period_index_ ] &&
	 $t_msecs_ <= $bad_periods_end_ [ $bad_period_index_ ] )
    {
	print OUTFILE $inline_."\n";
    }
}

close ( INFILE );

close ( OUTFILE );

exit ( 0 );

sub GetBadSamplesForDate {
  my $badsamples_file_ = shift;
  my $yyyymmdd_ = shift;

  open BADSHANDLE, "< $badsamples_file_" or PrintStacktraceAndDie ( "Could not open file $badsamples_file_ for writing\n" );
  my @bad_samples_vec_ = <BADSHANDLE>; chomp ( @bad_samples_vec_ );
  close BADSHANDLE;

  my @this_date_bad_samples_vec_ =  ( );
  foreach my $t_sample_ ( @bad_samples_vec_ ) {
    my ( $t_date_, $t_slot_ ) = split ( "_", $t_sample_ );
    if ( $t_date_ eq $yyyymmdd_ ) {
      push ( @this_date_bad_samples_vec_, $t_slot_ );
    }
  }

  my @bad_periods_start_ = ( );
  my @bad_periods_end_ = ( );
  foreach my $t_slot_ ( sort @this_date_bad_samples_vec_ ) {
    my $t_mfm_start_ = ($t_slot_ - 1) * 900000;
    my $t_mfm_end_ = $t_slot_ * 900000;
    push ( @bad_periods_start_, $t_mfm_start_ );
    push ( @bad_periods_end_, $t_mfm_end_ );
  }

  return ( \@bad_periods_start_, \@bad_periods_end_ );
}
