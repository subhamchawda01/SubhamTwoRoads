#!/usr/bin/perl
use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $USAGE="$0 sim_log_file C/E/I/O START_TIME END_TIME\n\tC=Cxl\tE=Exec\tI=Indicator\tO=Order";

if ( $#ARGV < 1 ) 
{ 
    printf "$USAGE\n"; 
    exit (0); 
}

my $log_file_ = $ARGV [ 0 ];
my $print_options_ = $ARGV [ 1 ];

my $start_time_ = 0;
my $end_time_ = 9999999999.999999;

if ( $#ARGV >= 3 )
{
    $start_time_ = $ARGV [ 2 ];
    $end_time_ = $ARGV [ 3 ];
}

my $confirmed_mine_cmd_ = "grep OrderConfirmed $log_file_ | grep -v Show";
my @confirmed_list_ = `$confirmed_mine_cmd_`;

my %saos_to_conf_time_ = ( );
my %saos_to_cxl_time_ = ( );
my %saos_to_exec_time_ = ( );

my %saos_to_price_ = ( );
my %saos_to_buysell_ = ( );
my %saos_to_size_ = ( );

for ( my $i = 0; $i <= $#confirmed_list_ ; $i ++ )
{
    my @conf_words_ = split ( ' ' , $confirmed_list_ [ $i ] );

    my $t_time_ = $conf_words_ [ 0 ];
    my $t_saos_ = $conf_words_ [ 5 ];
    my $t_price_ = $conf_words_ [ 11 ];
    my $t_buysell_ = $conf_words_ [ 13 ];
    my $t_size_ = $conf_words_ [ 15 ];

    $saos_to_conf_time_ { $t_saos_ } = $t_time_;

    $saos_to_price_ { $t_saos_ } = $t_price_;
    $saos_to_buysell_ { $t_saos_ } = substr ( $t_buysell_ , 0 , 1 );
    $saos_to_size_ { $t_saos_ } = $t_size_;
}

if ( $print_options_ eq "E" )
{
    my $executed_mine_cmd_ = "grep OrderExecuted $log_file_ | grep msecs";
    my @executed_list_ = `$executed_mine_cmd_`;

    for ( my $i = 0 ; $i <= $#executed_list_ ; $i ++ )
    {
	my @exec_words_ = split ( ' ' , $executed_list_ [ $i ] );
	
	my $t_time_ = $exec_words_ [ 0 ];
	my $t_saos_ = $exec_words_ [ 8 ];
	
	if ( $t_time_ >= $start_time_ && $t_time_ <= $end_time_ )
	{
	    $saos_to_exec_time_ { $t_saos_ } = $t_time_;
	}
    }
    
    print "   EXEC-TIMESTAMP SAOS        PRICE BS    CONF-TIMESTAMP\n";

    foreach my $saos_ ( sort { $saos_to_exec_time_ { $a } <=> $saos_to_exec_time_ { $b } }
			keys %saos_to_exec_time_ )
    {
	printf "%17f %4d %12.5f %2s %17f\n" , $saos_to_exec_time_ { $saos_ } , $saos_ , $saos_to_price_ { $saos_ } , $saos_to_buysell_ { $saos_ } , $saos_to_conf_time_ { $saos_ };
    }
}
elsif ( $print_options_ eq "C" )
{
    my $cxled_mine_cmd_ = "grep OrderCanceled $log_file_ | grep SmartOrderManager | grep -v Show";
    my @cxled_list_ = `$cxled_mine_cmd_`;

    for ( my $i = 0 ; $i <= $#cxled_list_ ; $i ++ )
    {
	my @cxl_words_ = split ( ' ' , $cxled_list_ [ $i ] );

	my $t_time_ = $cxl_words_ [ 0 ];
	my $t_saos_ = $cxl_words_ [ 5 ];

	if ( $t_time_ >= $start_time_ && $t_time_ <= $end_time_ )
	{
	    $saos_to_cxl_time_ { $t_saos_ } = $t_time_;
	}
    }

    print "    CXL-TIMESTAMP SAOS        PRICE BS    CONF-TIMESTAMP\n";

    foreach my $saos_ ( sort { $saos_to_cxl_time_ { $a } <=> $saos_to_cxl_time_ { $b } }
			keys %saos_to_cxl_time_ )
    {
	printf "%17f %4d %12.5f %2s %17f\n" , $saos_to_cxl_time_ { $saos_ } , $saos_ , $saos_to_price_ { $saos_ } , $saos_to_buysell_ { $saos_ } , $saos_to_conf_time_ { $saos_ };
    }
}
elsif ( $print_options_ eq "I" )
{
    my $indicator_mine_cmd_ = "grep \"value\\|DumpIndicatorValues\" $log_file_";
    my @indicator_list_ = `$indicator_mine_cmd_`;

    my $to_print_ = 0;

    for ( my $i = 0 ; $i <= $#indicator_list_ ; $i ++ )
    {
	if ( index ( $indicator_list_ [ $i ] , "DumpIndicatorValues" ) >= 0 )
	{
	    my @indicator_words_ = split ( ' ' , $indicator_list_ [ $i ] );

	    my $t_time_ = $indicator_words_ [ 0 ];

	    if ( $t_time_ >= $start_time_ && $t_time_ <= $end_time_ )
	    {
		printf "\n%17f " , $t_time_;
		$to_print_ = 1;
	    }
	    else
	    {
		$to_print_ = 0;
	    }
	}
	elsif ( $to_print_ == 1 )
	{
	    my @indicator_words_ = split ( ' ' , $indicator_list_ [ $i ] );

	    my $t_value_ = $indicator_words_ [ 1 ];
	    printf "%9.6f " , $t_value_;
	}
    }

    printf "\n";
}
elsif ( $print_options_ eq "O" )
{
    my $order_mine_cmd_ = "grep OnMarketUpdate $log_file_";

    if ( $#ARGV >= 2 )
    {
	my $saos_ = $ARGV [ 2 ];
	$order_mine_cmd_ = $order_mine_cmd_." | grep \" SAOS: $saos_ \"";
    }

    my @order_list_ = `$order_mine_cmd_`;

    print "        TIMESTAMP SAOS        PRICE BS           QUEUE MKTSZ MKTORD S_AHEAD S_BEHIND NUMEVENTS\n";

    for ( my $i = 0 ; $i <= $#order_list_ ; $i ++ )
    {
	my @order_words_ = split ( ' ' , $order_list_ [ $i ] );

	my $t_time_ = $order_words_ [ 0 ];
	my $t_buysell_ = $order_words_ [ 4 ];
	my $t_price_ = $order_words_ [ 5 ];
	my $t_size_ = $order_words_ [ 6 ];
	my $t_queue_stats_ = $order_words_ [ 9 ];

	my @queue_words_ = split ( '-' , $t_queue_stats_ );

	my $t_saos_ = $order_words_ [ 11 ];
	my $t_mktsz_ = $order_words_ [ 15 ];
	my $t_mktord_ = 0;
	if ( $#order_words_ >= 18 ) { $t_mktord_ = $order_words_ [ 18 ]; }

	printf "%17f %4d %12.5f %2s %15s %5d %6d %7d %8d %9d\n" , 
	$t_time_ , $t_saos_ , $t_price_ , $t_buysell_ , $t_queue_stats_ , $t_mktsz_ , $t_mktord_ , 
	substr ( $queue_words_ [ 0 ] , 1 ) , $queue_words_ [ 1 ] , substr ( $queue_words_ [ 2 ] , 0 , -1 );
    }
}
