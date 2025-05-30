#!/usr/bin/perl

use warnings;
use strict;
use List::Util qw/max min/; # for max

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="infracore";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/infracore_install/bin";
my $SPARE_HOME="/spare/local/".$USER."/";


if ( $#ARGV < 1 )
{
    printf "usage: Config date ors_trades message_limits \n";
    exit ( 0 );
}

my $SFILE=$ARGV[0];
my $date_ = $ARGV[1];
my $ors_trades_dir_ = "" ;
my $message_limit_ = 20000;

if ( $#ARGV >= 2 ) 
{
    $ors_trades_dir_ = $ARGV[2];
} 

if ( $#ARGV >= 3 ) 
{
    $message_limit_ = $ARGV[3];
}

my $MSG_STAT_BASE="/apps/logs/ORSMessages/";
my $MSG_STAT_PATH="/apps/logs/ORSMessages/BMFEQ";

my @unique_lines_ = () ;
my %set_to_acc_name_map_ = () ;
my %set_to_prod_vec_ = ();
my %set_to_msg_vec_ = ();
my %session_to_acc_ = ();
my %new_set_to_prod_vec_ = ();

my $dest_filename_ = $SFILE; 
$dest_filename_ =~ s/dvctrader/dvcinfra/g;

{ `rsync -raz dvcinfra\@10.23.23.12:$dest_filename_ $SFILE`; }
open ( SFILE_HANDLE, "< $SFILE" ) or PrintStackTraceAndDie (  " Could not open file $SFILE for reading \n" );
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
            #$set_to_msg_vec_ {$set_} = \@msg_vec_ ;
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

close SFILE_HANDLE;
my $today_dir_ = $MSG_STAT_PATH."/".$date_."/";
`mkdir -p $today_dir_`;

my $today_file_ = $today_dir_."message_stat.txt";
open TODAY_STAT, "> $today_file_ " or PrintStackTraceAndDie ( " Could not open file $today_file_ for writing\n" ); 

foreach my $key_ ( keys %set_to_prod_vec_ )
{
    my @prod_vec_ = @{ $set_to_prod_vec_{$key_}};
    my @today_msg_vec_ = ();
    foreach my $prod_ ( @prod_vec_ )
    {
        my $obr_cmd_ = "$BIN_DIR/ors_binary_reader $prod_ $date_ | grep \"Seqd\" | wc -l ";
        my $msg_count_ = `$obr_cmd_`; chomp ( $obr_cmd_ ) ;
        push ( @today_msg_vec_, $msg_count_ );
    }
    $set_to_msg_vec_{$key_} = \@today_msg_vec_;
}

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
my @dir_list_ = `ls $MSG_STAT_PATH | grep $yyyymm_ | sort`;
my %acc_to_prod_to_message_ = ();
my %acc_to_prod_to_days_ = () ;
my %acc_to_prod_to_notional_ = ();
my %acc_to_prod_to_num_trades_ = ();
my %acc_to_prod_to_eff_msg1_ = ();
my %acc_to_prod_to_eff_msg2_ = ();

foreach my $dir_ ( @dir_list_ ) 
{
    my %prod_to_acc_ = ();
	chomp ( $dir_ ) ;
	my $file_ = $MSG_STAT_PATH."/$dir_/message_stat.txt";
	open STAT, "< $file_ " or PrintStackTraceAndDie ( "Could not open file $file_ for readin\n"); 
	my @lines_ = <STAT>;
	close STAT;
	foreach my $line_ ( @lines_ ) 
	{
                #Ignore commented lines 
		if ( index ( $line_, "#" ) >= 0 ) { next ; }  
		my @lw_ = split ( ' ', $line_ ) ;
                #Ignore malformatted lines
		if ( $#lw_ <= 1 ) { next; }

		if ( exists $acc_to_prod_to_message_ {$lw_[1] } )
		{
			$acc_to_prod_to_message_{$lw_[1]}{$lw_[0]} += $lw_[2];
			$acc_to_prod_to_days_ {$lw_[1]}{$lw_[0]} += 1 ;
		}
		else
		{
			$acc_to_prod_to_message_ {$lw_[1]}{$lw_[0]} = $lw_[2];
			$acc_to_prod_to_days_ {$lw_[1]}{$lw_[0]} = 1 ;
		}
		$prod_to_acc_{$lw_[0]} = $lw_[1];
	}
    
    my $yyyy_ = substr ( $dir_, 0, 4 );
    my $mm_ = substr ( $dir_, 4, 2 ) ;
    my $dd_ = substr ( $dir_, 6, 2 ) ;

    my $trade_file_ = $ors_trades_dir_."/".$yyyy_."/".$mm_."/".$dd_."/trades.$dir_";
    print "tradefile: $trade_file_ \n";
     
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
                $acc_to_prod_to_eff_msg1_{$keys_}{$k_} = $eff_msg1_ ;
                $acc_to_prod_to_eff_msg2_{$keys_}{$k_} = $eff_msg2_ ;
                
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

foreach my $keys_ ( keys %set_to_prod_vec_ )
{
    my @prod_to_switch_ = @{$set_to_prod_vec_{$keys_}};
    foreach my $prod_ ( @prod_to_switch_ )
    {
        my $acc_ = $set_to_acc_name_map_{$keys_};
        my $msg1_ = $acc_to_prod_to_eff_msg1_{$acc_}{$prod_};
        my $msg2_ = $acc_to_prod_to_eff_msg2_{$acc_}{$prod_};
        if ( not $msg1_ or not $msg2_ ) { next; } 
        if ( $msg1_ > $message_limit_ || $msg2_ > $message_limit_ ) 
        {
            foreach my $key3_ ( keys %set_to_prod_vec_ )
            {
                if ( $keys_ eq $key3_ ) { next ; }
                else
                {
                    my $acc2_ = $set_to_acc_name_map_{$key3_};
                    if ( not exists $acc_to_prod_to_eff_msg2_{$acc2_}{$prod_} )
                    {
                        push ( @{ $new_set_to_prod_vec_{$key3_} } , $prod_ ) ;
                    }
                    else
                    {
                        print STDOUT "All acocunts have reached message limits,".$acc_to_prod_to_eff_msg2_{$acc_}{$prod_}.
                                    " And : ".$acc_to_prod_to_eff_msg2_{$acc2_}{$prod_}." prod: $prod_ assiniging to minimum one\n";
                        print STDOUT "All acocunts have reached message limits,".$acc_to_prod_to_eff_msg2_{$acc_}{$prod_}.
                                    " And : ".$acc_to_prod_to_eff_msg2_{$acc_}{$prod_}." prod: $prod_ assiniging to minimum one\n";
                                    
                        if (  $acc_to_prod_to_eff_msg2_{$acc2_}{$prod_}  > $acc_to_prod_to_eff_msg2_{$acc_}{$prod_} )
                        {
                            push ( @{ $new_set_to_prod_vec_{$keys_} } , $prod_ ) ;
                        }
                        else
                        {
                            push ( @{ $new_set_to_prod_vec_{$key3_} } , $prod_ ) ;
                        }
                    }
                }
            }
        }
        else
        {
            push ( @{ $new_set_to_prod_vec_{$keys_} } , $prod_ );
        }
    }
}

my @security_set_lines_ = ();
foreach my $keys_ ( keys %new_set_to_prod_vec_ ) {
    my @prod_vec_ = @{$new_set_to_prod_vec_{$keys_}};
    my $str_ = "";
    $str_ =$str_. sprintf "SECURITY_SET-".$keys_." " ;
    for ( my $i_ = 0; $i_ <=  $#prod_vec_; $i_++) {
        $str_ = $str_. sprintf $prod_vec_[$i_].",0,10000";
        if ( $i_ < $#prod_vec_ ) { $str_ = $str_.sprintf ","; } 
    }
    $str_ = $str_.sprintf "\n";
    push ( @security_set_lines_, $str_ ) ;
}

open ( SFILE_HANDLE, "< $SFILE" ) or PrintStackTraceAndDie (  " Could not open file $SFILE for reading \n" );
my @slines_ = <SFILE_HANDLE>;
close SFILE_HANDLE;
my @new_slines_ = ();

for ( my $i_=0; $i_<=$#slines_; $i_++ ) {
    chomp ( $slines_[$i_] ) ;
    
    if ( index ( $slines_[$i_], "SECURITY_SET" ) >= 0 ) {
    }
    else {
        push ( @new_slines_, $slines_[$i_] ) ;
    }
}

foreach my $sec_set_ ( @security_set_lines_ ) {
    print $sec_set_."\n";
    push ( @new_slines_, $sec_set_ ) ;
}

#sync the message directory/config to NAS1, and rsync update

my $sync_cmd_ = "rsync -raz $MSG_STAT_PATH dvcinfra\@10.23.74.40:$MSG_STAT_BASE";
print $sync_cmd_."\n";
`$sync_cmd_`;
my $cfg_sync_ = "rsync -raz $SFILE dvcinfra\@10.23.74.40:$today_dir_";
print $cfg_sync_."\n";
`$cfg_sync_`;

open ( OUT_SFILE, "> $SFILE" ) or PrintStackTraceAndDie ( " Could not open file $SFILE for writing \n " ) ;
foreach my $line_ ( @new_slines_ ) 
{
    print OUT_SFILE $line_."\n" ;
}
close OUT_SFILE ;
$cfg_sync_="rsync -raz $SFILE dvcinfra\@10.23.23.12:$dest_filename_";
print $cfg_sync_."\n";
`$cfg_sync_`;

