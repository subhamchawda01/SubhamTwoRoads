#!/usr/bin/perl
#
#


use strict;
use warnings;
use FileHandle;

my $EXEC_NAME = $ENV{'HOME'}."/infracore_install/bin/mds_log_reader_bw";
my @final_results_ = ( );
my %data_file_to_exchange_ = ( );
my $USAGE = "$0 EXCHANGE_LIST_FILE";

if( $#ARGV < 0 ) { print "$USAGE\n"; exit( 0 ); }

my $exchange_list_file_ = $ARGV[ 0 ];

open ( EXCHANGE_LIST, "<", $exchange_list_file_ ) or die "Could Not open list file $exchange_list_file_"; #PrintStacktraceAndDie( "Could Not open list file $exchange_list_file_ " );
my @file_lines_ = <EXCHANGE_LIST>; chomp( @file_lines_ );
foreach my $file_line_ ( @file_lines_ )
{
    my @line_words_= split( ' ' , $file_line_ );
    $data_file_to_exchange_{ $line_words_[ 1 ] } = $line_words_[ 0 ];
}

foreach my $data_file_ ( keys %data_file_to_exchange_ )
{
    my $exec_cmd_ = "$EXEC_NAME $data_file_to_exchange_{ $data_file_ } $data_file_";
    my @exchange_results_ = `$exec_cmd_`;
    push( @final_results_ , @exchange_results_ );
}

#foreach my $exchange_( @exchange_list_ )
#{
#    my $exchange_dir_name_ = $EXCHANGE_DIR_NAME."/$exchange_";
#    if( -d $exchange_dir_name_ )
#    {
#	if( opendir my $dh, $exchange_dir_name_ )
#	{
#	    while( my $t_item_ = readdir $dh )
#	    {
#	    	if( $t_item_ =~ /$date_/ )
#	    	{
#			my $full_path_ = $exchange_dir_name_."/".$t_item_;
#			if( -e $full_path_  )
#			{
#		    		my @exchange_results_ = `$EXEC_NAME $exchange_ $full_path_`;
#		    		push ( @final_results_ , @exchange_results_ );
#			}
#		}
#	    }
#	}
#    }
#}
foreach my $result_line_ ( @final_results_ )
{
    print $result_line_;
}
exit( 0 );
