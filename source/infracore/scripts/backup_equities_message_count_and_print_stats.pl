#!/usr/bin/perl

use warnings;
use strict;
use List::Util qw/max min/; # for max

if ( $#ARGV < 1 )
{
    printf "usage: Config date \n";
    exit ( 0 );
}

my $SFILE=$ARGV[0];
my $date_ = $ARGV[1];
my $ors_trades_dir_ = "" ;
if ( $#ARGV >= 2 ) 
{
    $ors_trades_dir_ = $ARGV[2];
} 

my $MSG_STAT_PATH="/spare/local/files/BMFEQ/Messages/";

my @unique_lines_ = () ;
my %set_to_acc_name_map_ = () ;
my %set_to_prod_vec_ = ();
my %set_to_msg_vec_ = ();
my %session_to_acc_ = ();

open ( SFILE_HANDLE, "< $SFILE" ) or die " Could not open file $SFILE for reading \n" ;
while ( my $sline_ = <SFILE_HANDLE> )
{
    chomp ( $sline_ ) ;
    if ( not $sline_ ) { next; } 
    if  ( index ( $sline_ , "SECURITY_SET" ) >= 0 )
    {
        my @line_words_ = split ( ' ', $sline_ ) ;
        if ( $#line_words_ >= 1) 
        {
            my $set_ = -1 ;
            my @acc_words_ = split ( '-', $line_words_ [0] );
            if ( $#acc_words_ >= 1 ) 
            {
                $set_ = $acc_words_[1];
            }
            
            my @prod_vec_ = ();
            my @msg_vec_ = () ;       
            my @stat_words_ = split ( ',', $line_words_[1] ) ;
            if ( $#stat_words_ >= 0 ) 
            {
                my $index_ = 0 ;
                while ( $index_ <= $#stat_words_ ) 
                {
                   my $prod_ = $stat_words_ [ $index_++] ;
                   my $msg_ = $stat_words_[$index_++];
                   $index_++;
                   push ( @prod_vec_, $prod_ ) ;
                   push ( @msg_vec_ , $msg_ ) ;
                }
            }
            $set_to_prod_vec_{$set_} = \@prod_vec_ ;
            $set_to_msg_vec_ {$set_} = \@msg_vec_ ;
        }
    }    
    elsif ( index ( $sline_ , "AccountName" ) >= 0 )
    {
        my @line_words_ = split ( ' ', $sline_ ) ;
        if ( $#line_words_ >= 1) 
        {
            my $session_ = -1 ;
            my @acc_words_ = split ( '-', $line_words_ [0] );
            if ( $#acc_words_ >= 1 ) 
            {
                $session_ = $acc_words_[1];
                chomp ( $line_words_[1] ) ;
                $session_to_acc_{$session_ } = $line_words_[1];
            }
        }
    }
    elsif ( index ( $sline_, "Multisessions" ) >= 0 )
    {
        my @line_words_ = split ( ' ', $sline_ ) ;
        if ( $#line_words_ >= 1) 
        {
            my $set_ = 0 ;
            my $current_acc_ = -1 ;
            my @acc_words_ = split ( ',', $line_words_ [1] );
            if ( $#acc_words_ >= 1 ) 
            {
                foreach my $sec_ ( @acc_words_ ) 
                {
                    my $this_acc_ = int ( $session_to_acc_{$sec_});
                    if ( $current_acc_ == -1 || $this_acc_ != $current_acc_ )
                    {
                        $set_to_acc_name_map_ {$set_} = $this_acc_ ;
                        $current_acc_ = $this_acc_ ;
                        $set_++;
                    }
                }
            }
        }
    }
}

my $today_dir_ = $MSG_STAT_PATH."/".$date_."/";
`mkdir -p $today_dir_`;

my $today_file_ = $today_dir_."message_stat.txt";
open TODAY_STAT, "> $today_file_ " or PrintStackTraceAndDie ( " Could not open file $today_file_ for writing\n" ); 

print TODAY_STAT "#instruement #account #message_\n";
foreach my $key_ ( keys %set_to_prod_vec_ ) 
{
    my @prod_vec_ = @{ $set_to_prod_vec_{$key_}};
    my @msg_vec_ = @{$set_to_msg_vec_ {$key_}};
    my $index_ = 0 ;
    while ( $index_ <= $#prod_vec_ ) 
    {
        print TODAY_STAT "$prod_vec_[$index_] ".$set_to_acc_name_map_{$key_}." $msg_vec_[$index_]\n";
        $index_++;
    }
}

close TODAY_STAT ;

my $yyyymm_ = substr ( $date_, 0, 6 ) ;
my @dir_list_ = `ls $MSG_STAT_PATH | grep $yyyymm_`;
my %acc_to_prod_to_message_ = ();
my %acc_to_prod_to_days_ = () ;
my %acc_to_prod_to_notional_ = ();
my %acc_to_prod_to_num_trades_ = ();

foreach my $dir_ ( @dir_list_ ) 
{
    my %prod_to_acc_ = ();
	chomp ( $dir_ ) ;
	my $file_ = $MSG_STAT_PATH."$dir_/message_stat.txt";
	open STAT, "< $file_ " or PrintStackTraceAndDie ( "Could not open file $file_ for readin\n"); 
	my @lines_ = <STAT>;
	close STAT;
	foreach my $line_ ( @lines_ ) 
	{
		if ( index ( $line_, "#" ) >= 0 ) { next ; }  
		my @lw_ = split ( ' ', $line_ ) ;
		if ( exists $acc_to_prod_to_message_ {$lw_[1] } )
		{
			$acc_to_prod_to_message_{$lw_[1]}{$lw_[0]} += $lw_[2];
			$acc_to_prod_to_days_ {$lw_[1]}{$lw_[0]} += 1 ;
            $prod_to_acc_{$lw_[0]} = $lw_[1];
		}
		else
		{
			$acc_to_prod_to_message_ {$lw_[1]}{$lw_[0]} = $lw_[2];
			$acc_to_prod_to_days_ {$lw_[1]}{$lw_[0]} = 1 ;
            $prod_to_acc_{$lw_[0]} = $lw_[1];
		}
	}
    
    my $trade_file_ = $ors_trades_dir_."/trades.$dir_";
    if ( -e $trade_file_ ) 
    {
        open ORS_TRADES, "< $trade_file_ " or PrintStackTraceAndDie ( "Could not open ors file $trade_file_ for reading\n" );
        my @trade_lines_ = <ORS_TRADES>;
        close ORS_TRADES;
        foreach my $trade_line_ ( @trade_lines_ ) 
        {
            my @lw_ = split( '\01', $trade_line_ ); 
            if ( $#lw_ >= 3 )
            {
                if ( exists $acc_to_prod_to_notional_{$prod_to_acc_{$lw_[0]}} )
                {
                    $acc_to_prod_to_notional_{$prod_to_acc_{$lw_[0]}}{$lw_[0]} += ($lw_[2] * $lw_[3] ) ;
                    $acc_to_prod_to_num_trades_{$prod_to_acc_{$lw_[0]}}{$lw_[0]} ++;
                }
                else
                {
                    $acc_to_prod_to_notional_{$prod_to_acc_{$lw_[0]}}{$lw_[0]} = ($lw_[2] * $lw_[3] ) ;
                    $acc_to_prod_to_num_trades_{$prod_to_acc_{$lw_[0]}}{$lw_[0]} = 1 ;
                }
            }
        }
    }
}

foreach my $keys_ ( keys %acc_to_prod_to_message_ )
{
	print "\n\n------------------- Account : $keys_ ------------------------\n\n"; 
    if ( $ors_trades_dir_ ) 
    {
        print "#prod    #msg_ #num_trades_ #notional_ #used_msg1_ #used_msg2_ #penalty1_ #penalty2_ \n";
    }
    else
    {
        print "#prod #total_msg_\n";
    }
	foreach my $k_ ( keys %{$acc_to_prod_to_message_{$keys_}} )
	{
		if ( $acc_to_prod_to_message_{$keys_}{$k_} > 0 ) 
		{
            my $msg_ = $acc_to_prod_to_message_{$keys_}{$k_};
            if ( $ors_trades_dir_ )
            {
                my $num_trades_ = $acc_to_prod_to_num_trades_{$keys_}{$k_};
                my $notional_ = $acc_to_prod_to_notional_{$keys_}{$k_} ;
                my $eff_msg1_ = $msg_ - $notional_ * 0.006 ;
                my $eff_msg2_ = $msg_ - $num_trades_ * 60 ;
                my $pro_rated_msg_limit_ = int ( 60000 * int ( substr ( $date_, 6, 2 ) ) / 30 ) ;
                printf "%6s %7d %6d %10d %10d %10d %10d %10d\n",$k_,$msg_, $num_trades_,$notional_, $eff_msg1_ , $eff_msg2_, max ( 0, $eff_msg1_-$pro_rated_msg_limit_)*0.01 , max ( 0, $eff_msg2_ - $pro_rated_msg_limit_ )*0.01, ;
            }
            else
            {
                printf "%s, %d \n",$k_,$msg_ ;
            }
		}
	}
}
