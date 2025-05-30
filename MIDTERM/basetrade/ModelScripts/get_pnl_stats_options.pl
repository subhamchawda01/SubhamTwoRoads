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

my $USAGE="$0 tradesfilename type=[(I)individual|(U)PerUnderlying|(A)All] ";
if ( $#ARGV < 0 ) { print "$USAGE\n"; exit ( 0 ); }
my $tradesfilename_ = $ARGV[0];
my $type_ = "I";
if ( $#ARGV >= 1 )
{
    $type_ = $ARGV[1];
}
my $targetcol = 9;

my $tradesfilebase_ = basename ( $tradesfilename_ ); chomp ($tradesfilebase_);

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";
my $USER=$ENV{'USER'};

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $SPARE_HOME="/spare/local/".$USER."/";

my $pnl_tempdir=$SPARE_HOME."/pnltemp";

require "$GENPERLLIB_DIR/get_num_from_numsecname.pl";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $delete_intermediate_files_ = 1;
my @intermediate_files_ = ();
my $pnl_all_file_ = $pnl_tempdir."/pnl_all_".$tradesfilebase_;
push ( @intermediate_files_, $pnl_all_file_ );

if ( ! -d $pnl_tempdir )
{
    `mkdir -p $pnl_tempdir`;
}

open PNLFILEHANDLE, "< $tradesfilename_" or PrintStacktraceAndDie ( "$0 could not open tradesfile $tradesfilename_\n" );

my %shcname_to_indfilename_map_ = ();
my %shcname_to_indfilehandle_map_ = ();
my %shcname_to_pnl_map_ = ();
my %shcname_to_pos_map_ = ();
my %shcname_to_mkt_vol_traded_map_ = ();
my %shcname_to_msgs_map_ = ();
my %shcname_to_uts_map_ = ();
my %shcname_to_otlhit_map_ = ();
my %shcname_to_lotsize_map_ = ();

my %underlying_to_indfilename_map_ = ();
my %underlying_to_indfilehandle_map_ = ();
my %underlying_to_pnl_map_ = ();
my %underlying_to_pos_map_ = ();
my %underlying_to_mkt_vol_traded_map_ = ();
my %underlying_to_msgs_map_ = ();
my %underlying_to_uts_map_ = ();
my %underlying_to_otlhit_map_ = ();
my %underlying_to_lotsize_map_ = ();
my %underlying_to_num_contracts_ = ();
my %underlying_to_pnl_samples_ = ();

my %runtime_id_to_simresult_map_ = ();

my %id_to_indfilename_map_ = ();
my %id_to_indfilehandle_map_ = ();
my %id_to_pnl_map_ = ();
my %id_to_pos_map_ = ();
my %id_to_msgs_map_ = ();
my %id_to_otlhit_map_ = ();
my %id_to_mkt_vol_traded_map_ = ();
my %id_to_pnl_samples_ = ();

my %shc_2_simresult_ = ();
my %shc_2_pnl_stats_ = ();


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



while ( my $inline_ = <PNLFILEHANDLE> )
{
    chomp ( $inline_ );
    my @pnlwords_ = split ( ' ', $inline_ );

    if ( $#pnlwords_ == 9 && $pnlwords_[0] eq "SIMRESULT" )
    {
	splice(@pnlwords_, -1); # remove min_pnl
	$shc_2_simresult_{$pnlwords_[1]." ".$pnlwords_[2]} = join(' ',$pnlwords_[3],
								  $pnlwords_[4],
								  $pnlwords_[5],
								  $pnlwords_[6],
								  $pnlwords_[7],
								  $pnlwords_[8]);
    }

    if ( ( $#pnlwords_ >= 8 ) && ! ($pnlwords_[0] eq "STATS:") && ! ($pnlwords_[0] eq "SIMRESULT" ) && ! ($pnlwords_[0] eq "PNLSAMPLES" ) ) 
    {
	my $numbered_shcname_ = $pnlwords_[2];
	my $pos_ = $pnlwords_[6]; 
	my $pnl_ = $pnlwords_[8];
	my $number_ = GetNumFromNumSecname ( $numbered_shcname_ );
        my $shc_ = GetSecnameFromNumSecname ( $numbered_shcname_ );
        my $underlying_ = GetUnderlyingFromShortcode($shc_);
        my $numbered_underlying_ = $underlying_.".".$number_;
	if (! (looks_like_number($pos_) && looks_like_number($pnl_)) ) { 
	    print STDERR  "Malformed Tradefile: $tradesfilename_ \nline: $inline_\n";
	    my $logfilename_ = $tradesfilename_ =~ s/trades/log/g;
	    `cp $tradesfilename_ $logfilename_ /spare/local/`;
	    next;
	}
	if ( ! ( exists $shcname_to_indfilehandle_map_{$numbered_shcname_} ) )
	{
	    my $indfilename_ = $pnl_tempdir."/pnl_".$numbered_shcname_."_".$tradesfilebase_;
	    push ( @intermediate_files_, $indfilename_ );
	    my $indfilehandle_ = FileHandle->new;
	    $indfilehandle_->open ( "> $indfilename_" );

	    $shcname_to_indfilename_map_{$numbered_shcname_} = $indfilename_;
	    $shcname_to_indfilehandle_map_{$numbered_shcname_} = $indfilehandle_;
	    $shcname_to_pnl_map_{$numbered_shcname_} = 0;
	    $shcname_to_pos_map_{$numbered_shcname_} = 0;
	}

	if ( ! ( exists $underlying_to_indfilename_map_{$numbered_underlying_} ) )
	{
	    my $indfilename_ = $pnl_tempdir."/pnl_".$numbered_underlying_."_".$tradesfilebase_;
	    push ( @intermediate_files_, $indfilename_ );
	    my $indfilehandle_ = FileHandle->new;
	    $indfilehandle_->open ( "> $indfilename_" );

	    $underlying_to_indfilename_map_{$numbered_underlying_} = $indfilename_;
	    $underlying_to_indfilehandle_map_{$numbered_underlying_} = $indfilehandle_;
	    $underlying_to_pnl_map_{$numbered_underlying_} = 0;
	    $underlying_to_pos_map_{$numbered_underlying_} = 0;
	}

	if ( ! ( exists $id_to_indfilename_map_{$number_} ) )
	{
	    my $indfilename_ = $pnl_tempdir."/pnl_".$number_."_".$tradesfilebase_;
	    push ( @intermediate_files_, $indfilename_ );
	    my $indfilehandle_ = FileHandle->new;
	    $indfilehandle_->open ( "> $indfilename_" );

	    $id_to_indfilename_map_{$number_} = $indfilename_;
	    $id_to_indfilehandle_map_{$number_} = $indfilehandle_;
	    $id_to_pnl_map_{$number_} = 0;
	    $id_to_pos_map_{$number_} = 0;
	}

	my $indfilehandle_ = $shcname_to_indfilehandle_map_{$numbered_shcname_};
	print $indfilehandle_ "$inline_\n";

	$id_to_pnl_map_{$number_} += $pnl_ - $shcname_to_pnl_map_{$numbered_shcname_};
        $underlying_to_pnl_map_{$numbered_underlying_} += $pnl_ - $shcname_to_pnl_map_{$numbered_shcname_};
	$shcname_to_pnl_map_{$numbered_shcname_} = $pnl_;

	$id_to_pos_map_{$number_} = $pnlwords_[18]; # Here position in terms of nifty can be written (new column in tradefile)
        $underlying_to_pos_map_{$numbered_underlying_} = $pnlwords_[16];
	$shcname_to_pos_map_{$numbered_shcname_} = $pos_;

        $indfilehandle_ = $underlying_to_indfilehandle_map_{$numbered_underlying_};
	$pnlwords_[8] = $underlying_to_pnl_map_{$numbered_underlying_};
	$pnlwords_[6] = $underlying_to_pos_map_{$numbered_underlying_};
	print $indfilehandle_ join ( ' ', @pnlwords_ )."\n";

        $indfilehandle_ = $id_to_indfilehandle_map_{$number_};
	$pnlwords_[8] = $id_to_pnl_map_{$number_};
	$pnlwords_[6] = $id_to_pos_map_{$number_};
	print $indfilehandle_ join ( ' ', @pnlwords_ )."\n";
    }
    else {
	if ($#pnlwords_ >=11 && $pnlwords_[0] eq "STATS:")
	{
	    my $numbered_shcname_ = $pnlwords_[1];
            my $number_ = GetNumFromNumSecname ( $numbered_shcname_ );
            my $shc_ = GetSecnameFromNumSecname ( $numbered_shcname_ );
            my $underlying_ = GetUnderlyingFromShortcode($shc_);
            my $numbered_underlying_ = $underlying_.".".$number_;

	    $shcname_to_mkt_vol_traded_map_{$numbered_shcname_}=$pnlwords_[5];
	    $shcname_to_msgs_map_{$numbered_shcname_}=$pnlwords_[3];
            $shcname_to_otlhit_map_{$numbered_shcname_} = $pnlwords_[7];
	    $shcname_to_uts_map_{$numbered_shcname_}=$pnlwords_[9];
            $shcname_to_lotsize_map_{$numbered_shcname_}=$pnlwords_[11];

            if ( ! ( exists $underlying_to_msgs_map_{$numbered_underlying_} ) )
            {
		$underlying_to_msgs_map_{$numbered_underlying_} = 0;
		$underlying_to_otlhit_map_{$numbered_underlying_} = 0;
		$underlying_to_uts_map_{$numbered_underlying_} = $pnlwords_[9]; # Same for all contracts in an underlying
		$underlying_to_mkt_vol_traded_map_{$numbered_underlying_} = 0;
		$underlying_to_lotsize_map_{$numbered_underlying_}=$pnlwords_[11]; # Same for all contracts in an underlying
                $underlying_to_num_contracts_{$numbered_underlying_} = 0;
            }

            if ( ! ( exists $id_to_msgs_map_{$number_} ) )
            {
		$id_to_msgs_map_{$numbered_underlying_} = 0;
            	$id_to_otlhit_map_{$number_} = 0;
            }

            $underlying_to_msgs_map_{$numbered_underlying_} += $pnlwords_[3];  
            $id_to_msgs_map_{$number_} += $pnlwords_[3];  
            $underlying_to_otlhit_map_{$numbered_underlying_} += $pnlwords_[7];
            $id_to_otlhit_map_{$number_} += $pnlwords_[7]; 
	    if ( substr($shc_, -4) ne "FUT0" )
	    {
		$underlying_to_mkt_vol_traded_map_{$numbered_underlying_} +=$pnlwords_[5];
                $underlying_to_num_contracts_{$numbered_underlying_} += 1;
	    }
	}
        elsif ($#pnlwords_ >=11 && $pnlwords_[0] eq "PNLSAMPLES")
	{
          my $numbered_underlying_ = $pnlwords_[1];
          my $number_ = GetNumFromNumSecname ( $numbered_underlying_ );
          my $underlying_ = GetSecnameFromNumSecname ( $numbered_underlying_ );
          splice ( @pnlwords_, 0, 2 );
          $underlying_to_pnl_samples_{ $underlying_ } = join ( ' ', @pnlwords_ );
	}

    }
}

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
      StatsForI(0);
      StatsForU(0);
      StatsForA(0);

      my @delta_pnls_ = `$MODELSCRIPTS_DIR/get_delta_pnl_stats_options.pl $tradesfilename_ X`;
      my @vega_pnls_ = `$MODELSCRIPTS_DIR/get_vega_pnl_stats_options.pl $tradesfilename_ X`;


      foreach my $shc_ ( keys %shc_2_simresult_ )
      {
        if ( exists ( $shc_2_pnl_stats_{$shc_} ) )
        {
          print $shc_." ".$shc_2_simresult_{$shc_}." ".$shc_2_pnl_stats_{$shc_}."\n";
        }
      }
      print @delta_pnls_;
      print @vega_pnls_;

    }

    when ("P")
    {
      foreach my $underlying_ ( keys %underlying_to_pnl_samples_ )
      {
        print $underlying_." ".$underlying_to_pnl_samples_{$underlying_}."\n";
      }
   }
}

sub StatsForI    
{
    my ( $print_ ) = @_;

    foreach my $numbered_shcname_ ( keys %shcname_to_indfilename_map_ ) 
    {
	my $this_pnl_filename_ = $shcname_to_indfilename_map_{$numbered_shcname_};
	my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats.pl $this_pnl_filename_ 1";
	my $inline_ = `$exec_cmd`;
	my $number_ = GetNumFromNumSecname ( $numbered_shcname_ );
	my $shc_ = GetSecnameFromNumSecname ( $numbered_shcname_ );

	my @t_words = split ' ', $inline_ ; 
	
	$t_words[0] = sprintf("%.2f" , $t_words[0]/$shcname_to_lotsize_map_{$numbered_shcname_});      # Changing average absolute position to number of lots
	$t_words[-3] = $t_words[-3]/$shcname_to_lotsize_map_{$numbered_shcname_};      # Changing open position to number of lots
	
	my $t_msg_count_value = 0;
	if ( exists $shcname_to_msgs_map_ { $numbered_shcname_ } ) {
	    $t_msg_count_value = int ( $shcname_to_msgs_map_ { $numbered_shcname_ } ) ;
	}
	if ( $#t_words < 12 ) {
	    push ( @t_words , $t_msg_count_value );
	} else {
	    splice @t_words, 12, 0, $t_msg_count_value;
	}

	my $t_opentrade_hits_ = 0;
	if ( exists $shcname_to_otlhit_map_ { $numbered_shcname_ } ) {
	    $t_opentrade_hits_ = int ( $shcname_to_otlhit_map_ { $numbered_shcname_ } ) ;
	}
	if ( $#t_words < 14 ) {
	    push ( @t_words , $t_opentrade_hits_ );
	} else {
	    splice @t_words, 14, 0, $t_opentrade_hits_;
	}

	my $t_mkt_volume_ = 0;
	if ( exists $shcname_to_mkt_vol_traded_map_ { $numbered_shcname_ } ) {
	    $t_mkt_volume_ = ( int ( $shcname_to_mkt_vol_traded_map_ { $numbered_shcname_ } * 10000 ) ) / 100 ;
	}
	if ( $#t_words < 16 ) {
	    push ( @t_words , $t_mkt_volume_ );
	} else {
	    splice @t_words, 16, 0, $t_mkt_volume_;
	}

	$exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats.pl $this_pnl_filename_ 2";
	$inline_ = `$exec_cmd`;
	my @t_words_result_ = split ' ', $inline_ ; 

	my $t_fill_ratio_ = 0;
	if ( exists $shcname_to_msgs_map_ { $numbered_shcname_ }  && exists $shcname_to_uts_map_ { $numbered_shcname_ } ) {
	    $t_fill_ratio_ = $t_words_result_[2] / (int ( $shcname_to_msgs_map_ { $numbered_shcname_ } ) ) ;
	}
	if ( $#t_words < 18 ) {
	    push ( @t_words , sprintf("%.4f", $t_fill_ratio_)  );
	} else {
	    splice @t_words, 19, 0, sprintf("%.4f", $t_fill_ratio_);
	}
	
	if ( $print_ == 1 )
	{
	    print "$number_ $shc_ ".join(' ',@t_words)."\n"; 
	}
	else
	{
	    $shc_2_pnl_stats_{$number_." ".$shc_} = join(' ',@t_words);
	}
    }
}

sub StatsForU
{
    my ( $print_ ) = @_;
    foreach my $numbered_underlying_ ( keys %underlying_to_indfilename_map_ ) 
    {
	my $this_pnl_filename_ = $underlying_to_indfilename_map_{$numbered_underlying_};
	my $number_ = GetNumFromNumSecname ( $numbered_underlying_ );
	my $underlying_ = GetSecnameFromNumSecname ( $numbered_underlying_ );
	my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats.pl $this_pnl_filename_ 1";
	my $inline_ = `$exec_cmd`;
	my @t_words = split ' ', $inline_ ; # split due to changing minpnl and msgcount

	$t_words[0] = sprintf("%.2f" , $t_words[0]);      
	$t_words[-3] = sprintf("%.2f" , $t_words[-3]);  

	my $t_msg_count_value = 0;
	if ( exists $underlying_to_msgs_map_ { $numbered_underlying_ } ) {
	    $t_msg_count_value = int ( $underlying_to_msgs_map_ { $numbered_underlying_ } ) ;
	}
	if ( $#t_words < 12 ) {
	    push ( @t_words , $t_msg_count_value );
	} else {
	    splice @t_words, 12, 0, $t_msg_count_value;
	}

	my $t_opentrade_hits_ = 0;
	if ( exists $underlying_to_otlhit_map_ { $numbered_underlying_ } ) {
	    $t_opentrade_hits_ = int ( $underlying_to_otlhit_map_ { $numbered_underlying_ } ) ;
	}
	if ( $#t_words < 14 ) {

	    push ( @t_words , $t_opentrade_hits_ );
	} else {
	    splice @t_words, 14, 0, $t_opentrade_hits_;
	}

	my $t_mkt_volume_ = 0;
	if ( exists $underlying_to_mkt_vol_traded_map_ { $numbered_underlying_ } ) {
            $t_mkt_volume_ = ( int ( $underlying_to_mkt_vol_traded_map_ { $numbered_underlying_ } * 10000 ) ) / (100*$underlying_to_num_contracts_{$numbered_underlying_}) ;
            $t_mkt_volume_ = sprintf("%.2f" , $t_mkt_volume_);
	}
	if ( $#t_words < 16 ) {
	    push ( @t_words , $t_mkt_volume_ );
	} else {
	    splice @t_words, 16, 0, $t_mkt_volume_;
	}


	if ( $print_ == 1 )
	{
	    print "$number_ $underlying_ ".join(' ',@t_words)."\n";  
	}
	else
	{
	    $shc_2_pnl_stats_{$number_." ".$underlying_} = join(' ',@t_words);
	}
    }
}

sub StatsForA
{
    my ( $print_ ) = @_;
    foreach my $number_ ( keys %id_to_indfilename_map_ ) 
    {
	my $this_pnl_filename_ = $id_to_indfilename_map_{$number_};
	my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats.pl $this_pnl_filename_ 1";
	my $inline_ = `$exec_cmd`;
	my @t_words = split ' ', $inline_ ; # split due to changing minpnl and msgcount

        $t_words[0] = sprintf("%.2f" , $t_words[0]);
        $t_words[-3] = sprintf("%.2f" , $t_words[-3]);

	my $t_msg_count_value = 0;
	if ( exists $id_to_msgs_map_ { $number_ } ) {
	    $t_msg_count_value = int ( $id_to_msgs_map_ { $number_ } ) ;
	}
	if ( $#t_words < 12 ) {
	    push ( @t_words , $t_msg_count_value );
	} else {
	    splice @t_words, 12, 0, $t_msg_count_value;
	}

	my $t_opentrade_hits_ = 0;
        if ( exists $id_to_otlhit_map_ { $number_ } ) {
	    $t_opentrade_hits_ = int ( $id_to_otlhit_map_ { $number_ } ) ;
        }
	if ( $#t_words < 14 ) {
	    push ( @t_words , $t_opentrade_hits_ );
	} else {
	    splice @t_words, 14, 0, $t_opentrade_hits_;
	}

	my $t_mkt_vol_traded_ = 0;
	if ( exists ( $id_to_mkt_vol_traded_map_{ $number_} ) )
	{
	    $t_mkt_vol_traded_ = int ( $id_to_mkt_vol_traded_map_ {$number_} * 10000 ) / 100 ;
	}

	if ( $#t_words < 16 ) {
	    push ( @t_words , $t_mkt_vol_traded_ );
	} else {
	    splice @t_words, 16, 0, $t_mkt_vol_traded_;
	}

	if ( $print_ == 1 )
	{
	    print "$number_ ALL ".join(' ',@t_words)."\n";  
	}
	else
	{
	    $shc_2_pnl_stats_{$number_." ALL"} = join(' ',@t_words);
	}
    }
}

if ( $delete_intermediate_files_ == 1 )
{
    for ( my $i = 0 ; $i <= $#intermediate_files_; $i ++ )
    {
	if ( -e $intermediate_files_[$i] )
	{
	    `rm -f $intermediate_files_[$i]`;
	}
    }
}
