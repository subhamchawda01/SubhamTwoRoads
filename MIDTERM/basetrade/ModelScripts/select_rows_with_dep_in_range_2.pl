#!/usr/bin/perl

# \file ModelScripts/select_rows_with_dep_exceeding_min.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes :
# R_REG_DATA_INPUT_FILENAME  MIN_STDEV_MULTIPLIER  RW_REG_DATA_OUTPUT_FILENAME

use strict;
use warnings;
use Math::Complex ; # sqrt
use List::Util qw/max min/;    # for max

my $USER           = $ENV{'USER'};
my $HOME_DIR       = $ENV{'HOME'};
my $REPO           = "basetrade";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl";  # PrintStacktrace , PrintStacktraceAndDie

# start
my $USAGE="$0  R_REG_DATA_INPUT_FILENAME  <IS_STDEV_FACTOR? (0/1)> LOWER_CUTOFF/-1 HIGHER_CUTOFF/-1  RW_REG_DATA_OUTPUT_FILENAME";

if ( $#ARGV < 2 ) { print $USAGE. "\n"; exit(0); }
my $r_reg_data_input_filename_   = $ARGV[0];
my $is_stdev_factor_ = $ARGV[1];
my $dep_min_cutoff_;
$dep_min_cutoff_ = $ARGV[2] if $ARGV[2] > 0;
my $dep_max_cutoff_;
$dep_max_cutoff_ = $ARGV[3] if $ARGV[3] > 0;
my $rw_reg_data_output_filename_ = $ARGV[4];

open( INFILE, "< $r_reg_data_input_filename_" ) 
or PrintStacktraceAndDie( "Could not open $r_reg_data_input_filename_ for reading!\n");

my @infile_lines_ = <INFILE>; chomp ( @infile_lines_ );
my $ncols_ = scalar ( split( /\s+/, $infile_lines_[0] ) );
@infile_lines_ = grep { scalar ( split( /\s+/, $_ ) ) >= $ncols_ } @infile_lines_;
  
my @dep_vec_ = map { ( split( /\s+/, $_ ) )[0] } @infile_lines_;

close INFILE;

if ( $is_stdev_factor_ ) {
  my $l0_dep_sum_ = scalar @dep_vec_;
  my $l2_dep_sum_ = 0;
  foreach ( @dep_vec_ ) { $l2_dep_sum_ += $_ * $_; }

  my $nomean_stdev_ = sqrt ( $l2_dep_sum_ / ( $l0_dep_sum_ - 1 ) );

  if ( defined $dep_min_cutoff_ ) { $dep_min_cutoff_ *= $nomean_stdev_; }
  if ( defined $dep_max_cutoff_ ) { $dep_max_cutoff_ *= $nomean_stdev_; }
}

my @filtered_idx_ = 0..$#infile_lines_;

if ( defined $dep_min_cutoff_ ) {
 @filtered_idx_ = grep { abs( $dep_vec_[ $_ ] ) >= $dep_min_cutoff_ } @filtered_idx_;
}
if ( defined $dep_max_cutoff_ ) {
  @filtered_idx_ = grep { abs( $dep_vec_[ $_ ] ) <= $dep_max_cutoff_ } @filtered_idx_;
}

open( OUTFILE, "> $rw_reg_data_output_filename_" )
or PrintStacktraceAndDie( "Could not open file $rw_reg_data_output_filename_ for writing\n");

print OUTFILE join("\n", @infile_lines_[ @filtered_idx_ ] )."\n";

close OUTFILE;

