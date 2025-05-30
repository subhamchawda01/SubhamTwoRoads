#!/usr/bin/perl

use strict;
use warnings;

my $HOME=$ENV{"HOME"};

my $REPO = "infracore";
my $USAGE = "$0 YYYYMMDD";

if( $#ARGV < 0 )
{
    print "$USAGE\n";
    exit( 0 );
}
my $yyyymmdd_ = $ARGV[ 0 ];
if ( $yyyymmdd_ eq "TODAY" )
{
    $yyyymmdd_ = `date +%Y%m%d`;chomp( $yyyymmdd_ );
}
if ( $yyyymmdd_ eq "YESTERDAY" )
{
    $yyyymmdd_ = `cat /tmp/YESTERDAY_DATE`; chomp( $yyyymmdd_ );
}

print $yyyymmdd_;
my $base_dst_dir_ = "/NAS1/data";
my $base_src_dir_ = "/spare/local/MDSlogs";

my %dest_dir_to_ip_ = ();
my %dest_dir_to_src_ = ();

$dest_dir_to_ip_{"BMFLoggedData/BRZ"} = "10.23.23.11";
$dest_dir_to_ip_{"CHIXLoggedData/BSL"} = "10.23.52.52";
$dest_dir_to_ip_{"CHIXLoggedData/FR2"} = "10.23.102.51";
$dest_dir_to_ip_{"CHIX_L1LoggedData/BSL"} = "10.23.52.51";
$dest_dir_to_ip_{"CHIX_L1LoggedData/FR2"} = "10.23.102.51";
$dest_dir_to_ip_{"CMELoggedData/BRZ"} = "10.23.23.11";
$dest_dir_to_ip_{"CMELoggedData/BSL"} = "10.23.52.52";
$dest_dir_to_ip_{"CMELoggedData/CHI"} = "10.23.82.52";
$dest_dir_to_ip_{"CMELoggedData/FR2"} = "10.23.102.52";
$dest_dir_to_ip_{"CMELoggedData/TOR"} = "10.23.182.51";
$dest_dir_to_ip_{"EUREXLoggedData/BRZ"} = "10.23.23.11";
$dest_dir_to_ip_{"EUREXLoggedData/BSL"} = "10.23.52.52";
$dest_dir_to_ip_{"EUREXLoggedData/CHI"} = "10.23.82.52";
$dest_dir_to_ip_{"EUREXLoggedData/FR2"} = "10.23.102.52";
$dest_dir_to_ip_{"LIFFELoggedData/BRZ"} = "10.23.23.11";
$dest_dir_to_ip_{"LIFFELoggedData/BSL"} = "10.23.52.52";
$dest_dir_to_ip_{"LIFFELoggedData/CHI"} = "10.23.82.52";
$dest_dir_to_ip_{"LIFFELoggedData/FR2"} = "10.23.102.52";
$dest_dir_to_ip_{"HKEXLoggedData/HK"} = "10.152.224.146";
$dest_dir_to_ip_{"HKEXLoggedData/TOK"} = "10.134.210.184";
$dest_dir_to_ip_{"OSE_L1LoggedData/HK"} = "10.152.224.146"
$dest_dir_to_ip_{"OSEPriceFeedLoggedData/TOK"} = "10.134.210.184";
$dest_dir_to_ip_{"TMX_FSLoggedData/TOR"} = "10.23.182.51";
$dest_dir_to_ip_{"NTPLoggedData/BRZ"} = "10.23.23.11";
$dest_dir_to_ip_{"NTP_ORDLoggedData/BRZ"} = "10.23.23.11";
$dest_dir_to_ip_{"RTS_P2LoggedData/MOS"} = "172.18.244.107";
$dest_dir_to_ip_{"RTSLoggedData/MOS"} = "172.18.244.107";
$dest_dir_to_ip_{"MICEXLoggedData/MOS"} = "172.18.244.107";

my $mail_body_ = "";

foreach my $dest_dir_ ( keys %dest_dir_to_ip_ )
{
    my $date_dir_ = substr( $yyyymmdd_ , 0 , 4 )."/".substr( $yyyymmdd_ , 4 , 2 )."/".substr( $yyyymmdd_ , 6 , 2 );
    my @dest_dir_words_ = split('/', $dest_dir_ );
    my $exchange_dir_ = $dest_dir_words_[ 0 ];
    $exchange_dir_ = substr( $exchange_dir_ , 0, -10 );
    if( $exchange_dir_ eq "TMX_FS" )
    {
	$exchange_dir_ = "TMX";
    }

    if ( $dest_dir_ eq "EUREXLoggedData/FR2" )
    {
	$exchange_dir_ = "EUREX_NTA";
    }
    my @no_files_at_src_ = `ssh dvcinfra\@$dest_dir_to_ip_{ $dest_dir_ } \'ls /spare/local/MDSlogs/$exchange_dir_ \| grep $yyyymmdd_ \| wc -l\'`;
    chomp( @no_files_at_src_ );
    my @no_files_at_dst_ = `ls /NAS1/data/$dest_dir_/$date_dir_/ \| wc -l`;
    chomp( @no_files_at_dst_ );
    if( $no_files_at_src_[0] ne $no_files_at_dst_[0] ){ $mail_body_ = $mail_body_."$dest_dir_: files at src:$no_files_at_src_[0] & files at dst: $no_files_at_dst_[0]\n"; }
}

if( $mail_body_ ne "" )
{
    open ( MAIL , "|/usr/sbin/sendmail -t" );
    print MAIL "To: nseall@tworoads.co.in, ravi.parikh\@tworoads.co.in\n";
    print MAIL "Subject: File Server Data Copy Check $yyyymmdd_\n\n\n";
    print MAIL "$mail_body_" ;
    close(MAIL);
}
