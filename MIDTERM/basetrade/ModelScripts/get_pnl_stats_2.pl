#!/usr/bin/perl
use strict ;
use warnings ;
use feature "switch"; # for given, when
use File::Basename ;
use FileHandle;
use Scalar::Util qw(looks_like_number);
use List::Util qw/max min/; # for max

sub StatsForA;
sub StatsForI;
sub StatsForH;

my $USAGE="$0 tradesfilename type=[(I)individual|(C)cumulative|(B)both] [strategy_files_dir]";
if ( $#ARGV < 0 ) { print "$USAGE\n"; exit ( 0 ); }
my $tradesfilename_ = $ARGV[0];

my $type_ = "I";
my $strategy_files_dir_ = "INVALIDFILE";
if ( $#ARGV >= 1 )
{
    $type_ = $ARGV[1];
}
if( $#ARGV >= 2)
{
    $strategy_files_dir_ = $ARGV[2];
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

my $number_shortcodes_ = `cat $tradesfilename_ | awk 'match\(\$1, /^[0-9.]+\$/\) {print \$3}' | sort -u | wc -l`;

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

my %secname_to_indfilename_map_ = ();
my %secname_to_indfilehandle_map_ = ();
my %secname_to_pnl_map_ = ();
my $total_pnl_ = 0;
my %secname_to_pos_map_ = ();
my %query_to_min_pnl_map_ = ();
my %query_to_opentrade_hits_map_ = ();
my %query_to_unit_trade_size_map_ = ();
my %query_to_msg_count_map_ = ();
my %query_to_sim_result_ = ();
my %query_to_pnl_stats_ = ();
my $total_pos_ = 0;
my %number_to_total_pnl_ = ();
my %number_to_min_pnl_ = ();
my %number_to_max_pnl_ = ();

open ALL_FILE_HANDLE, ">$pnl_all_file_" or PrintStacktraceAndDie ( "$0 cannot open $pnl_all_file_" );

while ( my $inline_ = <PNLFILEHANDLE> )
{
    chomp ( $inline_ );
    my @pnlwords_ = split ( ' ', $inline_ );
    if ($#pnlwords_ >= 8 && $pnlwords_[0] ne "PNLSAMPLES" && $pnlwords_[0] ne "SIMRESULT" && $pnlwords_[0] ne "PNLSPLIT")
    {
        my $numbered_secname_ = $pnlwords_[2];
        my $number_ = GetNumFromNumSecname($numbered_secname_);
        my $pos_ = $pnlwords_[6];
        my $pnl_ = $pnlwords_[8];
        if (!(looks_like_number($pos_) && looks_like_number($pnl_))) {
            print STDERR  "Malformed Tradefile: $tradesfilename_ \nline: $inline_\n";
            my $logfilename_ = $tradesfilename_ =~ s/trades/log/g;
            `cp $tradesfilename_ $logfilename_ /spare/local/`;
            next;
        }
        if (!( exists $secname_to_indfilehandle_map_{$numbered_secname_} ))
        {
            my $indfilename_ = $pnl_tempdir."/pnl_".$numbered_secname_."_".$tradesfilebase_;
            push ( @intermediate_files_, $indfilename_ );
            my $indfilehandle_ = FileHandle->new;
            $indfilehandle_->open ( "> $indfilename_" );
            $secname_to_indfilename_map_{$numbered_secname_} = $indfilename_;
            $secname_to_indfilehandle_map_{$numbered_secname_} = $indfilehandle_;
            $secname_to_pnl_map_{$numbered_secname_} = 0;
            $secname_to_pos_map_{$numbered_secname_} = 0;
        }
        if (!(exists $number_to_total_pnl_{$number_} ))
        {
            $number_to_total_pnl_{$number_} = 0;
            $number_to_min_pnl_{$number_} = 0;
            $number_to_max_pnl_{$number_} = 0;
        }


        my $indfilehandle_ = $secname_to_indfilehandle_map_{$numbered_secname_};
        print $indfilehandle_ "$inline_\n";
        $total_pnl_ += $pnl_ - $secname_to_pnl_map_{$numbered_secname_};
        $secname_to_pnl_map_{$numbered_secname_} = $pnl_;
        $total_pos_ += $pos_ - $secname_to_pos_map_{$numbered_secname_};
        $secname_to_pos_map_{$numbered_secname_} = $pos_;
        $pnlwords_[8] = $total_pnl_;
        $pnlwords_[6] = $total_pos_;
        print ALL_FILE_HANDLE join ( ' ', @pnlwords_ )."\n";

        $number_to_total_pnl_{$number_} = 0 ;
        foreach my $num_sec_ (keys %secname_to_indfilename_map_)
        {
            my $num_ = GetNumFromNumSecname($num_sec_);
            if ($num_ == $number_)
            {
                $number_to_total_pnl_{$number_} += $secname_to_pnl_map_{$num_sec_};
            }
        }
            $number_to_min_pnl_{$number_} = min($number_to_min_pnl_{$number_}, $number_to_total_pnl_{$number_});
            $number_to_max_pnl_{$number_} = max($number_to_max_pnl_{$number_}, $number_to_total_pnl_{$number_});
    }
    else {
        my $t_msg_count_ = 0;

        if ($#pnlwords_ >= 2 && $pnlwords_[0] eq "EOD_MIN_PNL:")
        {
            my $number_ = $pnlwords_[1];
            my $min_pnl_ = $pnlwords_[2];
            $query_to_min_pnl_map_{$number_} = $min_pnl_;
        }
        elsif ($#pnlwords_ >= 2 && $pnlwords_[ 0 ] eq "EOD_MSG_COUNT:")
        {
            my $number_ = $pnlwords_[ 1 ];

            # for portfolio and stir strats, EOD_MSG_COUNT has 4 tokens, special handling for them
            if ($#pnlwords_ > 2)
            {
                # consider the TOTAL line, ignore the rest
                if ($pnlwords_[2] eq "TOTAL")
                {
                    $t_msg_count_ = $pnlwords_[ 3 ];
                }
            }
            else
            {
                $t_msg_count_ = $pnlwords_[ 2 ];
            }
            $query_to_msg_count_map_{ $number_ } = $t_msg_count_;
        }
        elsif ($#pnlwords_ >= 2 && $pnlwords_[ 0 ] eq "NUM_OPENTRADE_HITS:")
        {
            my $number_ = $pnlwords_[ 1 ];
            my $num_opentrade_hits_ = $pnlwords_[ 2 ];
            $query_to_opentrade_hits_map_{ $number_ } = $num_opentrade_hits_;
        }
        elsif ($#pnlwords_ >= 2 && $pnlwords_[ 0 ] eq "UNIT_TRADE_SIZE:")
        {
            my $number_ = $pnlwords_[ 1 ];
            my $unit_trade_size_ = $pnlwords_[ 2 ];
            $query_to_unit_trade_size_map_{ $number_ } = $unit_trade_size_;
        }
        elsif ($#pnlwords_ >= 2 && $pnlwords_[ 0 ] eq "SIMRESULT")
        {
            my $number_ = $pnlwords_[ 1 ];

            my $total_ = $pnlwords_[4] + $pnlwords_[5] + $pnlwords_[6] + $pnlwords_[7];
            if ($total_ != 0) {
              $pnlwords_[$_] = int($pnlwords_[$_] *100/ $total_) foreach (4..7);
            }

            my $sim_result_ = join(' ', @pnlwords_[2..7]);
            $query_to_sim_result_{ $number_ } = $sim_result_;
        }
    }
}

{
    foreach my $numbered_secname_ (keys %secname_to_indfilehandle_map_)
    {
        $secname_to_indfilehandle_map_{$numbered_secname_}->close;
    }
}

close ALL_FILE_HANDLE;

given ( $type_ )
{
    when ("I")
    {
      StatsForI(1,0);
    }
    when ("H")
    {
      StatsForH(1);
    }
    when ("A")
    {
      StatsForA(1);
    }
    when ("X")
    {
      StatsForI(0,0);
      foreach my $number_ ( keys %query_to_sim_result_ )
      {
        if ( ! exists $query_to_pnl_stats_{$number_} )
        {
            $query_to_pnl_stats_{$number_} = "0 0 0 0 0 0 0 0 0 0 0 0 0\n";
        }
        print $number_." ".$query_to_sim_result_{$number_}." ".$query_to_pnl_stats_{$number_};
      } 
    }
    when ("SX")
    {
      #it is kind of a hack for structured strategies where I have averaged out ttcs and fracpos;
        # for min pnl I am adding up the min pnls of each shortcode (which is probably the most unoptimistic estimate)
        # Others are fine I think
        ## To update for min and max pnl; at every instance I calculate the total pnl as the sum of securities with the same
        # query ids; and then use that to calculate the min and max pnl. This seems to be a very exact estimate of min and max pnl
        # IT should be good to go without any issues; Any comments??
      StatsForI(0,1);

      my %average_abs_postion = ();
      my %median_ttc = ();
      my %avg_ttc = ();
      my %median = ();
      my %avg = ();
      my %stdev = ();
      my %sharpe = ();
      my %fracpos = ();
      my %min = ();
      my %max = ();
      my %norm_ttc = ();
      my %last_abs_position = ();
      my %total_positive_trades = ();
      my %total_closed_trades = ();
      my %number_count_ = ();
        foreach my $numbered_secname_ ( keys %secname_to_indfilename_map_ )
        {
            #foreach my $number_ ( keys %query_to_sim_result_ )
            if (!exists $query_to_pnl_stats_{$numbered_secname_})
            {
                $query_to_pnl_stats_{$numbered_secname_} = "0 0 0 0 0 0 0 0 0 0 0 0 0\n";
            }
            my $number_ = GetNumFromNumSecname($numbered_secname_);

            my @t_words = split ' ', $query_to_pnl_stats_{$numbered_secname_};

            if (!exists $average_abs_postion{$number_}) {
                $average_abs_postion{$number_} = int($t_words[0]);
                $median_ttc{$number_} = int($t_words[1]);
                $avg_ttc{$number_} = int($t_words[2]);
                $median{$number_} = int($t_words[3]);
                $avg{$number_} = int($t_words[4]);
                $stdev{$number_} = int($t_words[5]);
                $sharpe{$number_} = int($t_words[6]);
                $fracpos{$number_} = int($t_words[7]);
                $min{$number_} = int($t_words[8]);
                $max{$number_} = int($t_words[9]);
                $norm_ttc{$number_} = int($t_words[10]);
                $last_abs_position{$number_} = int($t_words[11]);
                $total_positive_trades{$number_} = int($t_words[12]);
                $ total_closed_trades{$number_} = int($t_words[13]);
                $number_count_{$number_} = 1;
            } else {
                $average_abs_postion{$number_} += int($t_words[0]);
                $median_ttc{$number_} += int($t_words[1]);
                $avg_ttc{$number_} += int($t_words[2]);
                $median{$number_} += int($t_words[3]);
                $avg{$number_} += int($t_words[4]);
                $stdev{$number_} += int($t_words[5]);
                $sharpe{$number_} += int($t_words[6]);
                $fracpos{$number_} += int($t_words[7]);
                $min{$number_} += int($t_words[8]);
                $max{$number_} += int($t_words[9]);
                $norm_ttc{$number_} += int($t_words[10]);
                $last_abs_position{$number_} += int($t_words[11]);
                $total_positive_trades{$number_} += int($t_words[12]);
                $total_closed_trades{$number_} += int($t_words[13]);
                $number_count_{$number_} += 1;
            }
        }
        foreach my $number_ ( keys %number_count_ ){
            $median_ttc{$number_} = int($median_ttc{$number_}/$number_count_{$number_});
            $avg_ttc{$number_} = int($avg_ttc{$number_}/$number_count_{$number_});
            $sharpe{$number_} = int($sharpe{$number_}/$number_count_{$number_});
            $fracpos{$number_} = int($fracpos{$number_}/$number_count_{$number_});
            $norm_ttc{$number_} = int($norm_ttc{$number_}/$number_count_{$number_});
       # Average Abs position; Median trade-close-time; Average trade-close-time; Median: Avg: Stdev:  Sharpe:
            # 0.25 Fracpos:; min-max-draw ; Max trade-close-time; Normalized averagetrade-close-time; Last Abs position:;
            # Total Positive Closed Trades:; Total Closed Trades: ;
      #print "Num $number_ \n";

#print $number_." ".$query_to_sim_result_{$number_}
        print $number_." $average_abs_postion{$number_} $median_ttc{$number_}" .
        " $avg_ttc{$number_} $median{$number_} $avg{$number_} $stdev{$number_} $sharpe{$number_} $fracpos{$number_}".
        " $number_to_min_pnl_{$number_} $number_to_max_pnl_{$number_} $norm_ttc{$number_}  $last_abs_position{$number_}  $total_positive_trades{$number_}".
        " $total_closed_trades{$number_}\n" ;
      }
    }
}

if ( $strategy_files_dir_ ne "INVALIDFILE")
{
	if ( ! -d $strategy_files_dir_ )
	{
	    print STDERR "get_pnl_stats_2.pl : $strategy_files_dir_ not a directory\n";
	}
	else
	{ # copy trades file to strategy_files_dir
	    for my $numbered_secname_ ( keys ( %secname_to_indfilename_map_ ) )
	    {
		my $yyyymmdd_ = substr ( $tradesfilebase_ , index ( $tradesfilebase_ , "trades" ) + 7 , 8 );
		my $unique_id_ = substr ( $numbered_secname_ , index ( $numbered_secname_ , "." ) + 1 );

		my $this_trades_filename_ = $strategy_files_dir_."/trades.$yyyymmdd_.$unique_id_";

		my $indfilename_ = $secname_to_indfilename_map_ { $numbered_secname_ };
		my $exec_cmd_ = "cp $indfilename_ $this_trades_filename_";
		`$exec_cmd_`;
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



sub StatsForI
{
  my ( $print_, $structured_) = @_;
  foreach my $numbered_secname_ ( keys %secname_to_indfilename_map_ )
  {
    my $this_pnl_filename_ = $secname_to_indfilename_map_{$numbered_secname_};
    my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats.pl $this_pnl_filename_ 1";
    my $inline_ = `$exec_cmd`;
    my $number_ = ( $numbered_secname_ );
    if (! $structured_) {
        $number_ = GetNumFromNumSecname( $numbered_secname_ );
    }
    my @t_words = split ' ', $inline_ ; # split due to changing minpnl and msgcount

    if ( ( exists $query_to_min_pnl_map_ { $number_ } ) && ( $#t_words >= 8 ) ) {
        $t_words[8] = int ( $query_to_min_pnl_map_{$number_} ) ;
    }

    my $t_msg_count_value = 0;
    if ( exists $query_to_msg_count_map_ { $number_ } ) {
        $t_msg_count_value = int ( $query_to_msg_count_map_ { $number_ } ) ;
    }
    if ( $#t_words < 12 ) {
        push ( @t_words , $t_msg_count_value );
    } else {
        splice @t_words, 12, 0, $t_msg_count_value;
    }

    my $t_opentrade_hits_ = 0;
    if ( exists $query_to_opentrade_hits_map_ { $number_ } ) {
        $t_opentrade_hits_ = int ( $query_to_opentrade_hits_map_ { $number_ } ) ;
    }
    if ( $#t_words < 14 ) {
        push ( @t_words , $t_opentrade_hits_ );
    } else {
        splice @t_words, 14, 0, $t_opentrade_hits_;
    }

    my $t_unit_trade_size_ = 0;
    if ( exists $query_to_unit_trade_size_map_ { $number_ } ) {
          $t_unit_trade_size_ = int ( $query_to_unit_trade_size_map_ { $number_ } ) ;
    }
    if ( $#t_words < 16 ) {
          push ( @t_words , $t_unit_trade_size_ );
    } else {
          splice @t_words, 16, 0, $t_unit_trade_size_;
    }

    if ( $print_ == 1 )
    {
      print "$number_ ".join(' ',@t_words)."\n"; # changed from print "$number_ $inline_"; 
    }
    else
    {
      $query_to_pnl_stats_{$number_} = join(' ',@t_words)."\n";
    }
  }
}

sub StatsForH
{
  my ( $print_ ) = @_;
  foreach my $numbered_secname_ ( keys %secname_to_indfilename_map_ ) 
  { 
    my $this_pnl_filename_ = $secname_to_indfilename_map_{$numbered_secname_};
    my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats.pl $this_pnl_filename_ 2";
#    print STDERR "$exec_cmd\n";
    my $inline_ = `$exec_cmd`;
    my $number_ = GetNumFromNumSecname ( $numbered_secname_ );
    if ( $print_ == 1 )
    {
      print "$number_ $inline_";
    }
    else 
    {
      $query_to_pnl_stats_{$number_} = $inline_;
    }
  }
}

sub StatsForA
{
  my ( $print_ ) = @_;
  my $this_pnl_filename_ = $pnl_all_file_;
  my $inline_ = `$MODELSCRIPTS_DIR/get_pnl_stats.pl $this_pnl_filename_ 1`;	
  if ( $print_ == 1 )
  {
    print "-1 $inline_";
  }
  else
  {
    $query_to_pnl_stats_{"-1"} = $inline_;
  }
}
