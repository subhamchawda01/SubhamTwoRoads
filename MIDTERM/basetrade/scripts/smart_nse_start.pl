#!/usr/bin/perl
#use strict;
#use warnings;
use FileHandle;
use POSIX;

#initialise paths:
my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'};
my $REPO="basetrade";
my $bt_directory_ = $HOME_DIR."/".$REPO;
my $INSTALL_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin/";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";


my $USAGE="$0 date_";
my $date_ = `date +'%Y%m%d'`;chomp($date_);
print "Todays date = $date_\n";

my $ids_ = `crontab -l | grep onload_start_real_trading.sh | awk '{print \$8}'`;
my $strats_ = `crontab -l | grep onload_start_real_trading.sh | awk '{print \$9}'`;

my @idarray_ = split '\n', $ids_;
my @stratarray_ = split '\n', $strats_;

my @banarray_ = ();
my @earningsarray_ = ();
my @banidarray_ = ();
my @earningsidarray_ = ();

for(my $i = 0; $i <= $#idarray_; $i ++) 
{ 
  if($idarray_[$i] < 2000)
  {
    my $banfile_ = "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/fo_secban_$date_.csv";
    my $earningsfile_ = "/spare/local/tradeinfo/NSE_Files/upcoming_earnings.csv";
    
    my $shortcode_ = `cat $stratarray_[$i] | awk '{print \$2}' | head -n1`;chomp($shortcode_);
    my $len_ = length $shortcode_ ;
    my $scd_ = substr($shortcode_, 4, $len_ - 9);
    #print $scd_."\n"; 
    #print $idarray_[$i]."\n";
       
    my $trade_ = 1;
 
    my $temp_ = `cat $banfile_ | grep $scd_`;chomp($temp_);
    if (length $temp_ > 0)
      {push @banarray_, $scd_; push @banidarray_, $idarray_[$i]; $trade_ = 0;}
    
    my $temp1_ = `cat $earningsfile_ | grep $scd_`;chomp($temp1_);
    if (length $temp1_ > 0)
    {
      my $date1_ = `cat $earningsfile_ | grep $scd_ | awk '{print \$2}'`;chomp($date1_);
      $date1_ =~ tr/-//d;
      if($date1_ == $date_)
        {push @earningsarray_, $scd_; push @earningsidarray_, $idarray_[$i]; $trade_ = 0;}
    }
    if($trade_ > 0)
    { print "LiveExec/bin/user_msg --traderid $idarray_[$i] --start\n"; }
      
  }
}

print "\nBanned Products:\n";
for(my $i = 0; $i <= $#banarray_; $i ++)
{print "$banarray_[$i] $banidarray_[$i]\n"; }

print "\nEarnings today for:\n";
for(my $i = 0; $i <= $#earningsarray_; $i ++)
{print "$earningsarray_[$i] $earningsidarray_[$i]\n"; }

