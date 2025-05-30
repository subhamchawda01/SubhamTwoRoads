#!/usr/bin/perl

# \file scripts/get_bad_periods.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
use strict;
use warnings;
use List::Util qw[min max]; # max , min

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

my $SHORTCODE = "";
my $YYYYMMDD = "";

my $PLOT_GRAPH = 0;
my $EMAIL_GRAPH = 0;

if ( $USER eq "sghosh" ) { $EMAIL_GRAPH = 1; }

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

require "$GENPERLLIB_DIR/get_real_query_id_prefix_for_shortcode_time_period.pl"; # GetRealQueryIdPrefixForShortcodeTimePeriod
require "$GENPERLLIB_DIR/get_trade_fields_from_trade_line.pl"; # Get*FromTradeLine

require "$GENPERLLIB_DIR/get_bad_periods_for_shortcode_for_date.pl"; #GetBadPeriodsForShortcodeForDate
require "$GENPERLLIB_DIR/no_data_date.pl"; #NoDataDateForShortcode 
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_shortcode_from_stratfile.pl"; # GetShortcodeFromStratFile

sub GetBadMfmForShortcodeForDate;
sub GetBadMfmForTradesFileList;

my $USAGE = "$0 shc strat_name output_files_dir days_to_look_behind";

if ( $#ARGV < 2 ) 
{ 
    printf "$USAGE\n"; 
    exit ( 0 ); 
}

my $shortcode_ = $ARGV[0];
my $strat_name_ = $ARGV[1];

my $output_files_dir_ = $ARGV [ 2 ];

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
$yyyymmdd_=CalcPrevWorkingDateMult($yyyymmdd_,1);
my $days_to_look_behind_ = $ARGV [ 3 ];

my $current_yyyymmdd_ = $yyyymmdd_;
my %date_to_bad_periods_ = ( );

print "shc: ".$shortcode_."\n";
for ( my $days_ = $days_to_look_behind_ ; $days_ != 0 ; $days_ -- ) 
{
    if ( ! ValidDate ( $current_yyyymmdd_ ) ) 
    { # Should not be here.
        print "Invalid date : $current_yyyymmdd_\n" ;
        last;
    }

    if ( SkipWeirdDate ( $current_yyyymmdd_ ) ||
        IsDateHoliday ( $current_yyyymmdd_ ) || 
            NoDataDateForShortcode ($current_yyyymmdd_, $shortcode_) ||
            IsProductHoliday ($current_yyyymmdd_, $shortcode_)) {
        $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
        $days_ ++;
        next;
    }

    # Get msecs for bad periods across all queries.
    $date_to_bad_periods_ { $current_yyyymmdd_ } = GetBadMfmForShortcodeForDate ( $shortcode_ , $current_yyyymmdd_ );

    $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
}

my $is_data_available_ = 0;
foreach my $date_ ( keys ( %date_to_bad_periods_ ) )
{
my @t_bad_mfm_ = @ { $date_to_bad_periods_ { $date_ } };

if ( $#t_bad_mfm_ >= 0 ) { $is_data_available_ = 1; last; }
}

if ( $is_data_available_ )
{
my $bad_period_file_name_ = $output_files_dir_."/"."bad_periods.".$shortcode_ ;
open ( BAD_PERIODS_FILE , "> $bad_period_file_name_ " ) or PrintStacktraceAndDie ( "Could not open file : $bad_period_file_name_\n" );

foreach my $date_ ( sort { $b <=> $a }
            keys ( %date_to_bad_periods_ ) )
{
    my @t_bad_mfm_ = @ { $date_to_bad_periods_ { $date_ } };

    if ( $#t_bad_mfm_ >= 0 )
    {
        print BAD_PERIODS_FILE $date_." ";

        for ( my $i = 0 ; $i <= $#t_bad_mfm_ ; $i += 2 )
        {
            print BAD_PERIODS_FILE $t_bad_mfm_ [ $i ]." ".$t_bad_mfm_ [ $i + 1 ]." ";
        }

        print BAD_PERIODS_FILE "\n";
    }
}

close ( BAD_PERIODS_FILE );
}

sub GetBadMfmForShortcodeForDate
{
    my ( $shortcode_ , $yyyymmdd_ ) = @_;

    # Check if already computed for this shortcode for this date.
    my ( $r_bad_periods_start_ , $r_bad_periods_end_ ) = GetBadPeriodsForShortcodeForDate ( $shortcode_ , $yyyymmdd_ );

    my @bad_periods_start_ = @$r_bad_periods_start_;
    my @bad_periods_end_ = @$r_bad_periods_end_;

    if ( $#bad_periods_start_ >= 0 ) # Already computed.
    {
#	print "Using precomputed bad-periods for $shortcode_ $yyyymmdd_\n";

	my @t_bad_mfm_ = ( );

	for ( my $i = 0 ; $i <= $#bad_periods_start_ ; $i ++ )
	{
	    push ( @t_bad_mfm_ , $bad_periods_start_ [ $i ] );
	    push ( @t_bad_mfm_ , $bad_periods_end_ [ $i ] );
	}

	return \@t_bad_mfm_;
    }

    my $t_yyyy_ = substr ( $yyyymmdd_ , 0 , 4 );
    my $t_mm_ = substr ( $yyyymmdd_ , 4 , 2 );
    my $t_dd_ = substr ( $yyyymmdd_ , 6 , 2 );
    my $exec_cmd_ = "";
    my $full_strat_path_ = $strat_name_;
    if ( -e $strat_name_ )
    {
        $full_strat_path_ = $strat_name_;
    }
    else
    {
        $exec_cmd_ = "$SCRIPTS_DIR/print_strat_from_base.sh $strat_name_ " ;
        $full_strat_path_ = `$exec_cmd_`; chomp ( $exec_cmd_ ) ;
    }
    
    my $id_ = `date +%N`; chomp ( $id_ ) ; $id_ = int ( $id_ ) ;
    $exec_cmd_ = "$BIN_DIR/sim_strategy SIM $full_strat_path_ $id_ $yyyymmdd_ 2 0 0.0 0 ADD_DBG_CODE -1 2>/dev/null";
    #print "$exec_cmd_\n";
    `$exec_cmd_` ;
    my $tradefilename_ = "/spare/local/logs/tradelogs/trades.$yyyymmdd_.$id_"; chomp ( $tradefilename_ ) ;
    #print " tradefle: ".$tradefilename_."\n";
    
    my @trades_file_list_ = () ;
    push ( @trades_file_list_, $tradefilename_ ) ;
    # To be used in GetBadMfmForTradesFileList
    $SHORTCODE = $shortcode_;
    $YYYYMMDD = $yyyymmdd_;

    my @t_bad_mfm_ = GetBadMfmForTradesFileList ( @trades_file_list_ );

    return \@t_bad_mfm_;
}

sub GetBadMfmForTradesFileList
{
    my ( @trades_file_list_ ) = @_;

    my @t_bad_times_ = ( );

    my @all_times_ = ( );

    my %time_to_pos_ = ( );
    my %time_to_pnl_ = ( );

    my %trades_file_to_time_to_pnl_ = ( );
    my %trades_file_to_time_to_pos_ = ( );

    for ( my $i = 0 ; $i <= $#trades_file_list_ ; $i ++ )
    {
        my $t_trades_file_ = $trades_file_list_ [ $i ];

        open ( TRADES_FILE , "< $t_trades_file_ " ) or PrintStacktraceAndDie ( "Could not open $t_trades_file_\n" );

        while ( my $trades_line_ = <TRADES_FILE> )
        {
            if ( not IsValidTradeLine ( $trades_line_) ) { next ; } 
            
            my $t_time_ = GetTradeTimeFromTradeLine ( $trades_line_ );

            my $t_pos_ = GetPosFromTradeLine ( $trades_line_ );
            my $t_pnl_ = GetPnlFromTradeLine ( $trades_line_ );

            if ( FindItemFromVec ( $t_time_ , @all_times_ ) ne $t_time_ ) { push ( @all_times_ , $t_time_ ); }

            $trades_file_to_time_to_pnl_ { $t_trades_file_ } { $t_time_ } = $t_pnl_;
            $trades_file_to_time_to_pos_ { $t_trades_file_ } { $t_time_ } = $t_pos_;
        }

        close ( TRADES_FILE );
        `rm -rf $t_trades_file_`;
    }

    # Need to create a time-series of position & pnl @ each time in @all_times_
    # merging position & pnls from all trades files.
    @all_times_ = sort ( @all_times_ );

    my %trades_file_to_last_pnl_ = ( );
    my %trades_file_to_last_pos_ = ( );

    foreach my $trades_file_ ( @trades_file_list_ )
    {
	$trades_file_to_last_pos_ { $trades_file_ } = 0;
	$trades_file_to_last_pnl_ { $trades_file_ } = 0;
    }

    my $max_abs_pos_ = 0;
    my $max_pnl_ = -1000000;
    my $min_pnl_ = 1000000;

    foreach my $time_ ( @all_times_ )
    {
        foreach my $trades_file_ ( @trades_file_list_ )
        {
            if ( exists ( $trades_file_to_time_to_pos_ { $trades_file_ } { $time_ } ) )
            { $trades_file_to_last_pos_ { $trades_file_ } = $trades_file_to_time_to_pos_ { $trades_file_ } { $time_ }; }
            else
            { $trades_file_to_time_to_pos_ { $trades_file_ } { $time_ } = $trades_file_to_last_pos_ { $trades_file_ }; }

            if ( exists ( $trades_file_to_time_to_pnl_ { $trades_file_ } { $time_ } ) )
            { $trades_file_to_last_pnl_ { $trades_file_ } = $trades_file_to_time_to_pnl_ { $trades_file_ } { $time_ }; }
            else
            { $trades_file_to_time_to_pnl_ { $trades_file_ } { $time_ } = $trades_file_to_last_pnl_ { $trades_file_ }; }

            $time_to_pos_ { $time_ } += $trades_file_to_last_pos_ { $trades_file_ };
            $time_to_pnl_ { $time_ } += $trades_file_to_last_pnl_ { $trades_file_ };

            $max_abs_pos_ = max ( $max_abs_pos_ , abs ( $time_to_pos_ { $time_ } ) );

            $max_pnl_ = max ( $max_pnl_ , $time_to_pnl_ { $time_ } );
            $min_pnl_ = min ( $min_pnl_ , $time_to_pnl_ { $time_ } );
        }
    }

    if ( $PLOT_GRAPH || $EMAIL_GRAPH )
    {
        `mkdir -p $HOME_DIR/stats`;
        my $t_stats_file_name_ = $HOME_DIR."/stats/".$SHORTCODE.".".$YYYYMMDD;
        open ( STATS_FILE , "> $t_stats_file_name_ " ) or PrintStacktraceAndDie ( "Could not open $t_stats_file_name_\n" );
        for ( my $i = 0 ; $i <= $#all_times_ ; $i ++ )
        {
            my $t_time_ = $all_times_ [ $i ];

            my $t_time_mfm_ = GetMfmFromUnixTimeDate ( $t_time_ , $YYYYMMDD );
            print STATS_FILE $t_time_mfm_." ".$time_to_pos_ { $t_time_ }." ".$time_to_pnl_ { $t_time_ }."\n";
    #	    print STATS_FILE $t_time_." ".$time_to_pos_ { $t_time_ }." ".$time_to_pnl_ { $t_time_ }."\n";
        }
        close ( STATS_FILE );
    }

    my $max_drop_ = ( $max_pnl_ - $min_pnl_ );

    # These maps only contain stats for bad segments.
    my @bad_period_starts_ = ( );
    my @bad_period_ends_ = ( );

    my $is_valid_drop_ = 0;
    my $start_time_index_ = 0;
    my $t_time_index_ = 0;

    for ( my $time_index_ = 0 ; $time_index_ <= $#all_times_ ; $time_index_ ++ )
    {
        my $time_ = $all_times_ [ $time_index_ ];
        my $start_time_ = $all_times_ [ $start_time_index_ ];
        my $t_time_ = $all_times_ [ $t_time_index_ ];
        my $pnl_ = $time_to_pnl_ { $time_ };

        if ( $is_valid_drop_ )
        {
            if ( $pnl_ <= $time_to_pnl_ { $t_time_ } ) { $t_time_ = $time_; $t_time_index_ = $time_index_; }
            else
            {
                if ( ( $pnl_ - $time_to_pnl_ { $t_time_ } ) > 0.06 * $max_drop_ )
                {
                    my $orig_drop_ = $time_to_pnl_ { $start_time_ } - $time_to_pnl_ { $t_time_ };

                    # Narrow the time period to the actual drop time.
                    for ( ; ( $time_to_pnl_ { $all_times_ [ $start_time_index_ ] } - $time_to_pnl_ { $t_time_ } ) >= ( 0.8 * $orig_drop_ ) ; $start_time_index_ ++ ) { }
                    $start_time_index_ --;

                    # Find a time before the start of the drop , where we were with an opposite position.
                    # Subsequently , the position we took leading upto the drop , was incorrect
                    # and a better model should avoid this.
                    my $pre_drop_pos_ = $time_to_pos_ { $all_times_ [ $start_time_index_ ] };
                    for ( ; $start_time_index_ >= 0 && $time_to_pos_ { $all_times_ [ $start_time_index_ ] } * $pre_drop_pos_ > 0 ; $start_time_index_ -- ) { }

                    $start_time_ = $all_times_ [ $start_time_index_ ];

                    if ( $start_time_ < $t_time_ )
                    {
                    push ( @bad_period_starts_ , $start_time_ );
                    push ( @bad_period_ends_ , $t_time_ );
                    }

                    $is_valid_drop_ = 0;
                }
            }
        }
        else
        {
            if ( $pnl_ >= $time_to_pnl_ { $t_time_ } ) { $t_time_ = $time_; $t_time_index_ = $time_index_; }
            else
            {
                if ( ( $time_to_pnl_ { $t_time_ } - $pnl_ ) > 0.175 * $max_drop_ )
                {
                    $start_time_ = $t_time_; $start_time_index_ = $t_time_index_;
                    $t_time_ = $time_; $t_time_index_ = $time_index_;
                    $is_valid_drop_ = 1;
                }
            }
        }
    }

    if ( $PLOT_GRAPH || $EMAIL_GRAPH )
    {
        if ( $#bad_period_starts_ >= 0 )
        {
            `mkdir -p $HOME_DIR/bad`;

            my $t_bad_file_name_ = $HOME_DIR."/bad/bad_periods.".$SHORTCODE.".".$YYYYMMDD;
            open ( BAD_FILE , "> $t_bad_file_name_ " ) or PrintStacktraceAndDie ( "Could not open $t_bad_file_name_\n" );

            my $last_bad_vec_index_ = 0;
            for ( my $i = 0 ; $i <= $#all_times_ ; $i ++ )
            {
                my $t_time_ = $all_times_ [ $i ];

                if ( $t_time_ >= $bad_period_starts_ [ $last_bad_vec_index_ ] &&
                     $t_time_ <= $bad_period_ends_ [ $last_bad_vec_index_ ] )
                {
                    my $t_time_mfm_ = GetMfmFromUnixTimeDate ( $t_time_ , $YYYYMMDD );
                    print BAD_FILE $t_time_mfm_." ".$time_to_pos_ { $t_time_ }." ".$time_to_pnl_ { $t_time_ }."\n";
        #		    print BAD_FILE $t_time_." ".$time_to_pos_ { $t_time_ }." ".$time_to_pnl_ { $t_time_ }."\n";
                    next;
                }

                if ( $t_time_ > $bad_period_ends_ [ $last_bad_vec_index_ ] ) 
                { 
                    $last_bad_vec_index_ ++; 
                    if ( $last_bad_vec_index_ > $#bad_period_starts_ ) { last; }
                }
            }

            close ( BAD_FILE );
        }

        my $t_stats_file_name_ = $HOME_DIR."/stats/".$SHORTCODE.".".$YYYYMMDD;
        my $t_bad_file_name_ = $HOME_DIR."/bad/bad_periods.".$SHORTCODE.".".$YYYYMMDD;

        if ( $PLOT_GRAPH )
        {
            my $PLOT_SCRIPT = $SCRIPTS_DIR."/plot_multifile_cols.pl";
            `$PLOT_SCRIPT $t_stats_file_name_ 3 $SHORTCODE.$YYYYMMDD.pnl WL $t_bad_file_name_ 3 $SHORTCODE.$YYYYMMDD.drops NL 2`;
            `$PLOT_SCRIPT $t_bad_file_name_ 2 $SHORTCODE.$YYYYMMDD.pos NL`;
            `$PLOT_SCRIPT $t_bad_file_name_ 3 $SHORTCODE.$YYYYMMDD.drops NL`;
        }

        if ( $EMAIL_GRAPH )
        {
            my $PLOT_SCRIPT = $SCRIPTS_DIR."/saveplot_multifile_cols.pl";

            `mkdir -p /home/sghosh/master/temp`;
            my $t_output_png_ = "/home/sghosh/master/temp/plot.".$SHORTCODE.".".$YYYYMMDD.".png";

            `$PLOT_SCRIPT $t_stats_file_name_ 3 $SHORTCODE.$YYYYMMDD.pnl WL $t_bad_file_name_ 3 $SHORTCODE.$YYYYMMDD.drops NL $t_output_png_ 2>/dev/null`;

            if ( ! ExistsWithSize ( $t_output_png_ ) ) { `rm -f $t_output_png_`; }
        }

        `rm -rf $HOME_DIR/stats`;
        `rm -rf $HOME_DIR/bad`;
    }

    for ( my $bad_period_index_ = 0 ; $bad_period_index_ <= $#bad_period_starts_ ; $bad_period_index_ ++ )
    {
	my $start_mfm_ = GetMfmFromUnixTimeDate ( $bad_period_starts_ [ $bad_period_index_ ] , $YYYYMMDD );
#	my $start_mfm_ = $bad_period_starts_ [ $bad_period_index_ ];

	push ( @t_bad_times_ , $start_mfm_ );

	my $end_mfm_ = GetMfmFromUnixTimeDate ( $bad_period_ends_ [ $bad_period_index_ ] , $YYYYMMDD );
#	my $end_mfm_ = $bad_period_ends_ [ $bad_period_index_ ];

	push ( @t_bad_times_ , $end_mfm_ );
    }

    return @t_bad_times_;
}

sub GetMfmFromUnixTimeDate
{
    my ( $unix_time_ , $yyyymmdd_ ) = @_;

    my $MFM_FROM_UTC = $BIN_DIR."/get_mfm_from_utc_time";
    my $mfm_exec_ = $MFM_FROM_UTC." ".$unix_time_." ".$yyyymmdd_;

    my $mfm_ = `$mfm_exec_`; chomp ( $mfm_ );

    return $mfm_;
}
