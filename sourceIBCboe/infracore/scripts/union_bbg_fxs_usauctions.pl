#!/usr/bin/perl

# \file ModelScripts/union_bbg_fxs.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
# union_bbg_fxs_usauctions.pl ~/basetrade/SysInfo/BloombergEcoReports/bbg_us_eco_2011_processed.txt ~/basetrade/SysInfo/FXStreetEcoReports/fxstreet_eco_2011_processed.txt ~/basetrade/SysInfo/BloombergEcoReports/auction_us_eco_2011_processed.txt

use strict;
use warnings;
use List::Util qw/max min/; # for max min
# use feature "switch"; # for given, when
# use File::Basename; # for basename and dirname
# use File::Copy; # for copy

sub GetEpochFromLine ;
sub AdjustBBGText;
sub AdjustFXSText;

#my $SPARE_DIR="/spare/local/basetrade/";
my $HOME_DIR=$ENV{'HOME'}; 

# start 
my $USAGE="$0 bbg_file fxs_file auction_file wasde_file output_file";

if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }
my $bbg_file_ = $ARGV[0];
my $fxs_file_ = $ARGV[1];
my $usa_file_ = $ARGV[2];
my $wasde_file_ = $ARGV[3];
my $output_filename_ = $ARGV[4];

# these are the arrays that will hold items that have not been merged yet
my @bbg_current_epoch_lines_ = ();
my @fxs_current_epoch_lines_ = ();

my @bbg_yet_to_be_merged_lines_ = ();
my @fxs_yet_to_be_merged_lines_ = ();
my @usa_yet_to_be_merged_lines_ = ();
my @wasde_yet_to_be_merged_lines_ = ();
my @all_files_vec_ = ();

# my @merged_lines_ = ();

my $earliest_unmerged_epoch_ = 9999999999;

#read the files in sepearate array. and store it in array of array



#reading the bloomberg file and storing it in the @bbg_yet_to_be_merged_lines_  array

open BBG_FILE_HANDLE, "< $bbg_file_ " or die "Could not open $bbg_file_\n" ;

while ( my $thisline_ = <BBG_FILE_HANDLE> )
{
    chomp ( $thisline_ );
#Not calling AdjustBBGText as it hasn't been called for psat 3 years
#    $thisline_ = AdjustBBGText ($thisline_);
    push ( @bbg_yet_to_be_merged_lines_, $thisline_ );
}

close ( BBG_FILE_HANDLE );
#storing the bloomberg array in all_files_vec array

push ( @all_files_vec_, \@bbg_yet_to_be_merged_lines_ );


#reading the fxstreet file and storing it in the @fxs_yet_to_be_merged_lines_ array

open FXS_FILE_HANDLE, "< $fxs_file_ " or die "Could not open $fxs_file_\n" ;

while ( my $thisline_ = <FXS_FILE_HANDLE> )
{
    chomp ( $thisline_ );
#Not calling AdjustFXSText as it hasn't been called for past 3 years
#$thisline_=AdjustFXSText($thisline_); 
    push ( @fxs_yet_to_be_merged_lines_, $thisline_ );
}

close ( FXS_FILE_HANDLE );

#storing the fxstreet array in all_files_vec array
push ( @all_files_vec_, \@fxs_yet_to_be_merged_lines_ );


#reading the auction file and storing it in the @usa_yet_to_be_merged_lines_ array
open ECO_US_FILE_HANDLE, "< $usa_file_ " or die "Could not open $usa_file_\n" ;

while ( my $thisline_ = <ECO_US_FILE_HANDLE> )
{
    chomp ( $thisline_ );
    push ( @usa_yet_to_be_merged_lines_, $thisline_ );
}

close ( ECO_US_FILE_HANDLE );


#storing the us acution array in all_files_vec array
push(@all_files_vec_, \@usa_yet_to_be_merged_lines_);


#reading the wasde file and storing the content of the file in the wasde_yet_to_be_merged_lines_ array
open ECO_WASDE_FILE_HANDLE, "< $wasde_file_ " or die "Could not open $wasde_file_\n" ;

while ( my $thisline_ = <ECO_WASDE_FILE_HANDLE> )
{
    chomp ( $thisline_ );
    push ( @wasde_yet_to_be_merged_lines_, $thisline_ );
}

close ( ECO_WASDE_FILE_HANDLE );

#storing the wasde array in the all files vec
push ( @all_files_vec_, \@wasde_yet_to_be_merged_lines_);

open OUTPUT_FILE_HANDLE, "> $output_filename_ " or die "Could not open $output_filename_ for writing\n" ;

#event array that has all the events added in the output file. no need to add an event multiple time

#write all the array to the ouitput file 

#foreach my $target_l1norm_model_ ( @target_l1norm_model_vec_ )


foreach my $event_array (@all_files_vec_) {
  foreach my $line (@$event_array){
    printf OUTPUT_FILE_HANDLE "%s\n",$line;
  }
}

close OUTPUT_FILE_HANDLE;
#sorting the file based on time
my $temp_merged_file = $output_filename_."_temp";
`cat $output_filename_|awk '{print \$1" "\$2" "\$3" "\$4" "\$5}'|sort|uniq > $temp_merged_file && mv $temp_merged_file $output_filename_`;





sub AdjustBBGText
{
    my $this_bbg_line_ = shift;
    $this_bbg_line_ =~ s/Continuing_Claims/Continuing_Jobless_Claims/g;
    $this_bbg_line_ =~ s/Continuing_Claims/Continuing_Jobless_Claims/g;
    $this_bbg_line_ =~ s/Construction_Spending_MoM/Construction_Spending_(MoM)/g;
    $this_bbg_line_ =~ s/Avg_Hourly_Earning_YOY_All_Emp/Average_Hourly_Earnings_(YoY)/g;
    $this_bbg_line_ =~ s/Avg_Hourly_Earning_MOM_All_Emp/Average_Hourly_Earnings_(MoM)/g;
    $this_bbg_line_ =~ s/Avg_Weekly_Hours_All_Employees/Avg_Weekly_Hours/g;
    $this_bbg_line_ =~ s/Consumer_Price_Index/CPI/g;
    $this_bbg_line_;
}

sub AdjustFXSText
{
    my $this_fxs_line_ = shift;
    $this_fxs_line_ =~ s/FOMC_Minutes/Minutes_of_FOMC_Meeting/g;
    $this_fxs_line_ =~ s/Consumer_Price_Index/CPI/g;
    $this_fxs_line_;
}

sub GetEpochFromLine
{
    my $thisline_ = shift;
    my @this_words_ = split ( ' ', $thisline_ );

    $this_words_[0];
}


