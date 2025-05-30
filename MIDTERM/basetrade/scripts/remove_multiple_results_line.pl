#!/usr/bin/perl

use strict;
use warnings;
use FileHandle;

my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE = "$0 GlobalResultsDirectory";

if( $#ARGV < 0 ) { print "$USAGE\n"; exit( 0 ); }

my $global_results_directory_ = $ARGV[0];

my $tmp_filename_ = "~/tmp_filename";

my $exec_cmd_ = "find $global_results_directory_ | grep database";

my @output_lines_ = `$exec_cmd_`;

my @strats_ = ( );

foreach my $results_file_ ( @output_lines_ )
{
    chomp($results_file_);
    $exec_cmd_ = "cat $results_file_";
    my @results_lines_ = `$exec_cmd_`;

    $exec_cmd_ = "rm -f $tmp_filename_";    
    `$exec_cmd_`;

    @strats_ = ( );

    

    foreach my $results_line_ ( @results_lines_ )
    {
	chomp($results_line_);
    	my @results_line_words_ = split( ' ', $results_line_ );
	if ( @results_line_words_ <= 0 )
	{
	    next;
	}
	my $stratname_ = $results_line_words_[0];
	
        if ( ! FindItemFromVec ( $stratname_, @strats_ ) )    
        {
            push ( @strats_ , $stratname_ );
        }    
        else
        {
	    next;
        }	    	

	$exec_cmd_ = "echo $results_line_ >> $tmp_filename_";
	`$exec_cmd_`;
    }
    
    $exec_cmd_ = "cp $tmp_filename_ $results_file_";
    `$exec_cmd_`;
}
