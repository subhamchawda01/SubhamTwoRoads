#!/usr/bin/perl

my $USER = $ENV { 'USER' } ;
my $HOME_DIR = $ENV { 'HOME' } ;
my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/calc_next_business_day.pl"; # CalcNextBusinessDay
require "$GENPERLLIB_DIR/exists_with_size.pl";

my $bscript_ = "$SCRIPTS_DIR/generate_bafin_report_V3.pl";
my $odir_ = "/spare/local/$USER/BafinReports";

`mkdir -p $odir_` ;

my $to_date_ = `date +"%Y%m%d" -d "1 day ago"` ;
chomp ( $to_date_ ) ;

my $from_date_ = $to_date_ ;
my $rsync_flag_ = 0 ;

if ( scalar ( @ARGV  ) == 3 )
{
    $from_date_ = $ARGV[0];
    $to_date_ = $ARGV[1];
    $rsync_flag_ =$ARGV[2] ;

}
else
{
    print "perl gbr.pl start_date end_date <rsync_flag> " ;
    exit ( 0 ) ;
}

while ( $from_date_ <= $to_date_ )
{
    `mkdir -p $odir_/$from_date_` ;
    `rm -rf $odir_/$from_date_/*` ;

    `perl $bscript_  $from_date_ $from_date_ $odir_/$from_date_ > $odir_/$from_date_/log$from_date_`;

    my $ofile_ = substr $from_date_, 2 ;
    $ofile_ = "DV".$ofile_ ;

    if (ExistsWithSize($odir_."/".$from_date_."/".$ofile_)) {
    `mv $odir_/$from_date_/$ofile_ "$odir_/$from_date_/P9WPHG_"$ofile_` ;
    `cp "$odir_/$from_date_/P9WPHG_"$ofile_ . ` ;

#    `zip "P9WPHG_"$ofile_."gz" "P9WPHG_"$ofile_` ;
    `gzip "P9WPHG_"$ofile_` ;
    `mv "P9WPHG_"$ofile_."gz" $odir_/$from_date_/ ` ;
    }
#    `rm "P9WPHG_"$ofile_ ` ;

    $from_date_ = CalcNextBusinessDay ( $from_date_ );

}

if ( ( $USER == "dvcinfra" || $USER == "dvctrader" ) && $rsync_flag_ == 1 )
{
    `rsync -avzp $odir_ dvcinfra\@10.23.74.40:/apps/data/MFGlobalTrades/`;   
}

