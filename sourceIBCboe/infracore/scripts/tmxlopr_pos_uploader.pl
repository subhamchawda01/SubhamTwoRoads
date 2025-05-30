#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;

#First look at the ATR files
my $USAGE="$0 ors_trades_filename_";
if ( $#ARGV < 0 ) { print $USAGE; exit ( 0 ); }
my $ors_trades_filename_ = $ARGV[0];
my $prev_day_pos_file_ = "";

if ( $#ARGV > 0) 
{
  $prev_day_pos_file_ = $ARGV[1] ;
}

my %symbol_to_pos_map_ = ();
my %symbol_to_vol_map_ =  ();

if ( $prev_day_pos_file_ ) 
{
  open PREV_DAY_POSITION_FILE_HANDLE, "< $prev_day_pos_file_ " or die "Could not open prev_day_file $prev_day_pos_file_\n";
  my @pos_lines_ = <PREV_DAY_POSITION_FILE_HANDLE> ;
  close ( PREV_DAY_POSITION_FILE_HANDLE ) ;
  my $start_reading_ = 0 ;
  foreach my $line_ ( @pos_lines_ ) 
  {
    if ( index ( $line_, "Security Position Map") >= 0 ) { $start_reading_ =1 ; } 
    elsif ( index ( $line_, "Cumulative Bid Size Map" ) >= 0 || index ( $line_, "Cumulative Ask Size Map" ) >= 0 ) { $start_reading_ = 0 ; } 

    if ( index ( $line_, "C_BAX" ) >= 0 ) { next ; } 
    if ( index ( $line_, "Security_Id:"  ) >= 0 && $start_reading_ > 0 ) 
    {
      my @line_words_ = split ( " ", $line_ );
      my $sec_name_str_ = $line_words_[0] ;
      my @sec_name_words_ = split ( ":", $sec_name_str_ ) ;
      my $secname_ = $sec_name_words_[1];
      my $t_word_ = $line_words_[1];
      my @t_word_words_ = split ( ":", $t_word_ );
      my $pos_ = $t_word_words_[1]; 
      my $symbol_ = substr ( $secname_ , 0 , -1 )."1".substr ( $secname_ , -1 , 1 );chomp ( $symbol_);
      $symbol_to_pos_map_ {$symbol_} = $pos_; 
#  print "POSITION $symbol_ $pos_\n"
    }
  }
}

my $ors_trades_file_base_ = basename ($ors_trades_filename_);
chomp ($ors_trades_file_base_);

#printf "%10s\n", $ors_trades_file_base_;

open ORS_TRADES_FILE_HANDLE, "< $ors_trades_filename_" or die "add_results_to_local_database.pl could not open ors_trades_filename_ $ors_trades_filename_\n";

my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;

close ORS_TRADES_FILE_HANDLE;

for ( my $i = 0; $i <= $#ors_trades_file_lines_; $i ++ ) 
{
    
    my @words_ = split (  '', $ors_trades_file_lines_[$i] );
    if ( $#words_ >= 4 ) 
    {
	my $symbol_ = $words_[0];
	my $buysell_ = $words_[1];
	my $tsize_ = $words_[2];
	my $tprice_ = $words_[3];
	if ( ! ( exists $symbol_to_pos_map_{$symbol_} ))
	{
	    $symbol_to_pos_map_{$symbol_} = 0;
	}
	if ( !(exists $symbol_to_vol_map_{$symbol_} ) )
	{
	    $symbol_to_vol_map_{$symbol_} = 0;
	}

	if ( (exists $symbol_to_vol_map_{$symbol_} ) )
	{
	    $symbol_to_vol_map_{$symbol_} += $tsize_;
	}
	if ( $buysell_ == 0 ) 
	{
	    $symbol_to_pos_map_{$symbol_} += $tsize_;
	}
	if ( $buysell_ == 1) 
	{
	    $symbol_to_pos_map_{$symbol_} -= $tsize_;
	}

    }
}

#check if all postions are ZERO for all instruments
my $if_all_positions_zero=1; #set to true
#exit(0);
foreach my $symbol_ (keys %symbol_to_pos_map_ )
{
#    printf "%s %d %d\n", $symbol_, $symbol_to_pos_map_{$symbol_}, $symbol_to_vol_map_{$symbol_};

    if ( $symbol_to_pos_map_{$symbol_} != 0 ) 
    {
	printf "%s has non-zero pos %s\n", $symbol_,$symbol_to_pos_map_{$symbol_};
	$if_all_positions_zero = 0; #set to false
	
    }
    
}
my $submit_title_ = "LOPR: FIX LOGS";
my $submit_subject_ = "LOPR: FIX LOGS";
my $to='nseall@tworoads.co.in';
my $from= 'nseall@tworoads.co.in';

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
    #SEND email first
    print "Content-type: text/html\n\n";
    
    my $title='LOPR non-zero position';
    my $subject='LOPR non-zero position';
    
    open(MAIL, "|/usr/sbin/sendmail -t");
    
## Mail Header
    print MAIL "To: $to\n";
    print MAIL "From: $from\n";
    print MAIL "Subject: $subject\n\n";
## Mail Body
    print MAIL "LOPR position for instruments are non-zero \n";

    my $register_account="/home/pengine/prod/live_scripts/ors_control.pl TMXLOPR BDMLOPR TMXREGISTERACCT";
    print STDOUT `$register_account` ;

    my $base_upload_command_ = "/home/pengine/prod/live_scripts/ors_control.pl TMXLOPR BDMLOPR TMXPOSITIONENTRY ";

    foreach my $symbol_ (keys %symbol_to_pos_map_ )
    {
	if ($symbol_to_pos_map_{$symbol_} != 0) {

	    my $t_converted_symbol_ = substr ( $symbol_ , 0 , -2 ).substr ( $symbol_ , -1 , 1 );

	    my $t_upload_command_ = " ".$base_upload_command_.$t_converted_symbol_." ".$symbol_to_pos_map_{$symbol_};
	    print STDOUT `$t_upload_command_ `;

	    printf MAIL "%s %d %d\n", $symbol_, $symbol_to_pos_map_{$symbol_}, $symbol_to_vol_map_{$symbol_};
	}
    }

    close(MAIL);
}

my $date_str_ = `date +\"%a %b %-d\"`; chomp ( $date_str_ ) ;
$date_str_ =~ s/GMT //;
my $exec_cmd_ = "sed -n -e '/$date_str_/,\$p' /spare/local/ORSlogs/TMXLOPR/BDMLOPR/LOPR_message.log";
print $exec_cmd_."\n";
my $content_ = `$exec_cmd_`; chomp ( $content_ ) ;
open(MAIL, "|/usr/sbin/sendmail -t");
print MAIL "To: $to\n";
print MAIL "From: $from\n";
print MAIL "Subject: $submit_subject_\n\n";
print MAIL "LOPR Message Fix Log $date_str_\n";
print MAIL $content_."\n";
close ( MAIL ); 
#
