#!/usr/bin/perl
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

my $USAGE="$0 CONFIGFILE START_DATE END_DATE";
if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }


my $global_config_file_ = $ARGV [ 0 ];
my $start_date_ = $ARGV[1];
my $end_date_ = $ARGV[2];
my %shortcode_pnl=();

my $ezone_string_cmd_ = "cat $global_config_file_ | awk '{ if(\$1==\"SHORTCODE-STRAT\") { stflag=1; } if(stflag==1 && NF==2) { print \$2; stflag=0; } }' | xargs -I % $SCRIPTS_DIR/print_strat_from_base.sh % | xargs -I % awk '{print \$NF}' %";
my $ezone_string_ = `$ezone_string_cmd_ 2>/dev/null`; chomp ( $ezone_string_ );
if ( $ezone_string_ eq "" ) {
  print "ERROR: Could not fetch the EzoneString..\n";
  exit(1);
}
print $ezone_string_."\n";

my $see_trade_script=$MODELSCRIPTS_DIR."/ebt_sendprodmsg.pl";
my $date_ezone_string_script_ = $SCRIPTS_DIR."/get_dates_for_traded_ezone.pl";
my $event_date_command_="$date_ezone_string_script_ $ezone_string_ $start_date_ $end_date_";
my @dates=`$date_ezone_string_script_ $ezone_string_ $start_date_ $end_date_`;
chomp(@dates);
my @total_pnl_=();
my $global_pnl_=0;

foreach my $index_ ( 0..$#dates ){
    my $see_traded_shortcodes_command_="$see_trade_script $global_config_file_ EBT SEETRADES $dates[$index_]|awk '{if(NR>2){print \$0}}'|awk '{print \$3}'";
    my $see_traded_pnl_command_="$see_trade_script $global_config_file_ EBT SEETRADES $dates[$index_]|awk '{if(NR>2){print \$0}}'|awk '{print \$9}'";
    my @traded_shortcodes_=`$see_traded_shortcodes_command_`;chomp(@traded_shortcodes_);
    my @traded_pnl_=`$see_traded_pnl_command_`;chomp(@traded_pnl_);
    if ($#traded_pnl_<1){
       next;
    }
    my $trade_sum=0;
    $trade_sum += $_ foreach @traded_pnl_;
    print "PNL for $dates[$index_]  :  ",$trade_sum,"\n";
    @traded_shortcodes_=map { (split(/\./,$_))[0] } @traded_shortcodes_;
    foreach my $texch_sym_ (0..$#traded_shortcodes_) {
      my $shc_ = `/home/dvctrader/basetrade_install/bin/get_shortcode_for_symbol $traded_shortcodes_[$texch_sym_] $dates[$index_] 2>/dev/null`;
      chomp ( $shc_ );
      if (exists $shortcode_pnl{$shc_}){
         $shortcode_pnl{$shc_}+=$traded_pnl_[$texch_sym_];
      }
      else{
         $shortcode_pnl{$shc_}=$traded_pnl_[$texch_sym_];
      }
    }
    $global_pnl_+=$trade_sum; 
}

print "Product PNL","\n";
while(my($k, $v) = each %shortcode_pnl) { 
  print "$k PNL: $v\n";  
}
print "Total overall PNL  :  ",$global_pnl_,"\n";
