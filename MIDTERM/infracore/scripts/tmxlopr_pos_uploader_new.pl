#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore_install";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $INSTALL_BIN="$HOME_DIR/$REPO/bin";

#First look at the ATR files
my $USAGE="$0 [pos-file]";

my $today_date_ = `date +"%Y%m%d"` ; chomp ( $today_date_ ) ;

my $prev_day_pos_file_ = "/spare/local/files/EODPositions/overnight_pnls_$today_date_.txt" ;

my $error_string_="";
my $slack_string_="";

if ( $#ARGV >= 0) 
{
  $prev_day_pos_file_ = $ARGV[0] ;
}

my %symbol_to_pos_map_ = ();

if ( -f $prev_day_pos_file_ ) 
{
  if (open (PREV_DAY_POSITION_FILE_HANDLE, "< $prev_day_pos_file_ ")){
  my @pos_lines_ = <PREV_DAY_POSITION_FILE_HANDLE> ;
  close ( PREV_DAY_POSITION_FILE_HANDLE ) ;
  
  foreach my $line_ ( @pos_lines_ ) 
  {
    if ( index ( $line_, "C_BAX" ) >= 0 ) { next ; } 
  
    my @line_words_ = split ( ",", $line_ );
    my $symbol_ = $line_words_[0] ; chomp($symbol_);
    my $pos_ = $line_words_[1]; chomp($pos_);
    $symbol_to_pos_map_ {$symbol_} = $pos_; 
  }
 }else{
  $error_string_ = "Could not open the position file.. Will submit 0 positions\n";
 }
}

#check if all postions are ZERO for all instruments
my $if_all_positions_zero=1; #set to true
#exit(0);
foreach my $symbol_ (keys %symbol_to_pos_map_ )
{
    if ( $symbol_to_pos_map_{$symbol_} != 0 ) 
    {
	printf "%s has non-zero pos %s\n", $symbol_,$symbol_to_pos_map_{$symbol_};
	$if_all_positions_zero = 0; #set to false
	
    }
    
}

#exit(0);
#IF all position are zero
if ( $if_all_positions_zero == 1 ) 
{
#Since all positions are zero upload delimiter
 
    my $register_account="/home/pengine/prod/live_scripts/ors_control.pl TMXLOPR BDMLOPR TMXREGISTERACCT";
    my $pos_delimiter="/home/pengine/prod/live_scripts/ors_control.pl  TMXLOPR BDMLOPR TMXPOSITIONDELIMITER";

    print STDOUT `$register_account` ;
    print STDOUT `$pos_delimiter `;

}
else
{
    $slack_string_ = $slack_string_."LOPR: NonZeroPositions\n";
    my $register_account="/home/pengine/prod/live_scripts/ors_control.pl TMXLOPR BDMLOPR TMXREGISTERACCT";
    print STDOUT `$register_account` ;

    my $base_upload_command_ = "/home/pengine/prod/live_scripts/ors_control.pl TMXLOPR BDMLOPR TMXPOSITIONENTRY ";

    foreach my $symbol_ (keys %symbol_to_pos_map_ )
    {
		if ($symbol_to_pos_map_{$symbol_} != 0) {

	    	my $t_converted_symbol_ = substr ( $symbol_ , 0 , -2 ).substr ( $symbol_ , -1 , 1 );

	    	my $t_upload_command_ = " ".$base_upload_command_.$t_converted_symbol_." ".$symbol_to_pos_map_{$symbol_};
	    	print STDOUT `$t_upload_command_ `;

	    	$slack_string_ = $slack_string_." ".$symbol_." ".$symbol_to_pos_map_{$symbol_}."\n";
		}
   }

}

$slack_string_ = $slack_string_."\n".$error_string_." "."FIX LOGS: \n";


my $date_str_ = `date +\"%a %b %-d\" `; chomp ( $date_str_ ) ;
$date_str_ =~ s/GMT //;
# current_format: Tue Mar 14
# logic below is to handle dates < 10 in which case unix date command prints 1 space while in lopr file there are 2 spaces before the number

my @date_vec = split(' ', $date_str_);
if ($#date_vec >= 2 && $date_vec[2]  < 10){
	$date_vec[2] = ' '.$date_vec[2];
}

$date_str_ = join(' ', @date_vec);
my $exec_cmd_ = "sed -n -e '/$date_str_/,\$p' /spare/local/ORSlogs/TMXLOPR/BDMLOPR/LOPR_message.log";
print $exec_cmd_."\n";
my $content_ = `$exec_cmd_`; chomp ( $content_ ) ;
$content_ = `echo "$content_" | grep -i ack`;

$slack_string_ = $slack_string_.$content_."\n";

`/home/pengine/prod/live_execs/send_slack_notification dvc-audits DATA "$slack_string_"`;

#
