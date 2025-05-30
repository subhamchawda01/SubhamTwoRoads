#!/usr/bin/perl
#
## \file ModelScripts/ebt_uts_risk.pl
##
## \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
##  Address:
##    Suite No 162, Evoma, #14, Bhattarhalli,
##    Old Madras Road, Near Garden City College,
##    KR Puram, Bangalore 560049, India
##    +91 80 4190 3551
#


#this script summarizes the results of the strategy in the pool for a given shortcodes

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $SPARE_HOME="/spare/local/".$USER."/";
my $REPO="basetrade";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";




my $TEMP_DIR="/spare/local/temp/";
my $USAGE="$0 ALL <strat_basename_list/Directory/POOL/STAGED> <DB/results_dir> Start_date End_date EzoneString [sortalgo=kCNAPnlAdjAverage] [skip_dates_file or INVALIDFILE]";
if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName

#get dates fror the ezone string
my $date_ezone_string_script_ = $SCRIPTS_DIR."/get_dates_for_traded_ezone.pl";

#reading the command line argument
my $strat_filename_=$ARGV[1];
my $result_storage_location_=$ARGV[2];
my $start_date_=$ARGV[3];
my $end_date_=$ARGV[4];
my $ezone_string_=$ARGV[5];
my $sortalgo_="kCNAPnlAdjAverage";
if ( $#ARGV > 5) { $sortalgo_ = $ARGV[6]; }
my $skip_dates_files_="INVALIDFILE";
if ( $#ARGV > 6) { $skip_dates_files_ = $ARGV[7]; }

my $temp_date_file_ = GetCSTempFileName ( $HOME_DIR."/ebt_temp_date" );
my @dates=`$date_ezone_string_script_ $ezone_string_ $start_date_ $end_date_`;
my $date_cmd=`$date_ezone_string_script_ $ezone_string_ $start_date_ $end_date_ > $temp_date_file_`;
`$date_cmd`;

$ezone_string_=~ s/[_0-9]*$//g;

my @strategies_;
my $BASE_STRAT_DIR=$HOME_DIR."/"."modelling";
if ($strat_filename_ eq "POOL"){
  my $list_strat_command_="ls /home/dvctrader/modelling/wf_strats/*/EBT/$ezone_string_/* |sort |xargs -I % basename %";
  @strategies_=`$list_strat_command_`;
  chomp(@strategies_);
}
elsif($strat_filename_ eq "STAGED"){
    my $list_strat_command_="ls /home/dvctrader/modelling/wf_staged_strats/*/EBT/$ezone_string_/* |sort |xargs -I % basename %";
    @strategies_=`$list_strat_command_`;
    chomp(@strategies_);

}
else{
open my $file_handle_, '<', $strat_filename_;
chomp( @strategies_ = <$file_handle_>);
close $file_handle_;
}


#print "$_\n" for @strategies_;
my @shortcodes=();
my @enumeration=(0..$#strategies_);
foreach my $index_ (@enumeration)
{
     
    my $full_strat_file_path_=`~/basetrade/scripts/print_strat_from_base.sh $strategies_[$index_]`;
    chomp($full_strat_file_path_);
    my $shortcode_=`awk -F "/" '{print \$6}' $full_strat_file_path_`;
    chomp($shortcode_);
    my $cstempfile_ = GetCSTempFileName ( $HOME_DIR."/cstemp" );
    open CSTF, "> $cstempfile_" or PrintStacktraceAndDie ( "Could not open $cstempfile_ for writing\n" );
    print CSTF "$strategies_[$index_]";


   my $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $cstempfile_ $result_storage_location_ $start_date_ $end_date_ $skip_dates_files_ $sortalgo_ 0 $temp_date_file_ 0";
   #print $exec_cmd,"\n";
   #print $shortcode_,"\n";
   my @ssr_output_ = `$exec_cmd`;
   chomp ( @ssr_output_ );
   print join ( "\n", @ssr_output_ )."\n";
}
