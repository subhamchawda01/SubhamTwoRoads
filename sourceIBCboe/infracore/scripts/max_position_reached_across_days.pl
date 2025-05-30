#!/usr/bin/perl

use strict;
use warnings;

my $USER=$ENV{"USER"};
my $HOME=$ENV{"HOME"};

my $REPO = "infracore";

my $GENPERLLIB_DIR = "$HOME/$REPO/GenPerlLib";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktraceAndDie

my $BASE_TRADE_DIR = "/NAS1/logs/ORSTrades";

my $USAGE="$0 END_DATE NUM_OF_PAST_DAYS";
if( $#ARGV < 1 )
{
    print "USAGE:$USAGE\n";
    exit( 0 );
}

my $end_date_ = $ARGV[ 0 ];
if( $end_date_ eq "TODAY" ) { $end_date_ = `date +%Y%m%d`; }
my $num_of_days_ = $ARGV[ 1 ];
my %shortcode_to_pos_= ();
my %shortcode_to_max_pos_ = ();

my @exchange_list_ = ("EUREX", "BMFEP", "CME", "TMX", "LIFFE");

my $t_date_ = $end_date_;
for( my $iter_ = 0; $iter_ < $num_of_days_; $iter_++ )
{
    while( (!ValidDate($t_date_)) || SkipWeirdDate($t_date_) || IsDateHoliday($t_date_) )
    {
	$t_date_ = CalcPrevWorkingDateMult( $t_date_, 1 ); 
    }
foreach my $exchange_ ( @exchange_list_ )
{
    my $exchange_trades_dir_ = "$BASE_TRADE_DIR/$exchange_";

    if ( -d $exchange_trades_dir_ )
    {
	if( opendir my $dh, $exchange_trades_dir_ )
	{
	    my @t_list_ = ();
	    while( my $t_item_ = readdir $dh )
	    {
		next if( $t_item_ eq "." || $t_item_ eq ".." );
		push( @t_list_, $t_item_ );
	    }
	    closedir $dh;
	    foreach my $session_ ( @t_list_ )
	    {
		my $trade_file_ = "$exchange_trades_dir_/$session_/".substr( $t_date_ , 0 , 4 )."/".substr( $t_date_ , 4 , 2 )."/".substr( $t_date_ , 6 , 2 )."/trades.$t_date_";
#print "looking for file: $trade_file_\n";
		if( ExistsWithSize ( $trade_file_ ) )
		{
		    open( TRADE_FILE, "<", $trade_file_ ) or PrintStacktraceAndDie( "Could Not Open $trade_file_\n" );
		    my @trade_lines_ = <TRADE_FILE>;
		    chomp( @trade_lines_ );
		    foreach my $trade_line_( @trade_lines_ )
		    {
			my @trade_line_words_ = split( '\001', $trade_line_ );
			my $t_shortcode_ = $trade_line_words_[ 0 ];
			if( ! exists( $shortcode_to_pos_{$t_shortcode_} ) )
			{
			    $shortcode_to_pos_{ $t_shortcode_ } = 0;
			    $shortcode_to_max_pos_{ $t_shortcode_ } = 0;
			}
			if( $trade_line_words_[ 1 ] == 0 ){ $shortcode_to_pos_{ $t_shortcode_ } += $trade_line_words_[ 2 ]; }
			if( $trade_line_words_[ 1 ] == 1 ){ $shortcode_to_pos_{ $t_shortcode_ } -= $trade_line_words_[ 2 ]; }
			if( abs( $shortcode_to_pos_{ $t_shortcode_ } ) > $shortcode_to_max_pos_{ $t_shortcode_ } )
			{
			    $shortcode_to_max_pos_{ $t_shortcode_ } = abs( $shortcode_to_pos_{ $t_shortcode_ } );
			}
		    }
		}
	    }
	}
    }
}
foreach my $t_shortcode_ ( keys %shortcode_to_pos_ )
{
    $shortcode_to_pos_{ $t_shortcode_ } = 0;
}
$t_date_ = CalcPrevWorkingDateMult( $t_date_, 1 );
}

foreach my $t_shortcode_ ( keys %shortcode_to_max_pos_ )
{
    printf "Shortcode: %16s\tMAX_POS: $shortcode_to_max_pos_{ $t_shortcode_ }\n", $t_shortcode_;
}
