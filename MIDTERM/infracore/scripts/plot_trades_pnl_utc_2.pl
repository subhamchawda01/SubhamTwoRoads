#!/usr/bin/perl
use strict ;
use warnings ;
use feature "switch"; # for given, when
use File::Basename ;
use FileHandle;

my $USAGE="$0 tradesfilename type=[(I)individual|(C)cumulative|(B)both]";
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
my $pnl_tempdir=$HOME_DIR."/pnltemp";

my $pnl_all_file_ = $pnl_tempdir."/pnl_all_".$tradesfilebase_;

if ( ! -d $pnl_tempdir )
{
    `mkdir -p $pnl_tempdir`;
}

open PNLFILEHANDLE, "< $tradesfilename_" or die "$0 could not open tradesfile $tradesfilename_\n";

my %secname_to_indfilename_map_ = ();
my %secname_to_indfilehandle_map_ = ();
my %secname_to_pnl_map_ = ();
my $total_pnl_ = 0;

open ALL_FILE_HANDLE , ">$pnl_all_file_" or die "$0 cannot open $pnl_all_file_";

while ( my $inline_ = <PNLFILEHANDLE> )
{
    chomp ( $inline_ );
    my @pnlwords_ = split ( ' ', $inline_ );
    if ( $#pnlwords_ >= 8 )
    {
	my $numbered_secname_ = $pnlwords_[2];
	my $pnl_ = $pnlwords_[8];
	if ( ! ( exists $secname_to_indfilehandle_map_{$numbered_secname_} ) )
	{
	    my $indfilename_ = $pnl_tempdir."/pnl_".$numbered_secname_."_".$tradesfilebase_;
	    my $indfilehandle_ = FileHandle->new;
	    $indfilehandle_->open ( "> $indfilename_" );

	    $secname_to_indfilename_map_{$numbered_secname_} = $indfilename_;
	    $secname_to_indfilehandle_map_{$numbered_secname_} = $indfilehandle_;
	    $secname_to_pnl_map_{$numbered_secname_} = 0;
	}
	my $indfilehandle_ = $secname_to_indfilehandle_map_{$numbered_secname_};
	print $indfilehandle_ join ( ' ', @pnlwords_ )."\n";
	$total_pnl_ += $pnl_ - $secname_to_pnl_map_{$numbered_secname_};
	$secname_to_pnl_map_{$numbered_secname_} = $pnl_;
	$pnlwords_[8] = $total_pnl_;
	print ALL_FILE_HANDLE join ( ' ', @pnlwords_ )."\n";
    }
}
close PNLFILEHANDLE;

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
	open (GP, "|gnuplot -persist ") or die "no gnuplot";
# force buffer to flush after each write
	GP->autoflush(1);

	print GP "set xdata time; \n set timefmt \"\%s\"; set grid \n"; # set terminal X11 size 1080,840 \n set autoscale xy \n show autoscale \n ";
	my $first = 1; # to detect if comman needed
	foreach my $numbered_secname_ ( keys %secname_to_indfilename_map_ ) 
	{
	
	    my $this_pnl_filename_ = $secname_to_indfilename_map_{$numbered_secname_};
	    if ( $first )
	    {
		print GP "plot ";
	    }
	    else
	    {
		print GP ", ";
	    }
	    print GP "\'$this_pnl_filename_\' u 1:$targetcol w l t \"$numbered_secname_\"";
	    $first = 0;
	}
	print GP " \n";
	close GP;

	foreach my $numbered_secname_ ( keys %secname_to_indfilename_map_ ) 
	{
	    my $this_pnl_filename_ = $secname_to_indfilename_map_{$numbered_secname_};
	    if ( -e $this_pnl_filename_ )
	    {
		`rm -f $this_pnl_filename_`;
	    }
	}
    }
    when ("A")
    {
	my $this_pnl_filename_ = $pnl_all_file_;
	if ( -e $pnl_all_file_ )
	{
	open (GP, "|gnuplot -persist ") or die "no gnuplot";
# force buffer to flush after each write
	GP->autoflush(1);
	print GP "set xdata time; \n set timefmt \"\%s\"; \n plot \'$this_pnl_filename_\' using 1:$targetcol with lines title \"$tradesfilebase_\" \n; ";
	close GP;

	    `rm -f $pnl_all_file_`;
	}
    }
}
