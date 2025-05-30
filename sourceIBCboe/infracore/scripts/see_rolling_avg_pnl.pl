#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;
use List::Util qw/max min/; # for max

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore_install";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $INSTALL_BIN="$HOME_DIR/$REPO/bin/";
my $LIVE_BIN="$HOME_DIR/LiveExec/bin/";

require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; # 

RollingAveragePnl();

sub RollingAveragePnl
{
	AveragePnl(5);
	AveragePnl(20);
}

sub AveragePnl
{
	my $num_days_ = shift ;
	my $cur_date_ = `date +%Y%m%d` ;
	chomp($cur_date_);
	
	my %symbol_to_avg_pnl_ = ();
	my %symbol_to_sum_pnl_ = ();
	my %symbol_to_avg_vol_ = ();
	my %symbol_to_sum_vol_ = ();
	my %symbol_to_num_days_ = ();
	my $total_pnl_sum_ = 0;
	my $total_vol_sum_ = 0;
	
#	$cur_date_ = CalcPrevBusinessDay ( $cur_date_ ); # REMOVE THIS
	
	my $cur_num_days_ = 0 ;
	while ( $cur_num_days_ < $num_days_ )
	{
		my $pnl_file_= "/apps/data/MFGlobalTrades/EODPnl/ors_equities_pnls_$cur_date_.txt";
		
#		print "$pnl_file_\n";
		
		if ( -e $pnl_file_ )
		{
			open PNL_FILE_HANDLE, " < $pnl_file_ " or die " could not open $pnl_file_ \n" ;
			my @pnl_lines_ = <PNL_FILE_HANDLE> ;
			close PNL_FILE_HANDLE ;
			
			for ( my $i = 0 ; $i <= $#pnl_lines_ ; $i++ )
			{
				
				my @words_ = split ( '\|', $pnl_lines_[$i] );
				if ( $#words_ >= 6 )
				{
					my $symbol_ = $words_[1]; $symbol_ =~ s/^\s+|\s+$//g ; 
					my $pnl_ = `echo $words_[2] | cut -d':' -f2` ; $pnl_ =~ s/^\s+|\s+$//g ;
					my $volume_ = `echo $words_[4] | cut -d':' -f2` ; $volume_ =~ s/^\s+|\s+$//g ;
					
#					print "$symbol_, $pnl_, $volume_ \n";
					
					if ( exists ( $symbol_to_sum_pnl_{$symbol_}) )
					{
						$symbol_to_sum_pnl_{$symbol_} +=  $pnl_ ;
						$symbol_to_sum_vol_{$symbol_} +=  $volume_ ;
						$symbol_to_num_days_{$symbol_} += 1 ;
					}
					else
					{
						$symbol_to_sum_pnl_{$symbol_} = $pnl_;
						$symbol_to_sum_vol_{$symbol_} = $volume_;
						$symbol_to_num_days_{$symbol_} = 1; 
					}
					
				}	
			}
			
			my $total_pnl_ = `cat $pnl_file_ | grep TOTAL | cut -d':' -f2 | cut -d'|' -f1` ; $total_pnl_ =~ s/^\s+|\s+$//g ;
			my $total_vol_ = `grep TOTAL $pnl_file_ | cut -d':' -f3 | cut -d'|' -f1` ; $total_vol_ =~ s/^\s+|\s+$//g ;
			my $rebate_ = `grep REBATE $pnl_file_ | cut -d':' -f2`; $rebate_ =~ s/^\s+|\s+$//g ;
			
			if ( not ( $rebate_ eq '' ) )
			{
				$total_pnl_ += $rebate_ ;
			}
			
			$total_pnl_sum_ += $total_pnl_ ;
			$total_vol_sum_ += $total_vol_ ;
			
			$cur_num_days_++;
		}
		
		$cur_date_ = CalcPrevBusinessDay ( $cur_date_ );		
			
	}
	
	print "\n\nRolling Average over last $num_days_ days \n\n";
		
	foreach my $symbol_ (sort keys %symbol_to_sum_pnl_ )
	{
		$symbol_to_avg_pnl_{$symbol_} =  $symbol_to_sum_pnl_{$symbol_}/$symbol_to_num_days_{$symbol_} ;
		$symbol_to_avg_vol_{$symbol_} =  $symbol_to_sum_vol_{$symbol_}/$symbol_to_num_days_{$symbol_} ;
		printf "$symbol_ | PNL: %.3f | VOLUME: %.1f  | NUM_DAYS: $symbol_to_num_days_{$symbol_}\n", $symbol_to_avg_pnl_{$symbol_}, $symbol_to_avg_vol_{$symbol_};
	}
	
	my $total_pnl_avg_ = $total_pnl_sum_/$num_days_ ;
	my $total_vol_avg_ = $total_vol_sum_/$num_days_ ;
	
	printf "\nAVG_TOTAL | PNL: %.3f | VOLUME : %.1f | NUM_DAYS : %d\n",$total_pnl_avg_,$total_vol_avg_,$num_days_ ;
	
	
}