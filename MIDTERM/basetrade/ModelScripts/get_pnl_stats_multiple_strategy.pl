#!/usr/bin/perl
use strict ;
use warnings ;
use feature "switch"; # for given, when
use File::Basename ;
use FileHandle;
use Scalar::Util qw(looks_like_number);

my $USAGE="$0 tradesfilename type=[(I)brief 1 stats|(H)brief 2 stats|(E)brief 3 stats]";
if ( $#ARGV < 0 ) { print "$USAGE\n"; exit ( 0 ); }
my $tradesfilename_ = $ARGV[0];
my $type_ = "I";
if ( $#ARGV > 0 )
{
    $type_ = $ARGV[1];
}
my $targetcol = 9;

my $tradesfilebase_ = basename ( $tradesfilename_ ); chomp ($tradesfilebase_);

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

my $pnl_tempdir=$HOME_DIR."/pnltemp";

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
my $total_pos_ = 0;

open ALL_FILE_HANDLE , ">$pnl_all_file_" or PrintStacktraceAndDie ( "$0 cannot open $pnl_all_file_" );

while ( my $inline_ = <PNLFILEHANDLE> )
{
    chomp ( $inline_ );
    my @pnlwords_ = split ( ' ', $inline_ );
    if ( $#pnlwords_ >= 8 )
    {
	my $numbered_secname_ = $pnlwords_[2];
	my $pos_ = $pnlwords_[6]; 
	my $pnl_ = $pnlwords_[8];
	if (! (looks_like_number($pos_) && looks_like_number($pnl_)) ) { 
	    print STDERR  "Malformed Tradefile: $tradesfilename_ \nline: $inline_\n";
	    my $logfilename_ = $tradesfilename_ =~ s/trades/log/g;
	    `cp $tradesfilename_ $logfilename_ /spare/local/`;
	    next;
	}
	if ( ! ( exists $secname_to_indfilehandle_map_{$numbered_secname_} ) )
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
	my $indfilehandle_ = $secname_to_indfilehandle_map_{$numbered_secname_};
	print $indfilehandle_ "$inline_\n";
	$total_pnl_ += $pnl_ - $secname_to_pnl_map_{$numbered_secname_};
	$secname_to_pnl_map_{$numbered_secname_} = $pnl_;
	$total_pos_ += $pos_ - $secname_to_pos_map_{$numbered_secname_};
	$secname_to_pos_map_{$numbered_secname_} = $pos_;
	$pnlwords_[8] = $total_pnl_;
	$pnlwords_[6] = $total_pos_;
	print ALL_FILE_HANDLE join ( ' ', @pnlwords_ )."\n";
    }
	else{
	if ($#pnlwords_ >=2 && $pnlwords_[0] eq "EOD_MIN_PNL:"){
	  my $number_ = $pnlwords_[1];
	  my $min_pnl_ = $pnlwords_[2];
	  $query_to_min_pnl_map_{$number_}=$min_pnl_;
	#printf("%d %f\n",$number_,$min_pnl_);
	}
#	printf("%d words\n", $#pnlwords_);
	}
}

{
    foreach my $numbered_secname_ ( keys %secname_to_indfilehandle_map_ )
    {
	$secname_to_indfilehandle_map_{$numbered_secname_}->close ;
    }
}
close ALL_FILE_HANDLE ;

given ( $type_ )
{
    when ("I")
    {
	foreach my $numbered_secname_ ( keys %secname_to_indfilename_map_ ) 
	{
	    my $this_pnl_filename_ = $secname_to_indfilename_map_{$numbered_secname_};
	    my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_single_strategy.pl $this_pnl_filename_ 1";
#	    print STDERR "$exec_cmd\n";
	    my $inline_ = `$exec_cmd`;
	    my $number_ = GetNumFromNumSecname ( $numbered_secname_ );
	   # my $new_min_=-1;
	    my $new_min_ = $query_to_min_pnl_map_{$number_}; $new_min_ = int ($new_min_);
	    my @t_words = split ' ', $inline_ ;
	    $t_words[8] = $new_min_ ;
	    #print "$number_ $inline_";
	    print "$number_ ".join(' ',@t_words)."\n";
	}
    }
    when ("E")
    {
	foreach my $numbered_secname_ ( keys %secname_to_indfilename_map_ ) 
	{
	    my $this_pnl_filename_ = $secname_to_indfilename_map_{$numbered_secname_};
	    my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_single_strategy.pl $this_pnl_filename_ 3";
#	    print STDERR "$exec_cmd\n";
	    my $inline_ = `$exec_cmd`;
	    my $number_ = GetNumFromNumSecname ( $numbered_secname_ );
	   # my $new_min_=-1;
	    my $new_min_ = $query_to_min_pnl_map_{$number_}; $new_min_ = int ($new_min_);
	    my @t_words = split ' ', $inline_ ;
	    $t_words[8] = $new_min_ ;
	    #print "$number_ $inline_";
	    print "$number_ ".join(' ',@t_words)."\n";
	}
    }
    when ("H")
    {
	foreach my $numbered_secname_ ( keys %secname_to_indfilename_map_ ) 
	{
	    my $this_pnl_filename_ = $secname_to_indfilename_map_{$numbered_secname_};
	    my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_single_strategy.pl $this_pnl_filename_ 2";
#	    print STDERR "$exec_cmd\n";
	    my $inline_ = `$exec_cmd`;
	    my $number_ = GetNumFromNumSecname ( $numbered_secname_ );
	    print "$number_ $inline_";
	}
    }
    when ("A")
    {
	my $this_pnl_filename_ = $pnl_all_file_;
	my $inline_ = `$MODELSCRIPTS_DIR/get_pnl_stats_single_strategy.pl $this_pnl_filename_ 1`;	
	print "-1 $inline_";
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
