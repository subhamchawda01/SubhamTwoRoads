#!/usr/bin/perl
use strict ;
use warnings ;
use feature "switch"; # for given, when
use File::Basename ;
use FileHandle;
use Scalar::Util qw(looks_like_number);

my $USAGE="$0 tradesfilename type=[(I)individual|(C)cumulative|(B)both] club=[(0)id/(1)name]";
if ( $#ARGV < 0 ) { print "$USAGE\n"; exit ( 0 ); }
my $tradesfilename_ = $ARGV[0];
my $type_ = "I";
my $club_by_id_or_name_ = 0 ;
if ( $#ARGV > 0 )
{
    $type_ = $ARGV[1];
}

if ( $#ARGV > 1 )
{
	$club_by_id_or_name_ = $ARGV[2]; 
}

my $targetcol = 9;

my $tradesfilebase_ = basename ( $tradesfilename_ ); chomp ($tradesfilebase_);

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $pnl_tempdir=$HOME_DIR."/pnltemp";

require "$GENPERLLIB_DIR/get_num_from_numsecname.pl";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $delete_intermediate_files_ = 1;
my @intermediate_files_ = ();
my $pnl_all_file_ = $pnl_tempdir."/pnl_all_".$tradesfilebase_;
#print $pnl_all_file_."\n";
push ( @intermediate_files_, $pnl_all_file_ );

if ( ! -d $pnl_tempdir )
{
    `mkdir -p $pnl_tempdir`;
}

open PNLFILEHANDLE, "< $tradesfilename_" or PrintStacktraceAndDie ( "$0 could not open tradesfile $tradesfilename_\n" );

my %runtimeid_to_indfilename_map_ = ();
my %runtimeid_to_indfilehandle_map_ = ();
my %runtimeid_to_pnl_map_ = ();
my $total_pnl_ = 0;
my %runtimeid_to_pos_map_ = ();
my %runtimeid_to_min_pnl_map_ = ();
my %runtimeid_to_opentrade_hits_map_ = ();
my %runtimeid_to_msg_count_map_ = ();
my %runrime_id_to_simresult_ = () ;
my $total_pos_ = 0;

my $tradingdate_ = 20160405;
my @date_words_ = split ( '\.', $tradesfilebase_ ) ; chomp ( @date_words_);
if ( $#date_words_ >= 1 ) {
    $tradingdate_ = $date_words_[1];
}
open ALL_FILE_HANDLE , ">$pnl_all_file_" or PrintStacktraceAndDie ( "$0 cannot open $pnl_all_file_" );

while ( my $inline_ = <PNLFILEHANDLE> )
{
  chomp ( $inline_ );
  my @pnlwords_ = split ( ' ', $inline_ );
  if ( $#pnlwords_ >= 8 && ! ( $pnlwords_[0] eq "PNLSAMPLES"  ) && ! ( $pnlwords_[0] eq "PNLSPLIT"  ) )
  {
    my $numbered_secname_ = $pnlwords_[2];
    my @numbered_secname_words_ = split ( '\.', $numbered_secname_ );
    my $t_runtime_id_ = $numbered_secname_words_[1];
    if ( $club_by_id_or_name_ == 1 ) { $t_runtime_id_ = $numbered_secname_;}

    my $pos_ = $pnlwords_[6]; 
    my $pnl_ = $pnlwords_[8] ;
    if (! (looks_like_number($pos_) && looks_like_number($pnl_)) ) { 
      print STDERR  "Malformed Tradefile: $tradesfilename_ \nline: $inline_\n";
      my $logfilename_ = $tradesfilename_ =~ s/trades/log/g ;
      `cp $tradesfilename_ $logfilename_ /spare/local/`;
      next;
    }
    if ( ! ( exists $runtimeid_to_indfilehandle_map_{$t_runtime_id_} ) )
    {
      my $indfilename_ = $pnl_tempdir."/pnl_".$t_runtime_id_."_".$tradesfilebase_;
      #push ( @intermediate_files_, $indfilename_ );
      my $indfilehandle_ = FileHandle->new;
      $indfilehandle_->open ( "> $indfilename_" );

      $runtimeid_to_indfilename_map_{$t_runtime_id_} = $indfilename_;
      $runtimeid_to_indfilehandle_map_{$t_runtime_id_} = $indfilehandle_;
      $runtimeid_to_pnl_map_{$t_runtime_id_} = 0;
      $runtimeid_to_pos_map_{$t_runtime_id_} = 0;
    }
    my $indfilehandle_ = $runtimeid_to_indfilehandle_map_{$t_runtime_id_};
    print $indfilehandle_ "$inline_\n";
    $total_pnl_ += $pnl_ - $runtimeid_to_pnl_map_{$t_runtime_id_};
    $runtimeid_to_pnl_map_{$t_runtime_id_} = $pnl_;
    $total_pos_ += $pos_ - $runtimeid_to_pos_map_{$t_runtime_id_};
    $runtimeid_to_pos_map_{$t_runtime_id_} = $pos_;
    $pnlwords_[8] = $total_pnl_;
    $pnlwords_[6] = $total_pos_;
    print ALL_FILE_HANDLE join ( ' ', @pnlwords_ )."\n";
  }
  else 
  {
    if ($#pnlwords_ >=2 && $pnlwords_[0] eq "EOD_MIN_PNL:")
    {
      my $number_ = $pnlwords_[1];
      my $min_pnl_ = $pnlwords_[2] ;
      $runtimeid_to_min_pnl_map_{$number_}=$min_pnl_;
    }
    elsif ( $#pnlwords_ >= 2 && $pnlwords_ [ 0 ] eq "EOD_MSG_COUNT:" )
    {
      my $number_ = $pnlwords_ [ 1 ];
      my $t_msg_count_ = 0 ;
      if ( $#pnlwords_>= 3 )
      {
        $t_msg_count_ = $pnlwords_[3];
        my $t_runtime_id_ = $pnlwords_[2].".".$pnlwords_[1];
        if ( $pnlwords_[2] eq "TOTAL" )
        {
          $runtimeid_to_msg_count_map_ { $t_runtime_id_ } = $t_msg_count_;
          $runtimeid_to_msg_count_map_ { $pnlwords_[1] } = $t_msg_count_;
        }
        elsif ( $club_by_id_or_name_ == 1 )
        {
#will happen in case the product didnt trade
          if ( ! ( exists $runtimeid_to_indfilehandle_map_{$t_runtime_id_} ) )
          {
            my $indfilename_ = $pnl_tempdir."/pnl_".$t_runtime_id_."_".$tradesfilebase_;
            push ( @intermediate_files_, $indfilename_ );
            my $indfilehandle_ = FileHandle->new;
            $indfilehandle_->open ( "> $indfilename_" );

            $runtimeid_to_indfilename_map_{$t_runtime_id_} = $indfilename_;
            $runtimeid_to_indfilehandle_map_{$t_runtime_id_} = $indfilehandle_;
            $runtimeid_to_pnl_map_{$t_runtime_id_} = 0;
            $runtimeid_to_pos_map_{$t_runtime_id_} = 0;
          }
          my $indfilehandle_ = $runtimeid_to_indfilehandle_map_{$t_runtime_id_};
          print $indfilehandle_ "EOD_MSG_COUNT: $pnlwords_[1] $t_msg_count_\n";
          $runtimeid_to_msg_count_map_ { $t_runtime_id_ } = $t_msg_count_;
        }
      }
      else
      {
        $t_msg_count_ = $pnlwords_ [ 2 ];
        $runtimeid_to_msg_count_map_ { $number_ } = $t_msg_count_;
      }
    }
    elsif ( $#pnlwords_ >= 2 && $pnlwords_ [ 0 ] eq "NUM_OPENTRADE_HITS:" )
    {
      my $number_ = $pnlwords_ [ 1 ];
      my $num_opentrade_hits_ = $pnlwords_ [ 2 ];
      $runtimeid_to_opentrade_hits_map_ { $number_ } = $num_opentrade_hits_;
    }
    elsif ( $#pnlwords_ >= 2 && $pnlwords_[0] eq "PNLSPLIT" )
    {
      my $idx_ = 1 ;
      my $run_id_ = $pnlwords_[$idx_ ];
      $idx_++;
      while ( $idx_ < $#pnlwords_ )
      {
        my $prod_ = $pnlwords_[$idx_ ] ;
        my $prod_pnl_ = $pnlwords_ [$idx_+ 1] ;
        my $prod_vol_ = $pnlwords_ [$idx_+ 2 ];
        my $prod_non_best_per_ = $pnlwords_[$idx_ + 3];
        my $prod_best_per_ = $pnlwords_[$idx_ + 4 ];
        my $prod_improve_ = $pnlwords_[$idx_ + 5 ];
        my $prod_agg_ = $pnlwords_[$idx_ + 6 ];
        $idx_ = $idx_ + 7 ;
        if ( $prod_ !~ /NSE|BSE/ ) {
          $prod_ = `$LIVE_BIN_DIR/get_exchange_symbol $prod_ $tradingdate_`; chomp ( $prod_ );
        }
        $runrime_id_to_simresult_ {$prod_.".".$run_id_} = "$prod_pnl_ $prod_vol_ $prod_non_best_per_ $prod_best_per_ $prod_improve_ $prod_agg_ ";
      }
    }
  }
}

{
    foreach my $t_runtime_id_ ( keys %runtimeid_to_indfilehandle_map_ )
    {
    	$runtimeid_to_indfilehandle_map_{$t_runtime_id_}->close ;
    }
}
close ALL_FILE_HANDLE ;

foreach my $t_runtime_id_ ( sort ( keys %runtimeid_to_indfilename_map_ ) ) 
{
  my $this_pnl_filename_ = $runtimeid_to_indfilename_map_{$t_runtime_id_};
  my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_stir.pl $this_pnl_filename_ 1";
  if ( $club_by_id_or_name_ == 1 )
  {
    $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats.pl $this_pnl_filename_ 1";
  }
  my $inline_ = `$exec_cmd`;
  my $number_ =  $t_runtime_id_ ;

  my @t_words = split ' ', $inline_ ; # split due to changing minpnl and msgcount

    if ( ( exists $runtimeid_to_min_pnl_map_ { $number_ } ) && ( $#t_words >= 8 ) ) {
      $t_words[8] = int ( $runtimeid_to_min_pnl_map_{$number_} ) ; 
    }

  my $t_msg_count_value = 0;
  if ( exists $runtimeid_to_msg_count_map_ { $number_ } ) {
    $t_msg_count_value = int ( $runtimeid_to_msg_count_map_ { $number_ } ) ;
  }
  if ( $#t_words < 12 ) {
    push ( @t_words , $t_msg_count_value );
  } else {
    splice @t_words, 12, 0, $t_msg_count_value;
  }

  my $t_opentrade_hits_ = 0;
  if ( exists $runtimeid_to_opentrade_hits_map_ { $number_ } ) {
    $t_opentrade_hits_ = int ( $runtimeid_to_opentrade_hits_map_ { $number_ } ) ;
  }
  if ( $#t_words < 14 ) {
    push ( @t_words , $t_opentrade_hits_ );
  } else {
    splice @t_words, 14, 0, $t_opentrade_hits_;
  }
  if ( $club_by_id_or_name_ == 1 )
  {
    if ( exists $runrime_id_to_simresult_{$number_} )
    {
      print "$number_ ".$runrime_id_to_simresult_{$number_}." ".join(' ',@t_words)."\n"; # changed from print "$number_ $inline_";
    }
    else
    {
      print "$number_ 0 0 0 0 0 ".join(' ',@t_words)."\n";
    }
  }
  else
  {
    print "$number_ ".join(' ',@t_words)."\n"; # changed from print "$number_ $inline_";
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
