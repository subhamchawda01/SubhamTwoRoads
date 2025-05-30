#!/usr/bin/perl
# \file scripts/get_bmf_top_volume_stocks.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
use strict;
use warnings;

my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'};

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $GET_VOLUME_EXEC = $BIN_DIR."/get_volume_on_day";
my $GET_UTC_HHMM_STR_EXEC = $BIN_DIR."/get_utc_hhmm_str";

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetMedianConst

require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE="$0 Exch Today num_past_days";

if ( $#ARGV < 2 )
{ 
  printf "$USAGE\n"; 
  exit ( 0 ); 
}

my $exchange_ = $ARGV [ 0 ];
my $date_ = $ARGV [ 1 ];
my $num_past_days_ = $ARGV[2] ;

my $location_ = "BRZ";
if ($exchange_ eq "NSE") {
	$location_ = "NSE";
}

my $data_path_ = "/NAS1/data/".$exchange_."LoggedData/".$location_;

my %secname_to_volume_       = ();
my %secname_to_notional_     = ();
my %secname_to_avg_volume_   = ();
my %secname_to_avg_notional_ = () ;

my $days_so_far_ = 0 ;
my $today_date_ = $date_;

while ( $days_so_far_ < $num_past_days_ ) 
{
    my $yyyy_ = substr ( $date_, 0, 4 ) ;
    my $mm_ = substr ( $date_, 4, 2 ) ;
    my $dd_ = substr ( $date_, 6, 2 ) ;
    my $today_data_dir_ = $data_path_."/".$yyyy_."/".$mm_."/".$dd_."/";

    if ( -d $today_data_dir_ ) 
    {
        if ( opendir my $dir_, $today_data_dir_ )
        {
            while ( my $item_ = readdir $dir_ ) 
            {
                my @name_words_ = split ( '_', $item_ ) ; chomp ( @name_words_ ) ;
                my $contract_name_ = $name_words_[0];
                if ( substr ( $contract_name_,length ( $contract_name_) -1 , 1 )  eq 'F' ) { next ; } 
                my $this_full_file_path_ = $today_data_dir_."/".$item_;
                my $volume_cmd_ = "$BIN_DIR/all_volumes_on_day $exchange_ $this_full_file_path_ 00 2400 0"; 
                my $volume_line_ = `$volume_cmd_`; chomp ( $volume_line_ ) ;
                my @volume_words_ = split ( ' ', $volume_line_ ) ;
                my $volume_ = 0 ;
                if ( $#volume_words_ >= 0 )  { $volume_ = $volume_words_[1] ; }
                else  { next; }
                
                $volume_cmd_ = "$BIN_DIR/all_volumes_on_day $exchange_ $this_full_file_path_ 00 2400 1"; 
                my $notional_line_ = `$volume_cmd_`; chomp ( $notional_line_ ) ;
                my @notional_words_ = split ( ' ', $notional_line_ ) ;
                my $notional_ = 0; 
                if ( $#notional_words_ >= 1 ) { $notional_ = $notional_words_[1] ; } 
                else { next; }
                
                if ( $days_so_far_ == 0 )
                {
                    $secname_to_volume_{$contract_name_} = $volume_ ;
                    $secname_to_notional_{$contract_name_} = $notional_ ;
                    $secname_to_avg_notional_{$contract_name_} = $notional_;
                    $secname_to_avg_volume_ {$contract_name_} = $volume_;
                }
                else
                {
                    if ( exists $secname_to_avg_notional_{$contract_name_} ) 
                    {
                        $secname_to_avg_notional_{$contract_name_} += $notional_;
                        $secname_to_avg_volume_{$contract_name_} += $volume_;
                    }
                }
                #print "$date_ $contract_name_ $notional_ $volume_\n";
            }
        }
    }
    
    $date_ = `$BIN_DIR/calc_prev_week_day $date_ 1`; chomp ( $date_ ) ;
    $days_so_far_++;
}

foreach my $key_ ( keys %secname_to_avg_notional_ )
{
    $secname_to_avg_notional_{$key_} /= $days_so_far_;
}

foreach my $key_ ( keys %secname_to_avg_volume_ ) 
{
    $secname_to_avg_volume_{$key_} /= $days_so_far_;
}

print " \n\n===================== Last $num_past_days_ day TOP 100 stocks by Notional : =============\n\n";
printf "%-8s | %20s | %20s | %20s | %20s | %20s\n\n", "stock", "avg_volume", "avg_notional", "last_day_volume", "last_day_notional", "info";
my $count_ = 0;
foreach my $pair_ ( sort { $secname_to_avg_notional_{$b} <=> $secname_to_avg_notional_{$a} } keys %secname_to_avg_notional_ ) 
{
    my $comment_str_ = "";
    my $contract_specs_ = `$BIN_DIR/get_contract_specs $pair_ $today_date_ LOTSIZE | awk '{print \$2}'`; chomp ( $contract_specs_ ) ;
    if ( not $contract_specs_) { $comment_str_ = "Not in secdef"; }
    printf "%-8s | %20s | %20s | %20s | %20s | %20s \n", $pair_, $secname_to_avg_volume_{$pair_}, $secname_to_avg_notional_{$pair_},$secname_to_volume_{$pair_}, $secname_to_notional_{$pair_}, $comment_str_;
    $count_++;
    if ( $count_ >= 100 ) { last ; } 
}

print "\n\n\n\n Last $num_past_days_ day TOP 100 Volume stocks: \n\n\n\n";
$count_ = 0;
foreach my $pair_ ( sort { $secname_to_avg_volume_{$b} <=> $secname_to_avg_notional_{$a} } keys %secname_to_avg_volume_ ) 
{
    my $comment_str_ = "";
    my $contract_specs_ = `$BIN_DIR/get_contract_specs $pair_ $today_date_ LOTSIZE | awk '{print \$2}'`; chomp ( $contract_specs_ ) ;
    if ( not $contract_specs_) { $comment_str_ = "Not in secdef"; }
    printf "%-8s | %20s | %20s | %20s | %20s | %20s \n", $pair_, $secname_to_avg_volume_{$pair_}, $secname_to_avg_notional_{$pair_},$secname_to_volume_{$pair_}, $secname_to_notional_{$pair_}, $comment_str_;
    $count_++;
    if ( $count_ >= 100 ) { last ; } 
}
