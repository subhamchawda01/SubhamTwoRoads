#!/usr/bin/perl
use strict;
use warnings;

my $USER = $ENV { 'USER' } ;
my $HOME_DIR = $ENV { 'HOME' } ;
my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetMedianConst

my $USAGE = "$0 SHORTCODE DATE\n" ;
if ( $#ARGV < 1 ) 
{ 
    print $USAGE; 
    exit ( 0 ); 
}

my $shortcode_ = $ARGV [ 0 ] ; chomp ( $shortcode_ ) ;
my $yyyymmdd_ = $ARGV [ 1 ] ; chomp ( $yyyymmdd_ ) ;

my $ORS_BIN_READER_EXEC = $HOME_DIR."/infracore_install/bin/ors_binary_reader" ;
my $MKT_BOOK_EXEC = $HOME_DIR."/infracore_install/bin/mkt_book_print" ;

# Pulled from the ors_bin_logs
my %saos_to_cxl_time_ = ( ) ;
my %saos_to_size_ = ( ) ;
my %saos_to_buysell_ = ( ) ;
my %saos_to_int_px_ = ( ) ;

# Pulled from the mkt_data using approximate heuristic
my %saos_to_mean_mkt_update_time_ = ( ) ;
my %saos_to_median_mkt_update_time_ = ( ) ;

my @cxl_lines_ = `$ORS_BIN_READER_EXEC $shortcode_ $yyyymmdd_ | grep " Cxld "` ;

for ( my $i = 0 ; $i <= $#cxl_lines_ ; $i ++ )
{
    my @cxl_words_ = split ( ' ' , $cxl_lines_ [ $i ] );

    my $t_int_px_ = $cxl_words_ [ 6 ];
    my $t_buysell_ = $cxl_words_ [ 8 ];
    my $t_time_ = $cxl_words_ [ 11 ];
    my $t_saos_ = $cxl_words_ [ 17 ];
    my $t_size_ = $cxl_words_ [ 25 ]; chomp ( $t_size_ );

    if ( ! exists $saos_to_cxl_time_ { $t_saos_ } )
    {
	$saos_to_cxl_time_ { $t_saos_ } = $t_time_;
	$saos_to_size_ { $t_saos_ } = $t_size_;
	$saos_to_buysell_ { $t_saos_ } = $t_buysell_;
	$saos_to_int_px_ { $t_saos_ } = $t_int_px_;
    }
}

my $temp_dir_ = $HOME_DIR."/simtemp";
`mkdir -p $temp_dir_`;

my $t_unique_id_ = `date +%N`; $t_unique_id_ = $t_unique_id_ + 0;

my $t_mkt_data_file_ = $temp_dir_."/".$shortcode_.".".$yyyymmdd_.".".$t_unique_id_;

if ( index ( $shortcode_ , "BR_IND" ) >= 0 || index ( $shortcode_ , "BR_WIN" ) >= 0 || index ( $shortcode_ , "BR_WDO" ) >= 0 || index ( $shortcode_ , "BR_DOL" ) >= 0 )
{
    `$MKT_BOOK_EXEC SIM $shortcode_ $yyyymmdd_ NTP > $t_mkt_data_file_`;
}
else
{
    `$MKT_BOOK_EXEC SIM $shortcode_ $yyyymmdd_ > $t_mkt_data_file_`;
}

# For each cxled saos, look after the cxl. time in the mkt data
# to find an update which "most likely" contains our order.

foreach my $saos_ ( sort { $saos_to_cxl_time_ { $a } <=> $saos_to_cxl_time_ { $b } } 
		    keys %saos_to_cxl_time_ )
{
    my $t_cxl_time_ = $saos_to_cxl_time_ { $saos_ };
    my $t_size_ = $saos_to_size_ { $saos_ };
    my $t_buysell_ = $saos_to_buysell_ { $saos_ };
    my $t_int_px_ = $saos_to_int_px_ { $saos_ };

    # In the mkt data, we will search for "our" update,
    # starting 100 usecs before cxl. time and upto 6 msecs after cxl. time.
    my $t_search_time_start_ = $t_cxl_time_ + 0.000100; # Might be too little for EUREX, but too much for others.
    my $t_search_time_end_ = $t_cxl_time_ + 0.006000; # Might be too much for EUREX, but too little for others.

    my @affected_lines_ = ( );

    if ( $t_buysell_ eq "B" )
    {
	@affected_lines_ = `grep $t_int_px_ $t_mkt_data_file_ | awk '{ if ( \$1 >= $t_search_time_start_ && \$1 <= $t_search_time_end_ && \$6 == $t_int_px_ ) { print \$1\" \"\$4\" \"\$3\"\\n\"; } }'`;
    }
    else
    {
	@affected_lines_ = `grep $t_int_px_ $t_mkt_data_file_ | awk '{ if ( \$1 >= $t_search_time_start_ && \$1 <= $t_search_time_end_ && \$8 == $t_int_px_ ) { print \$1\" \"\$10\" \"\$11\"\\n\"; } }'`;
    }

    # Holds a list of likely update times.
    my @t_cxl_to_mkt_update_times_ = ( );

    my $prev_sz_ = 0;
    my $prev_oc_ = 0;

    for ( my $i = 0 ; $i <= $#affected_lines_ ; $i ++ )
    {
	my $t_line_ = $affected_lines_ [ $i ]; chomp ( $t_line_ );

	my @affected_words_ = split ( ' ' , $t_line_ );

	if ( $#affected_words_ < 2 )
	{
	    next;
	}

	my $t_time_ = $affected_words_ [ 0 ];
	my $t_oc_ = $affected_words_ [ 1 ];
	my $t_sz_ = $affected_words_ [ 2 ];

	if ( $prev_oc_ == 0 && $prev_sz_ == 0 )
	{
	    $prev_sz_ = $t_sz_;
	    $prev_oc_ = $t_oc_;
	    next;
	}

	my $t_expected_oc_ = $prev_oc_ - 1;
	my $t_expected_sz_ = $prev_sz_ - $t_size_;

	if ( $t_oc_ == $t_expected_oc_ && $t_sz_ == $t_expected_sz_ )
	{ 
	    # Order count went down by 1, and the size decreased by our order size.
	    # This could be our order.
	    push ( @t_cxl_to_mkt_update_times_ , ( $t_time_ - $t_cxl_time_ ) );
	}

	$prev_sz_ = $t_sz_;
	$prev_oc_ = $t_oc_;
    }

    if ( $#t_cxl_to_mkt_update_times_ >= 0 )
    {
	my $t_mean_ = GetAverage ( \@t_cxl_to_mkt_update_times_ );
	my $t_median_ = GetMedianConst ( \@t_cxl_to_mkt_update_times_ );

	$saos_to_mean_mkt_update_time_ { $saos_ } = $t_mean_;
	$saos_to_median_mkt_update_time_ { $saos_ } = $t_median_;

	if ( $t_mean_ != 0 || $t_median_ != 0 )
	{
	    print "$t_cxl_time_ $t_mean_ $t_median_ $saos_ $#t_cxl_to_mkt_update_times_\n";
	}
    }
}

`rm -f $t_mkt_data_file_`;
