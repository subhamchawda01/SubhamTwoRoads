#!/usr/bin/perl
use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore";

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/is_bmf_holiday.pl"; # Holiday
require "$GENPERLLIB_DIR/get_number_of_working_days_BMF_between_two_dates.pl" ; # CalNumberOfWorkingDaysBMFBetweenDates
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD

my $USAGE="$0 YYYYMMDD";
if ( $#ARGV < 0 ) { print "$USAGE\n"; exit ( 0 ); }

my $today_date_ = $ARGV[0];

if ( $today_date_ eq "TODAY" )
{
   $today_date_ = `date "+%Y%m%d"`;
}
print $today_date_ ; 
my %symbol_to_noof_working_days_till_expiry_ = ();

$symbol_to_noof_working_days_till_expiry_{"DI1F13"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130102 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1F14"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140102 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1F15"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150102 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1F16"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20160102 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1F17"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170131 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1F18"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20180131 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1N13"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130731 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1N14"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140731 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1N15"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150731 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1N16"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20160731 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1N17"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170731 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1N18"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20180731 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1G13"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130228 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1G14"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140228 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1G15"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150228 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1J13"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130430 ) ;

$symbol_to_noof_working_days_till_expiry_{"DI1J14"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140431 ) ;

$symbol_to_noof_working_days_till_expiry_{"OTHER"} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170131 ) ;
    

my $filename = '/spare/local/files/DI_commissions';
# my $filename = 'DI_commisions';
open(my $fh, '>', $filename) or die "Could not open file '$filename' $!";

foreach my $sh_code_ (sort keys %symbol_to_noof_working_days_till_expiry_)
{
    my $sh_comm_ = GetDICommissions($sh_code_, $today_date_);
    print $fh  "$sh_code_,$sh_comm_\n";
}
exit(0);

sub GetDICommissions {

    my $shortcode_ = shift;
    my $retval = shift;    
    my $count_ = 0 ;
    my $total_vol_ = 0;
    my $adj_vol_ = 0;
    my $comm_ = 0.3;
    

    # my $today_date_ = $retval ;
    # chomp($shortcode_);
    # print "DI_comm: $shortcode_,$retval \n";

    while ( $count_ < 21 )
    {
        $retval = `$BINDIR/calc_prev_day $retval`; # no chomp required

        while ( IsBMFHolidayIncludingWeekends ( $retval ) == 1 )
        {
            $retval = `$BINDIR/calc_prev_day $retval`; # no chomp required
        }
        
        # maintains invariant that $retval is never a holiday
        my ($YYYY_, $MM_, $DD_) = BreakDateYYYYMMDD ( $retval );
        # my @shcs_ = split("\n", `cat /apps/data/MFGlobalTrades/ProductPnl/$YYYY_/$MM_/$DD_/pnl.csv | grep DI | awk -F, '{print \$2}'`) ;
        # my @vols_ = split("\n", `cat /apps/data/MFGlobalTrades/ProductPnl/$YYYY_/$MM_/$DD_/pnl.csv | grep DI | awk -F, '{print \$4}'`) ;

        my @shcs_ = split("\n", `cat /NAS1/data/MFGlobalTrades/EODPnl/ors_pnls_$retval.txt | grep DI | awk  '{print \$2}'`) ;
        my @vols_ = split("\n", `cat /NAS1/data/MFGlobalTrades/EODPnl/ors_pnls_$retval.txt | grep DI | awk '{print \$14}'`) ;


       for (my $i = 0; $i <= $#shcs_; $i++) {       
            
            my $shc_ = $shcs_[$i];
            my $vol_ = $vols_[$i];   

           # print "$shc_, $vol_\n";     

            $total_vol_ += $vol_;

            if ( exists($symbol_to_noof_working_days_till_expiry_{$shc_} ))
            {
                $adj_vol_ += $vol_*($symbol_to_noof_working_days_till_expiry_{$shc_}/252.0);
            }
            else
            {
                $adj_vol_ += $vol_*($symbol_to_noof_working_days_till_expiry_{"OTHER"}/252.0);
            }
        }
        #exit(0);

        $count_ ++ ;
    }
    

    my $avg_vol_ = $total_vol_/21;
    # print "$total_vol_, $avg_vol_\n";
    my $const_avg_vol_ = $total_vol_/21.0;


    my $p_exch_ = 0; 
    my $p_reg_ = 0;

    while($avg_vol_ > 0)
    {
       if( $avg_vol_ <= 100 )
        {            
            $p_exch_ += $avg_vol_* 0.0012022 ;
             $p_reg_ += $avg_vol_* 0.0009790 ;
            $avg_vol_ = 0;
        }
        elsif( $avg_vol_ <= 1260)
        {
           $p_exch_ += ( $avg_vol_ - 100 )* 0.0011421 ;
           $p_reg_ += ( $avg_vol_ - 100 )* 0.0009301 ;
           $avg_vol_ = 100;
        }
        elsif( $avg_vol_ <= 2800)
        {
            $p_exch_ += ( $avg_vol_ - 1260 )* 0.0010281 ;
            $p_reg_ += ( $avg_vol_ - 1260 )* 0.0008322 ;
            $avg_vol_ = 1260;
        }
        elsif( $avg_vol_ <= 7300)
        {
            $p_exch_ += ( $avg_vol_ - 2800)* 0.0009618 ;
             $p_reg_ += ( $avg_vol_ - 2800)* 0.0007832 ;
            $avg_vol_ = 2800;
        }
        elsif( $avg_vol_ <= 47900)
        {
            $p_exch_ += ($avg_vol_ - 7300)* 0.0009016 ;
            $p_reg_ += ($avg_vol_ - 7300)* 0.0007343 ;
            $avg_vol_ = 7300;          
        }
        else
        {
            $p_exch_ += ($avg_vol_ - 47900)*0.0007815 ;
            $p_reg_ += ($avg_vol_ - 47900)*0.0006363 ;
            $avg_vol_ = 47900;            
        }
    }
    $p_exch_ = $p_exch_/$const_avg_vol_ ;
    $p_reg_ = $p_reg_/$const_avg_vol_ ;
    # print "$p_exch_, $p_reg_ \n";

    my $term = $symbol_to_noof_working_days_till_expiry_{$shortcode_};
    if ( $term > 105)
    {
        $term = 105.00;
    }
    my $unit_value_ = 100000.00*( ( 1 + $p_exch_/100.0)**($term/252.0) - 1 );
    $unit_value_ += 100000.00*( ( 1 + $p_reg_/100.0)**($term/252.0) - 1 );

    $comm_ = 0.9 * 0.35 * $unit_value_ + 0.1166181 + 0.01;
#my $temp_ = 0.9 * 0.35 * $unit_value_;
    print "$shortcode_,  $comm_\n";
    # print "comm: $comm_ \n";
    


   $comm_;
}

1;
