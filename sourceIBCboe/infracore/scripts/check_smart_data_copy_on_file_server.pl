#!/usr/bin/perl

use strict;
use warnings;

my $HOME=$ENV{"HOME"};

my $REPO = "infracore";
my $USAGE = "$0 YYYYMMDD [EXCHANGE=ALL]";
my $first_session = 0;

if( $#ARGV < 0 )
{
    print "$USAGE\n";
    exit( 0 );
}
my $yyyymmdd_ = $ARGV[ 0 ];
my $exchange_ = "ALL";

#Are we checking for a specific exchange (default is ALL)
if ($#ARGV >= 1) {
  $exchange_ = $ARGV[1];
}

if ($#ARGV >= 2) {
  $first_session = $ARGV[2];
  $first_session = ($first_session lt 'UTC_0900');
}

if ( $yyyymmdd_ eq "TODAY" )
{
    $yyyymmdd_ = `date +%Y%m%d`;chomp( $yyyymmdd_ );
}
if ( $yyyymmdd_ eq "YESTERDAY" )
{
    $yyyymmdd_ = `cat /tmp/YESTERDAY_DATE`; chomp( $yyyymmdd_ );
}

my $base_nas_dir_ = "/NAS1/data/";
my $base_prod_dir_ = "/spare/local/MDSlogs/";
my $date_dir_ = substr( $yyyymmdd_ , 0 , 4 )."/".substr( $yyyymmdd_ , 4 , 2 )."/".substr( $yyyymmdd_ , 6 , 2 );

my $mail_body_ = "";
my %exchange_loc_map = ();

$exchange_loc_map{"BMF"} = "BRZ";
$exchange_loc_map{"BMFEQ"} = "BRZ";
$exchange_loc_map{"LIFFE"} = "BSL";
$exchange_loc_map{"ICE"} = "BSL";
$exchange_loc_map{"CFE"} = "CFE";
$exchange_loc_map{"CME"} = "CHI";
$exchange_loc_map{"EUREX"} = "FR2";
$exchange_loc_map{"HONGKONG"} = "HK";
$exchange_loc_map{"RTS"} = "MOS";
$exchange_loc_map{"MICEX"} = "MOS";
$exchange_loc_map{"AFLASH"} = "NY4";
$exchange_loc_map{"ASX"} = "SYD";
$exchange_loc_map{"OSE"} = "TOK";
$exchange_loc_map{"TMX"} = "TOR";

#Assuming that datacopy is successful by default
my $copy_completed = 1;

my $cfg_file = '/home/pengine/prod/live_configs/data_copy.cfg';
open(my $cfg, '<:encoding(UTF-8)', $cfg_file) or die "Could not open file '$cfg_file' $!";

#Go through each line one by one
while (my $row = <$cfg>) {
  chomp $row;
#Dont process commented lines
  if ($row =~ /^#/) {
    next;
  }
  my @row_words = split(' ', $row );
#Verify format is correct
  if ($#row_words < 3) {
    print "Line format must be \"LOC PROD_DIR NAS_DIR IP\". Invalid line format: $row. Skipping ...\n";
  }
#if exchange name was provided, then check for the specific exchange only
  if (($exchange_ ne "ALL") && ($exchange_ ne $row_words[0]) && ((! defined $exchange_loc_map{$exchange_}) || ($exchange_loc_map{$exchange_} ne $row_words[0]))) {
    next;
  }

  my $nas_dir = $base_nas_dir_.$row_words[2]."LoggedData/".$row_words[0]."/$date_dir_/";
  my $prod_dir = $base_prod_dir_.$row_words[1]."/";
  my $filter_flag = "--ignore='*.gz' --ignore='*.tar'";
  if ("$row_words[2]" eq "ORS") {
    #slightly different paths for ORS logged data
    $nas_dir = $base_nas_dir_."ORSData/".$row_words[0]."/$date_dir_/";
    $prod_dir = "/spare/local/ORSBCAST/".$row_words[1]."/";
    $filter_flag = "";
  }
  my $prod_ip = $row_words[3];
  my @no_files_at_src_ = `ssh dvcinfra\@$prod_ip \'ls -lb $filter_flag $prod_dir/ \| grep -v total \| grep -v \\\\\\\\ \| grep $yyyymmdd_ \| wc -l\'`;
  chomp( @no_files_at_src_ );
  my @no_files_at_dst_ =`ssh dvctrader\@52.0.55.252 'find /media/ephemeral*/s3_cache/$nas_dir/ -type f | wc -l'`;
  chomp( @no_files_at_dst_ );
  if( $no_files_at_src_[0] > $no_files_at_dst_[0] || $no_files_at_src_[0] == 0) {
    $mail_body_ = $mail_body_."$nas_dir: files at src:$no_files_at_src_[0] & files at dst: $no_files_at_dst_[0]\n";
  }
  if (($no_files_at_src_[0] > $no_files_at_dst_[0] || ($no_files_at_dst_[0] <= 0 && ("$row_words[2]" ne "ORS"))) && (!($first_session == 1 && $exchange_ eq "CHI" && ($row_words[1] eq "ICE" || $row_words[1] eq "NTP")))) {
    #my $tmp_id_ = `date +%N`; chomp($tmp_id_);
    print STDERR "ssh dvcinfra\@$prod_ip \'ls -lb $filter_flag $prod_dir/ \| grep -v total \| grep -v \\\\\\\\ \| grep $yyyymmdd_ \| wc -l\' ssh dvctrader\@52.0.55.252 'find /media/ephemeral*/s3_cache/$nas_dir/ -type f | wc -l' $no_files_at_src_[0] $no_files_at_dst_[0] \n";
   #Use this when need to check which script is failing
   #`ssh dvcinfra\@$prod_ip \'ls -lb $filter_flag $prod_dir/ \| grep -v total \| grep -v \\\\\\\\ \| grep $yyyymmdd_' &> /tmp/files/$exchange_.$tmp_id_`;   
   #`ssh dvctrader\@52.0.55.252 'find /media/ephemeral*/s3_cache/$nas_dir/ -type f | xargs ls -lrt'  &> /tmp/files/hs1_$exchange_.$tmp_id_` ;
    $copy_completed = 0;
  }

}
#Print the value to stdout
print $copy_completed;

#Mail only when exchange is "ALL", not for specific exchanges
if( $exchange_ eq "ALL" && $mail_body_ ne "" )
{  
    open ( MAIL , "|/usr/sbin/sendmail -t" );
    print MAIL "To: nseall@tworoads.co.in, chandan.kumar\@tworoads.co.in\n";
    print MAIL "Subject: Data Copy Check $yyyymmdd_\n\n\n";
    print MAIL "$mail_body_" ;
    close(MAIL);
}
