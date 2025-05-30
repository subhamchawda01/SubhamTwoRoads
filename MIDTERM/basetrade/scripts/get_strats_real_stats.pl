#!/usr/bin/perl

use strict;
use warnings;
use FileHandle;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD

my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."/ModelScripts";

my $cmd_ = "$0 shc start_id end_id start_date end_date [ TOP = 20 ] \n";

if ( $#ARGV <  2 )
{
    print $cmd_;
    exit ( 0 ) ;
}

my $shc_ = $ARGV[0];
my $s_date_ = $ARGV[1];
my $e_date_ = $ARGV[2];
my $sid_ = 0;
my $eid_ = 0;
my @fids_ = ( );
my %hfids_ = ( );

if ( $#ARGV == 4 )
{
    $sid_ = $ARGV[3];
    $eid_ = $ARGV[4];
    @fids_ = `seq $sid_ $eid_` ;
}
elsif ( $#ARGV == 3 )
{
    $sid_ = $ARGV[3];
    $eid_ = $ARGV[3];
    @fids_ = `seq $sid_ $eid_` ;
}

my $QT_BDIR = "/NAS1/logs/QueryTrades";
my $QL_BDIR = "/NAS1/logs/QueryLogs";

# { strat total_pnl total_vol n_days }
my %strat_pnl_ = ( );
my %strat_vol_ = ( );
my %strat_n_ = ( );

for ( my $yyyymmdd_ = $e_date_ ; $yyyymmdd_ >= $s_date_ ; $yyyymmdd_ = CalcPrevDate ( $yyyymmdd_ ) )
{
    print $yyyymmdd_."\n";
    my ($yyyy_, $mm_, $dd_) = BreakDateYYYYMMDD ( $yyyymmdd_ );
    my $QTF = $QT_BDIR."/".$yyyy_."/".$mm_."/".$dd_."/trades.".$yyyymmdd_;
    my $QLF = $QL_BDIR."/".$yyyy_."/".$mm_."/".$dd_."/log.".$yyyymmdd_;

    my $e_sym_ = `$BIN_DIR/get_exchange_symbol $shc_ $yyyymmdd_` ;    
    #my @pc_ids_ = `awk '{ print \$3 }' $QTF* | awk -F\. '{ print \$1" "\$2 }' | sort | uniq | grep $e_sym_ | awk '{ print \$2 }'` ;
    my @pc_ids_ = `grep $e_sym_ $QTF* | awk '{ print \$1" "\$3 }'  | awk -F"[.: ]" '{ print \$3" "\$7 }' | sort | uniq ` ;
    chomp ( @pc_ids_ );

    foreach my $pcid_ ( @pc_ids_ )
    {
	my ($pid, $cid) = split ( " " , $pcid_ ) ;

	if ( ! grep ( /$cid/ , @fids_ )  && scalar ( @fids_ ) != 0 ) 
	{ 
	    next ; 
	}

	if ( -e "$QLF.$pid.gz" )
	{
	    my $strat_ = `zgrep STRATEGYLINE $QLF".$pid.gz" | awk '{ print \$8" "\$9 }' | grep $cid | tail -n1` ;
	    my $stats_ = `$MODELSCRIPTS_DIR/get_pnl_stats_2.pl $QTF".$pid" H | grep $cid | awk '{print \$1" "\$2" "\$3}'` ;
	    chomp ( $strat_ ) ; chomp ( $stats_ ) ;
	    if ( $strat_ )
	    {
		my @str_ = split ( " " , $strat_ ) ;
		my @sta_ = split ( " " , $stats_ ) ;
		$strat_pnl_{$str_[1]} += $sta_[1];
		$strat_vol_{$str_[1]} += $sta_[2];
		$strat_n_{$str_[1]} += 1;
		#print $str_[1]." ".$sta_[1]." ".$sta_[2]."\n";
	    }
	}
    }
}
print "\nSUMMARY: \n\n" ;

foreach my $strat ( keys %strat_pnl_ )
{
    print $strat." ".$strat_pnl_{$strat}." ".$strat_vol_{$strat}." ".$strat_n_{$strat}."\n";
}
