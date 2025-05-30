#!/usr/bin/perl
use strict ;
use warnings ;
use feature "switch"; # for given, when
use File::Basename ;
use FileHandle;
use Scalar::Util qw(looks_like_number);

sub GetUnderlyingFromShortcode ; 
sub StatsForA;
sub StatsForI;
sub StatsForU;

#per contract
#replace tradesize with contract_delta*tradesize # none
#replace open_position with contract_delta*contract_open_position # none
#replace total_pnl with realized_pnl + open_pnl # contract_realized_pnl_
# run pnlstats on this file 

#per underlying
#replace tradesize with contract_delta*tradesize # none
#replace open_position with sum(contract_delta*contract_open_position) # each_contract_open_position, each_contract_latest_delta
#replace total_pnl with realized_pnl + open_pnl
# run pnlstats on this file



my $USAGE="$0 tradesfilename type=[(I)individual|(U)PerUnderlying|(A)All] ";
if ( $#ARGV < 0 ) { print "$USAGE\n"; exit ( 0 ); }
my $tradesfilename_ = $ARGV[0];
my $type_ = "I";
if ( $#ARGV >= 1 )
{
    $type_ = $ARGV[1];
}

my $tradesfilebase_ = basename ( $tradesfilename_ ); chomp ($tradesfilebase_);

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $USER=$ENV{'USER'};

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/basetrade_install/bin";

my $SPARE_HOME="/spare/local/".$USER."/";

my $pnl_tempdir=$SPARE_HOME."/pnltemp";

require "$GENPERLLIB_DIR/get_num_from_numsecname.pl";
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $delete_intermediate_files_ = 1;
my @intermediate_files_ = ();

if ( ! -d $pnl_tempdir )
{
    `mkdir -p $pnl_tempdir`;
}


my $date_ = `date -d @\`head -n1 $tradesfilename_ | awk '{ print \$1}'\` +\%Y\%m\%d`;
chomp($date_);
my $n2d_ = `/home/dvctrader/basetrade_install/bin/get_numbers_to_dollars NSE_NIFTY_FUT0 $date_`;
chomp($n2d_);

open PNLFILEHANDLE1, "< $tradesfilename_" or PrintStacktraceAndDie ( "$0 could not open tradesfile $tradesfilename_\n" );
open PNLFILEHANDLE2, "< $tradesfilename_" or PrintStacktraceAndDie ( "$0 could not open tradesfile $tradesfilename_\n" );

my %shcname_to_indfilename_map_ = ();
my %shcname_to_indfilehandle_map_ = ();
my %shcname_to_prev_position_map_ = ();
my %shcname_to_prev_delta_map_ = ();
my %shcname_to_prev_futprice_map_ = ();
my %shcname_to_total_pnl_map_ = ();
my %shc_2_simresult_struct_ = ();
my %shcname_to_lotsize_map_ = ();


my %shcname_to_underlying_map_ = ();

my %underlying_to_indfilename_map_ = ();
my %underlying_to_indfilehandle_map_ = ();
my %underlying_to_total_pnl_map_ = ();
my %underlying_to_prev_position_map_ = ();
my %underlying_to_lotsize_map_ = ();
my %underlying_to_num_contracts_ = ();
my %underlying_2_simresult_struct_ = ();

my %id_to_indfilename_map_ = ();
my %id_to_indfilehandle_map_ = ();
my %id_to_total_pnl_map_ = ();
my %id_to_prev_position_map_ = ();
my %id_2_simresult_struct_ = ();


#returns SBIN
sub GetUnderlyingFromShortcode
{
    my ( $this_shortcode_ ) = @_;
    my $this_underlying_ = 0;
    my @swords_ = split ( '_', $this_shortcode_ );
    if ( $#swords_ >= 1 )
    { # expect string like A.B
        $this_underlying_ = $swords_[1];
    }
    $this_underlying_;
}


while ( my $inline_ = <PNLFILEHANDLE1> )
{
    chomp ( $inline_ );
    my @pnlwords_ = split ( ' ', $inline_ );
    if ($#pnlwords_ >=11 && $pnlwords_[0] eq "STATS:")
    {
	my $numbered_shcname_ = $pnlwords_[1];
	$shcname_to_lotsize_map_{$numbered_shcname_}=int($pnlwords_[11]);

	my $shc_ = GetSecnameFromNumSecname ( $numbered_shcname_ );
	my $numbered_ = GetNumFromNumSecname ( $numbered_shcname_ );

	my $underlying_ = GetUnderlyingFromShortcode($shc_);
	$underlying_to_lotsize_map_{$underlying_.".".$numbered_} = int($pnlwords_[11]);
    }
}


while ( my $inline_ = <PNLFILEHANDLE2> )
{
    chomp ( $inline_ );
    my @pnlwords_ = split ( ' ', $inline_ );

    # we are justing collect S B I A for each secnumbered
    # we need to add thisgreek_totalpnl thisgreek_totalrisk ( dividing by lot_size (?))

#SIMRESULT 2897 NSE_SBIN_C0_A -520.799087 480 0 15 0 84 -520.799087
#Collect S B I A
    if ( $#pnlwords_ == 9 && $pnlwords_[0] eq "SIMRESULT" )
    {
	splice(@pnlwords_, -1); # remove min_pnl
	$shc_2_simresult_struct_{$pnlwords_[2].".".$pnlwords_[1]} = join(' ', $pnlwords_[5],
									 $pnlwords_[6],
									 $pnlwords_[7],
									 $pnlwords_[8]);

	my $underlying_ = GetUnderlyingFromShortcode($pnlwords_[2]);
	$underlying_2_simresult_struct_{$underlying_.".".$pnlwords_[1]} = join(' ', $pnlwords_[5],
									       $pnlwords_[6],
									       $pnlwords_[7],
									       $pnlwords_[8]);

	$id_2_simresult_struct_{$pnlwords_[1]} = join(' ', $pnlwords_[5],
					      $pnlwords_[6],
					      $pnlwords_[7],
					      $pnlwords_[8]);
    }

#STATS: NSE_SBIN_C0_A.2897 EOD_MSG_COUNT: 3537 PERCENT_MKT_VOL_TRADED: 0.149161 OTL_HIT: 0 UTS: 1 LOTSIZE: 3000
#collect lotsize
 #make trade file to run through stats_scripts
    if ( ( $#pnlwords_ >= 15 ) && ! ($pnlwords_[0] eq "STATS:") && ! ($pnlwords_[0] eq "SIMRESULT" ) && ! ($pnlwords_[0] eq "PNLSAMPLES" ) ) 
    {
	
	# time_stamp open/flat secname side trade_size contract_price contract_position {0 .. 6}
	# unrealized_pnl total_pnl {7 .. 8}
	# l1_book {9 .. 15}
	# underlying_risk underlying_pnl strategy_risk strategy_pnl {16 .. 19}
	# contract_delta contract_gamma contract_vega contract_theta futprice crontact_impliedvol {20 .. 25}

	my $numbered_shcname_ = $pnlwords_[2];
	my $number_ = GetNumFromNumSecname ( $numbered_shcname_ );
	my $shc_ = GetSecnameFromNumSecname ( $numbered_shcname_ );
# for I 
	my $underlying_ = GetUnderlyingFromShortcode($shc_);
	my $numbered_underlying_ = $underlying_.".".$number_;

	# position and trade size normalized by lotsize
	if ( ! ( exists $shcname_to_indfilehandle_map_{$numbered_shcname_} ) )
	{
	    my $indfilename_ = $pnl_tempdir."/pnl_".$numbered_shcname_."_".$tradesfilebase_;
	    push ( @intermediate_files_, $indfilename_ );
	    my $indfilehandle_ = FileHandle->new;
	    $indfilehandle_->open ( "> $indfilename_" );

	    $shcname_to_indfilename_map_{$numbered_shcname_} = $indfilename_;
	    $shcname_to_indfilehandle_map_{$numbered_shcname_} = $indfilehandle_;
	    $shcname_to_prev_position_map_{$numbered_shcname_} = 0;
	    $shcname_to_prev_delta_map_{$numbered_shcname_} = 0;
	    $shcname_to_prev_futprice_map_{$numbered_shcname_} = 0;
	    $shcname_to_total_pnl_map_{$numbered_shcname_} = 0;
	    $shcname_to_underlying_map_{$numbered_shcname_} = $underlying_;
	}
	if ( ! ( exists $underlying_to_indfilehandle_map_{$numbered_underlying_} ) )
	{
	    my $indfilename_ = $pnl_tempdir."/pnl_".$numbered_underlying_."_".$tradesfilebase_;
	    push ( @intermediate_files_, $indfilename_ );
	    my $indfilehandle_ = FileHandle->new;
	    $indfilehandle_->open ( "> $indfilename_" );

	    $underlying_to_indfilename_map_{$numbered_underlying_} = $indfilename_;
	    $underlying_to_indfilehandle_map_{$numbered_underlying_} = $indfilehandle_;
	    $underlying_to_prev_position_map_{$numbered_underlying_} = 0;
	    $underlying_to_total_pnl_map_{$numbered_underlying_} = 0;
	}
	if ( ! ( exists $id_to_indfilehandle_map_{$number_} ) )
	{
	    my $indfilename_ = $pnl_tempdir."/pnl_".$number_."_".$tradesfilebase_;
	    push ( @intermediate_files_, $indfilename_ );
	    my $indfilehandle_ = FileHandle->new;
	    $indfilehandle_->open ( "> $indfilename_" );

	    $id_to_indfilename_map_{$number_} = $indfilename_;
	    $id_to_indfilehandle_map_{$number_} = $indfilehandle_;
	    $id_to_prev_position_map_{$number_} = 0; # beta adjusted ( equal weighted for now)
	    $id_to_total_pnl_map_{$number_} = 0;
	}

	if ( substr($shc_, -4) eq "FUT0" )
	{
	    $shcname_to_total_pnl_map_{$numbered_shcname_} += $shcname_to_prev_position_map_{$numbered_shcname_} *
		($pnlwords_[5] - $shcname_to_prev_futprice_map_{$numbered_shcname_}) *
		(1) * $n2d_;
	    $underlying_to_total_pnl_map_{$numbered_underlying_} += $shcname_to_prev_position_map_{$numbered_shcname_} *
		($pnlwords_[5] - $shcname_to_prev_futprice_map_{$numbered_shcname_}) *
		(1) * $n2d_;
	    $id_to_total_pnl_map_{$number_} += $shcname_to_prev_position_map_{$numbered_shcname_} *
		($pnlwords_[5] - $shcname_to_prev_futprice_map_{$numbered_shcname_}) *
		(1) * $n2d_;

	    my $current_position_ = $pnlwords_[6];

	    #changing three items to run pnl_stats scripts
	    # same for all three
	    $pnlwords_[4] = abs(1 * $pnlwords_[4]);

	    # delta adjusted
	    $pnlwords_[6] = (1 * $pnlwords_[6]);
	    $pnlwords_[8] = $shcname_to_total_pnl_map_{$numbered_shcname_};
	    my $indfilehandle_ = $shcname_to_indfilehandle_map_{$numbered_shcname_};
	    print $indfilehandle_ join ( ' ', @pnlwords_ )."\n";


	    # delta sum adjusted 
	    # adjusted added contract position with previous contract position
	    $pnlwords_[6] += $underlying_to_prev_position_map_{$numbered_underlying_} - $shcname_to_prev_delta_map_{$numbered_shcname_} * $shcname_to_prev_position_map_{$numbered_shcname_} ;
	    $pnlwords_[8] = $underlying_to_total_pnl_map_{$numbered_underlying_};
	    $indfilehandle_ = $underlying_to_indfilehandle_map_{$numbered_underlying_};
	    print $indfilehandle_ join ( ' ', @pnlwords_ )."\n";

	    my $current_position_underlying_ = $pnlwords_[6];

	    # additional step to normalize
	    $pnlwords_[4] = int ( $pnlwords_[4]/$underlying_to_lotsize_map_{$numbered_underlying_} * 10000 ) / 10000;
	    $pnlwords_[6] = int ( $pnlwords_[6]/$underlying_to_lotsize_map_{$numbered_underlying_} * 10000 ) / 10000;
	    # beta + delta sum adjusted
	    # adjusted added underlying position with previous underlying position
	    $pnlwords_[6] += $id_to_prev_position_map_{$number_} - int( $underlying_to_prev_position_map_{$numbered_underlying_} / $underlying_to_lotsize_map_{$numbered_underlying_} * 10000 ) / 10000;
	    $pnlwords_[8] = $id_to_total_pnl_map_{$number_};
	    $indfilehandle_ = $id_to_indfilehandle_map_{$number_};
	    print $indfilehandle_ join ( ' ', @pnlwords_)."\n";

	    $shcname_to_prev_delta_map_{$numbered_shcname_} = 1;
	    $shcname_to_prev_futprice_map_{$numbered_shcname_} = $pnlwords_[5];
	    $shcname_to_prev_position_map_{$numbered_shcname_} = $current_position_;
	    $underlying_to_prev_position_map_{$numbered_underlying_} = $current_position_underlying_;
	    $underlying_to_prev_position_map_{$number_} = $pnlwords_[6];

	} else {
	    $shcname_to_total_pnl_map_{$numbered_shcname_} += $shcname_to_prev_position_map_{$numbered_shcname_} *
		($pnlwords_[24] - $shcname_to_prev_futprice_map_{$numbered_shcname_}) *
		($pnlwords_[20]) * $n2d_;
	    $underlying_to_total_pnl_map_{$numbered_underlying_} += $shcname_to_prev_position_map_{$numbered_shcname_} *
		($pnlwords_[24] - $shcname_to_prev_futprice_map_{$numbered_shcname_}) *
		($pnlwords_[20]) * $n2d_;
	    $id_to_total_pnl_map_{$number_} += $shcname_to_prev_position_map_{$numbered_shcname_} *
		($pnlwords_[24] - $shcname_to_prev_futprice_map_{$numbered_shcname_}) *
		($pnlwords_[20]) * $n2d_;

	    my $current_position_ = $pnlwords_[6];

	    #changing three items to run pnl_stats scripts
	    $pnlwords_[4] = abs($pnlwords_[20] * $pnlwords_[4]);


	    $pnlwords_[6] = $pnlwords_[20] * $pnlwords_[6];
	    $pnlwords_[8] = $shcname_to_total_pnl_map_{$numbered_shcname_};
	    my $indfilehandle_ = $shcname_to_indfilehandle_map_{$numbered_shcname_};
	    print $indfilehandle_ join ( ' ', @pnlwords_ )."\n";


	    $pnlwords_[6] += $underlying_to_prev_position_map_{$numbered_underlying_} - $shcname_to_prev_delta_map_{$numbered_shcname_} * $shcname_to_prev_position_map_{$numbered_shcname_};
	    $pnlwords_[8] = $underlying_to_total_pnl_map_{$numbered_underlying_};
	    $indfilehandle_ = $underlying_to_indfilehandle_map_{$numbered_underlying_};
	    print $indfilehandle_ join ( ' ', @pnlwords_ )."\n";
	    
	    my $current_underlying_position_ = $pnlwords_[6];
	    # additional step to normalize
	    $pnlwords_[4] = int ( $pnlwords_[4]/$underlying_to_lotsize_map_{$numbered_underlying_} * 10000 ) / 10000;
	    $pnlwords_[6] = int ( $pnlwords_[6]/$underlying_to_lotsize_map_{$numbered_underlying_} * 10000 ) / 10000;

	    $pnlwords_[6] += $id_to_prev_position_map_{$number_} - int( $underlying_to_prev_position_map_{$numbered_underlying_}/$underlying_to_lotsize_map_{$numbered_underlying_} * 10000 ) / 10000;
	    $pnlwords_[8] = $id_to_total_pnl_map_{$number_};
	    $indfilehandle_ = $id_to_indfilehandle_map_{$number_};
	    print $indfilehandle_ join(' ', @pnlwords_)."\n";

	    $shcname_to_prev_delta_map_{$numbered_shcname_} = $pnlwords_[20];
	    $shcname_to_prev_futprice_map_{$numbered_shcname_} = $pnlwords_[24];
	    $shcname_to_prev_position_map_{$numbered_shcname_} = $current_position_;
	    $underlying_to_prev_position_map_{$numbered_underlying_} = $current_underlying_position_;
	    $id_to_prev_position_map_{$number_} = $pnlwords_[6];	    
	}
    }
}


# close files
close PNLFILEHANDLE1;
close PNLFILEHANDLE2;
{
    foreach my $numbered_shcname_ ( keys %shcname_to_indfilehandle_map_ )
    {
	$shcname_to_indfilehandle_map_{$numbered_shcname_}->close ;
    }

    foreach my $numbered_underlying_ ( keys %underlying_to_indfilehandle_map_ )
    {
	$underlying_to_indfilehandle_map_{$numbered_underlying_}->close ;
    }
    foreach my $number_ ( keys %id_to_indfilehandle_map_ )
    {
	$id_to_indfilehandle_map_{$number_}->close ;
    }

}

given ( $type_ )
{
    when ("I")
    {
	StatsForI(1);
    }
    when ("U")
    {
	StatsForU(1);
    }
    when ("A")
    {
	StatsForA(1);
    }
    when ("X")
    {
	StatsForI(1);
	StatsForU(1);
	StatsForA(1);
    }
}

sub StatsForI    
{
    my ( $print_ ) = @_;

    foreach my $numbered_shcname_ ( keys %shcname_to_indfilename_map_ ) 
    {
	my $this_pnl_filename_ = $shcname_to_indfilename_map_{$numbered_shcname_};

	#avg_abs_position # divide by lot_size
	#median_ttc #nothing
	#avg_ttc #nothing
	#median_closed_trade_pnls #nothing
	#average_closed_trade_pnls #nothing
	#stdev_closed_trade_pnls #nothing
	#sharpe_closed_trade_pnls #nothing
	#fracpos_closed_trade_pnls #nothing
	#min_pnl #nothing
	#max_pnl #nothing
	#max_drawdown #nothing
	#max_ttc #nothing
	#navg_ttc #nothing
	#last_abs_pos #divide by lot size
	#pt #nothing
	#tt #nothing

	my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats.pl $this_pnl_filename_ 1";
	my $inline1_ = `$exec_cmd`;
	my @stats1_ = split(' ', $inline1_);
	$stats1_[0] = int($stats1_[0]/$shcname_to_lotsize_map_{$numbered_shcname_}*100)/100;
	$stats1_[-3] = int($stats1_[-3]/$shcname_to_lotsize_map_{$numbered_shcname_}*100)/100;
	#12 should be MSG_COUNT {same as total, set to 0}
	#14 should be OTL_HITS {we are not monitoring, set to 0}
	#16 should be MKT_VOL {set to 0}
	#19 should be FILL_RATIO {set to 0}
	if ( $#stats1_ < 12 ) {
	    push ( @stats1_ , 0 );
	} else {
	    splice @stats1_, 12, 0, 0;
	}
	if ( $#stats1_ < 14 ) {
	    push ( @stats1_ , 0 );
	} else {
	    splice @stats1_, 14, 0, 0;
	}
	if ( $#stats1_ < 16 ) {
	    push ( @stats1_ , 0 );
	} else {
	    splice @stats1_, 16, 0, 0;
	}
	if ( $#stats1_ < 18 ) {
	    push ( @stats1_ , 0 );
	} else {
	    splice @stats1_, 19, 0, 0;
	}
	

	$inline1_ = join(' ', @stats1_);
	my $number_ = GetNumFromNumSecname ( $numbered_shcname_ );
	my $shc_ = GetSecnameFromNumSecname ( $numbered_shcname_ );

	#final_pnl
	#final_volume
	#num_trades
	#average_abs_position
	#median_ttc
	#max_dd

	$exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats.pl $this_pnl_filename_ 2";
	my $inline2_ = `$exec_cmd`;
	chomp($inline2_);
	

	my @stats2_ = split(' ', $inline2_);
	print join(' ', $number_, $shc_."_DELTA", $stats2_[0], int($stats2_[1]/$shcname_to_lotsize_map_{$numbered_shcname_}*100)/100, $shc_2_simresult_struct_{$numbered_shcname_}, $inline1_);
	print "\n";
    }
}


sub StatsForU
{
    my ( $print_ ) = @_;

    foreach my $numbered_underlying_ ( keys %underlying_to_indfilename_map_ ) 
    {
	my $this_pnl_filename_ = $underlying_to_indfilename_map_{$numbered_underlying_};

	#avg_abs_position # divide by lot_size
	#median_ttc #nothing
	#avg_ttc #nothing
	#median_closed_trade_pnls #nothing
	#average_closed_trade_pnls #nothing
	#stdev_closed_trade_pnls #nothing
	#sharpe_closed_trade_pnls #nothing
	#fracpos_closed_trade_pnls #nothing
	#min_pnl #nothing
	#max_pnl #nothing
	#max_drawdown #nothing
	#max_ttc #nothing
	#navg_ttc #nothing
	#last_abs_pos #divide by lot size
	#pt #nothing
	#tt #nothing

	my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats.pl $this_pnl_filename_ 1";
	my $inline1_ = `$exec_cmd`;
	my @stats1_ = split(' ', $inline1_);
	$stats1_[0] = int($stats1_[0]/$underlying_to_lotsize_map_{$numbered_underlying_}*100)/100;
	$stats1_[-3] = int($stats1_[-3]/$underlying_to_lotsize_map_{$numbered_underlying_}*100)/100;
	#12 should be MSG_COUNT {same as total, set to 0}
	#14 should be OTL_HITS {we are not monitoring, set to 0}
	#16 should be MKT_VOL {set to 0}
	#19 should be FILL_RATIO {set to 0}
	if ( $#stats1_ < 12 ) {
	    push ( @stats1_ , 0 );
	} else {
	    splice @stats1_, 12, 0, 0;
	}
	if ( $#stats1_ < 14 ) {
	    push ( @stats1_ , 0 );
	} else {
	    splice @stats1_, 14, 0, 0;
	}
	if ( $#stats1_ < 16 ) {
	    push ( @stats1_ , 0 );
	} else {
	    splice @stats1_, 16, 0, 0;
	}
	if ( $#stats1_ < 18 ) {
	    push ( @stats1_ , 0 );
	} else {
	    splice @stats1_, 19, 0, 0;
	}
	

	$inline1_ = join(' ', @stats1_);
	my $number_ = GetNumFromNumSecname ( $numbered_underlying_ );
	my $shc_ = GetSecnameFromNumSecname ( $numbered_underlying_ );

	#final_pnl
	#final_volume
	#num_trades
	#average_abs_position
	#median_ttc
	#max_dd

	$exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats.pl $this_pnl_filename_ 2";
	my $inline2_ = `$exec_cmd`;
	chomp($inline2_);
	

	my @stats2_ = split(' ', $inline2_);
	print join(' ', $number_, $shc_."_DELTA", $stats2_[0], int($stats2_[1]/$underlying_to_lotsize_map_{$numbered_underlying_}*100)/100, $underlying_2_simresult_struct_{$numbered_underlying_}, $inline1_);
	print "\n";
    }
}


sub StatsForA
{
    my ( $print_ ) = @_;

    foreach my $number_ ( keys %id_to_indfilename_map_ ) 
    {
	my $this_pnl_filename_ = $id_to_indfilename_map_{$number_};

	#avg_abs_position # divide by lot_size
	#median_ttc #nothing
	#avg_ttc #nothing
	#median_closed_trade_pnls #nothing
	#average_closed_trade_pnls #nothing
	#stdev_closed_trade_pnls #nothing
	#sharpe_closed_trade_pnls #nothing
	#fracpos_closed_trade_pnls #nothing
	#min_pnl #nothing
	#max_pnl #nothing
	#max_drawdown #nothing
	#max_ttc #nothing
	#navg_ttc #nothing
	#last_abs_pos #divide by lot size
	#pt #nothing
	#tt #nothing

	my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats.pl $this_pnl_filename_ 1";
	my $inline1_ = `$exec_cmd`;
	my @stats1_ = split(' ', $inline1_);
	#12 should be MSG_COUNT {same as total, set to 0}
	#14 should be OTL_HITS {we are not monitoring, set to 0}
	#16 should be MKT_VOL {set to 0}
	#19 should be FILL_RATIO {set to 0}
	if ( $#stats1_ < 12 ) {
	    push ( @stats1_ , 0 );
	} else {
	    splice @stats1_, 12, 0, 0;
	}
	if ( $#stats1_ < 14 ) {
	    push ( @stats1_ , 0 );
	} else {
	    splice @stats1_, 14, 0, 0;
	}
	if ( $#stats1_ < 16 ) {
	    push ( @stats1_ , 0 );
	} else {
	    splice @stats1_, 16, 0, 0;
	}
	if ( $#stats1_ < 18 ) {
	    push ( @stats1_ , 0 );
	} else {
	    splice @stats1_, 19, 0, 0;
	}
	

	$inline1_ = join(' ', @stats1_);

	#final_pnl
	#final_volume
	#num_trades
	#average_abs_position
	#median_ttc
	#max_dd

	$exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats.pl $this_pnl_filename_ 2";
	my $inline2_ = `$exec_cmd`;
	chomp($inline2_);
	

	my @stats2_ = split(' ', $inline2_);
	print join(' ', $number_, "ALL_DELTA", $stats2_[0], $stats2_[1], $id_2_simresult_struct_{$number_}, $inline1_);
	print "\n";
    }
}



if ( $delete_intermediate_files_ == 1 )
{
    for ( my $i = 0 ; $i <= $#intermediate_files_; $i ++ )
    {
	if ( -e $intermediate_files_[$i] )
	{
#	    print $intermediate_files_[$i]."\n";
	    `rm -f $intermediate_files_[$i]`;
	}
    }
}
