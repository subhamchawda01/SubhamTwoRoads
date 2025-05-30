#!/usr/bin/perl
use strict;
use warnings;

my $USER = $ENV { 'USER' } ;
my $HOME_DIR = $ENV { 'HOME' } ;
my $GENPERLLIB_DIR = $HOME_DIR."/infracore/GenPerlLib" ;

my $USAGE = "$0 trades_file1 trades_file2" ;

if ( $#ARGV < 1 ) 
{ 
    printf "$USAGE\n" ;
    exit ( 0 ) ; 
}

my $trades_file_1_ = $ARGV [ 0 ] ; chomp ( $trades_file_1_ );
my $trades_file_2_ = $ARGV [ 1 ] ; chomp ( $trades_file_2_ );

my %time_to_pnl_1_ = ( ) ;
my %time_to_pnl_2_ = ( ) ;
my %time_to_pos_1_ = ( ) ;
my %time_to_pos_2_ = ( ) ;
my %time_to_vol_1_ = ( ) ;
my %time_to_vol_2_ = ( ) ;
my @all_times_ = ( ) ;

{
    open TRADES_FILE , "< $trades_file_1_" or die "Could not open $trades_file_1_\n" ;
    my @trades_1_lines_ = <TRADES_FILE> ;
    close ( TRADES_FILE ) ;

    my $t_vol_ = 0;

    for ( my $i = 0 ; $i <= $#trades_1_lines_ ; $i ++ )
    {
	my @trade_words_ = split ( ' ', $trades_1_lines_ [ $i ] );
	my $t_time_ = $trade_words_ [ 0 ];
	my $t_pos_ = $trade_words_ [ 6 ] ;
	my $t_pnl_ = $trade_words_ [ 8 ] ;

	my $t_size_ = $trade_words_ [ 4 ] ;
	$t_vol_ += $t_size_ ;

	$time_to_pnl_1_ { $t_time_ } = $t_pnl_ ;
	$time_to_pos_1_ { $t_time_ } = $t_pos_ ;
	$time_to_vol_1_ { $t_time_ } = $t_vol_ ;

	push ( @all_times_ , $t_time_ ) ;
    }
}

{
    open TRADES_FILE , "< $trades_file_2_" or die "Could not open $trades_file_2_\n" ;
    my @trades_2_lines_ = <TRADES_FILE> ;
    close ( TRADES_FILE ) ;

    my $t_vol_ = 0;

    for ( my $i = 0 ; $i <= $#trades_2_lines_ ; $i ++ )
    {
	my @trade_words_ = split ( ' ', $trades_2_lines_ [ $i ] );
	my $t_time_ = $trade_words_ [ 0 ];
	my $t_pos_ = $trade_words_ [ 6 ] ;
	my $t_pnl_ = $trade_words_ [ 8 ];

	my $t_size_ = $trade_words_ [ 4 ] ;
	$t_vol_ += $t_size_ ;

	$time_to_pnl_2_ { $t_time_ } = $t_pnl_ ;
	$time_to_pos_2_ { $t_time_ } = $t_pos_ ;
	$time_to_vol_2_ { $t_time_ } = $t_vol_ ;

	push ( @all_times_ , $t_time_ ) ;
    }
}

@all_times_ = sort ( @all_times_ ) ;

my %time_to_abs_pnl_diff_ = ( ) ;
my %time_to_abs_pos_diff_ = ( ) ;
my %time_to_abs_vol_diff_ = ( ) ;

my %time_to_rel_pnl_diff_30_mins_ =  ( ) ;

{ # Write pnl diffs to temporary plot file.
    my $temp_id_ = `date +%N` ;
    $temp_id_ = $temp_id_ + 0 ;

    my $temp_plot_file_ = $HOME_DIR."/simtemp/sim_real_diff".$temp_id_ ;

    my $last_t1_pnl_ = 0 ;
    my $last_t2_pnl_ = 0 ;
    my $last_t1_pos_ = 0 ;
    my $last_t2_pos_ = 0 ;
    my $last_t1_vol_ = 0 ;
    my $last_t2_vol_ = 0 ;

    for ( my $i = 0 ; $i <= $#all_times_ ; $i ++ )
    {
	my $time_ = $all_times_ [ $i ] ;

	if ( exists $time_to_pnl_1_ { $time_ } )
	{
	    $last_t1_pnl_ = $time_to_pnl_1_ { $time_ } ;
	    $last_t1_pos_ = $time_to_pos_1_ { $time_ } ;
	    $last_t1_vol_ = $time_to_vol_1_ { $time_ } ;
	}

	if ( exists $time_to_pnl_2_ { $time_ } )
	{
	    $last_t2_pnl_ = $time_to_pnl_2_ { $time_ } ;
	    $last_t2_pos_ = $time_to_pos_2_ { $time_ } ;
	    $last_t2_vol_ = $time_to_vol_2_ { $time_ } ;
	}

	my $abs_pnl_diff_ = ( $last_t1_pnl_ - $last_t2_pnl_ ) ;
	my $abs_pos_diff_ = abs ( $last_t1_pos_ - $last_t2_pos_ ) ;
	my $abs_vol_diff_ = ( $last_t1_vol_ - $last_t2_vol_ ) ;

	$time_to_abs_pnl_diff_ { $time_ } = $abs_pnl_diff_ ;
	$time_to_abs_pos_diff_ { $time_ } = $abs_pos_diff_ ;
	$time_to_abs_vol_diff_ { $time_ } = $abs_vol_diff_ ;
    }

    open TEMP_PNL_FILE , ">" , $temp_plot_file_ or die "Could not create file $temp_plot_file_\n";

    foreach my $time_ ( sort { $a <=> $b } keys %time_to_abs_pnl_diff_ )
    {
	print TEMP_PNL_FILE "$time_ $time_to_abs_pnl_diff_{$time_} $time_to_abs_pos_diff_{$time_} $time_to_abs_vol_diff_{$time_}\n";
    }

    close TEMP_PNL_FILE ;

    `/tmp/plot_multifile_cols.pl $temp_plot_file_ 2 pnl_diff`;
#    `/tmp/plot_multifile_cols.pl $temp_plot_file_ 3 pos_diff`;
    `/tmp/plot_multifile_cols.pl $temp_plot_file_ 4 vol_diff`;
    `rm -f $temp_plot_file_`;
}
