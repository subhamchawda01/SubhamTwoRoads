#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;

#First look at the ATR files
my $USAGE="$0 ors_trades_filename_";
if ( $#ARGV < 0 ) { print $USAGE; exit ( 0 ); }
my $ors_trades_filename_ = $ARGV[0];

my $ors_trades_file_base_ = basename ($ors_trades_filename_);
chomp ($ors_trades_file_base_);

#printf "%10s\n", $ors_trades_file_base_;

open ORS_TRADES_FILE_HANDLE, "< $ors_trades_filename_" or die "add_results_to_local_database.pl could not open ors_trades_filename_ $ors_trades_filename_\n";

my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;

close ORS_TRADES_FILE_HANDLE;

my %symbol_to_pos_map_ = ();

for ( my $i = 0; $i <= $#ors_trades_file_lines_; $i ++ ) 
{
    
    my @words_ = split (  '', $ors_trades_file_lines_[$i] );
    if ( $#words_ >= 4 ) 
    {
	my $symbol_ = $words_[0];
	my $buysell_ = $words_[1];
	my $tsize_ = $words_[2];
	my $tprice_ = $words_[3];
	if ( ! ( exists $symbol_to_pos_map_{$symbol_} ))
	{
	    $symbol_to_pos_map_{$symbol_} = 0;
	}
	if ( $buysell_ == 0 ) 
	{
	    $symbol_to_pos_map_{$symbol_} += $tsize_;
	}
	if ( $buysell_ == 1) 
	{
	    $symbol_to_pos_map_{$symbol_} -= $tsize_;
	}

    }
}

foreach my $symbol_ (keys %symbol_to_pos_map_ )
{
    printf "%s %d\n", $symbol_, $symbol_to_pos_map_{$symbol_};
    

}


#------------------------AGGREGATE REAL TRADES FROM DIFFERENT TRADING SERVERS--------
# YYYYMMDD=$(date "+%Y%m%d");

# if [ $# -eq 1 ] ;
# then
#     YYYYMMDD=$1;
# fi

# scp -q dvcinfra@10.23.182.51:/spare/local/ORSlogs/TMX/BDMA/trades.$YYYYMMDD $HOME/LOPR_upload



