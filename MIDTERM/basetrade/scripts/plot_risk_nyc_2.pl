#!/usr/bin/perl
use strict ;
use warnings ;
use feature "switch"; # for given, when
use File::Basename ;
use FileHandle;

my $USAGE="$0 tradesfilename type=[(I)individual|(B)both|(A)all|(F)file]";
if ( $#ARGV < 0 ) { print "$USAGE\n"; exit ( 0 ); }
my $tradesfilename_ = $ARGV[0];
my $type_ = "I";
if ( $#ARGV >= 1 )
{
    $type_ = $ARGV[1];
}
my $output_file_name_ = "";
if ( $type_ eq "F" )
{
    if ( $#ARGV >= 2 )
    {
	$output_file_name_ = $ARGV[2];
    }
    else
    {
	print "need outputfile\n"; exit ( 0 );
    }
}

my $targetcol = 7;
my $cumulative_target_ = 17;

my $tradesfilebase_ = basename ( $tradesfilename_ ); chomp ($tradesfilebase_);
my $tradingdate_ = 20121101; 

if ( $tradesfilebase_ =~ m/pnl_all_cat_file./ )
{
    $tradingdate_ = substr ( $tradesfilebase_, 17, 8 ) ;
}
elsif ( $tradesfilebase_ =~ m/trades./ )
{
    $tradingdate_ = substr ( $tradesfilebase_, 7, 8 );
}
else
{
    print STDERR "debug $tradesfilebase_ $tradingdate_\n";
}

my $HOME_DIR=$ENV{'HOME'};
my $pnl_tempdir=$HOME_DIR."/pnltemp";

my $pnl_all_file_ = $pnl_tempdir."/pnl_all_".$tradesfilebase_;

if ( ! -d $pnl_tempdir )
{
    `mkdir -p $pnl_tempdir`;
}

my $ind_files_needed_ = 0;
my $all_file_needed_ = 0;

given ( $type_ )
{
    when ("B")
    {
	$ind_files_needed_ = 1;
	$all_file_needed_ = 1;
    }
    when ("I")
    {
	$ind_files_needed_ = 1;
    }
    when ("F")
    {
	$ind_files_needed_ = 1;
    }
    when ("A")
    {
	$all_file_needed_ = 1;
    }
}

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $hours_offset_ = 400;
if ( -e "$LIVE_BIN_DIR/get_utc_hhmm_str" )
{ 
    $hours_offset_ = `$LIVE_BIN_DIR/get_utc_hhmm_str EST_0000 $tradingdate_` ; 
    chomp ($hours_offset_);
}

my $hostserver = `hostname` ;

#HK visibility 
if ( index ( $hostserver, "SDV-HK-SRV11" ) == 0 || index ( $hostserver, "SDV-HK-SRV12" ) == 0 ) {  $hours_offset_ = 0 ;} 

my $offset_sec = -36 * $hours_offset_ ;

open PNLFILEHANDLE, "< $tradesfilename_" or die "$0 could not open tradesfile $tradesfilename_\n";

my %secname_to_indfilename_map_ = ();
my %secname_to_indfilehandle_map_ = ();
my %secname_to_pnl_map_ = ();
my $total_pnl_ = 0;
my $GLOBAL_FILE_HANDLE = FileHandle->new;

my $cumulative_secname_ = "CUMULATIVE";

if ( $ind_files_needed_ )
{
	my $out_numbered_secname_ = $cumulative_secname_;
	my $indfilename_ = $pnl_tempdir."/pnl_".$out_numbered_secname_."_".$tradesfilebase_;
	$GLOBAL_FILE_HANDLE->open ( "> $indfilename_" );
	$secname_to_indfilename_map_{$out_numbered_secname_} = $indfilename_;
	$secname_to_indfilehandle_map_{$out_numbered_secname_} = $GLOBAL_FILE_HANDLE;
}


my $arbit_secname_ = "";
if ( $all_file_needed_ )
{
open ALL_FILE_HANDLE , ">$pnl_all_file_" or die "$0 cannot open $pnl_all_file_";
}

while ( my $inline_ = <PNLFILEHANDLE> )
{
    chomp ( $inline_ );
    my @pnlwords_ = split ( ' ', $inline_ );
    if ( $#pnlwords_ >= 8 && ( $pnlwords_[3] eq "B" || $pnlwords_[3] eq "S" ) )
    {
	$pnlwords_[0] += $offset_sec;

	my $numbered_secname_ = $pnlwords_[2]; # used for pnl
	if ( ( ! ( $arbit_secname_ ) ) &&
	     ( $all_file_needed_ ) )
	{
	    $arbit_secname_ = $numbered_secname_ ;
	}

	my $pnl_ = $pnlwords_[6];
	my $cumulative_pnl_ = $pnlwords_[17];

	if ( ! exists $secname_to_pnl_map_{$numbered_secname_} )
	{
	    $secname_to_pnl_map_{$numbered_secname_} = 0;
	}
	my $pnl_change_this_line_ = $pnl_ - $secname_to_pnl_map_{$numbered_secname_} ;
	$secname_to_pnl_map_{$numbered_secname_} = $pnl_;
        $secname_to_pnl_map_{$cumulative_secname_} = $cumulative_pnl_;

	$total_pnl_ += $pnl_change_this_line_;

	if ( $ind_files_needed_ )
	{
	    my $out_numbered_secname_ = $numbered_secname_ ; # used for file naming
	    if ( ! ( exists $secname_to_indfilehandle_map_{$out_numbered_secname_} ) )
	    {
		my $indfilename_ = $pnl_tempdir."/pnl_".$out_numbered_secname_."_".$tradesfilebase_;
		my $indfilehandle_ = FileHandle->new;
		$indfilehandle_->open ( "> $indfilename_" );
		
		$secname_to_indfilename_map_{$out_numbered_secname_} = $indfilename_;
		$secname_to_indfilehandle_map_{$out_numbered_secname_} = $indfilehandle_;
	    }
	    my $indfilehandle_ = $secname_to_indfilehandle_map_{$out_numbered_secname_};
	    
	    print $indfilehandle_ join ( ' ', @pnlwords_ )."\n";
	    print $GLOBAL_FILE_HANDLE join(' ', @pnlwords_)."\n";
	}

	if ( $all_file_needed_ )
	{
	    $pnlwords_[6] = $total_pnl_;
	    print ALL_FILE_HANDLE join ( ' ', @pnlwords_ )."\n";
	}
    }
}
close PNLFILEHANDLE;

{
    foreach my $numbered_secname_ ( keys %secname_to_indfilehandle_map_ )
    {
	$secname_to_indfilehandle_map_{$numbered_secname_}->close ;
    }
}
if ( $all_file_needed_ )
{
    close ALL_FILE_HANDLE ;
}

given ( $type_ )
{
    when ("I")
    {
	open (GP, "|gnuplot -persist ") or die "no gnuplot";
# force buffer to flush after each write
	GP->autoflush(1);

	print GP "set xdata time; \n set timefmt \"\%s\"; set grid \n set title \'$tradingdate_ (EST)\'; \n "; # set terminal X11 size 1080,840 \n set autoscale xy \n show autoscale \n ";
	my $first = 1; # to detect if comman needed
	foreach my $numbered_secname_ ( keys %secname_to_indfilename_map_ ) 
	{
		if($numbered_secname_ eq $cumulative_secname_){
			next;
		}
		
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
	my $this_pnl_filename_ = $secname_to_indfilename_map_{$cumulative_secname_};
	print GP ",";
	print GP "\'$this_pnl_filename_\' u 1:$cumulative_target_ w l t \"$cumulative_secname_\"";
	
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
    when ("F")
    {
	open (GP, "|gnuplot -persist ") or die "no gnuplot";
# force buffer to flush after each write
	GP->autoflush(1);

	print GP "set terminal png; \n set out \'$output_file_name_\'; \n set xdata time; \n set timefmt \"\%s\"; set grid \n set title \'$tradingdate_\'; \n"; # set terminal X11 size 1080,840 \n set autoscale xy \n show autoscale \n ";
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
	print GP "set xdata time; \n set timefmt \"\%s\"; set grid; \n set title \'$tradingdate_\'; \n plot \'$this_pnl_filename_\' using 1:$targetcol with lines title \"$arbit_secname_\" \n; ";
	close GP;

	    `rm -f $pnl_all_file_`;
	}
    }
}
