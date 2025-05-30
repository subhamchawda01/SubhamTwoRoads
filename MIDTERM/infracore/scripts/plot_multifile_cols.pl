#!/usr/bin/perl
use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE="$0 file1 col1 tag1 [WL/NL] file2 col2 tag2 [WL/NL]\n";
if ( $#ARGV < 3 ) { print $USAGE; exit ( 0 ); }

my @trade_file_list_ = ();
my @col_list_ = ();
my @tag_list_ = ();
my @line_list_ = ( );

for (my $i = 0; $i <= $#ARGV; $i += 4) {
    push (@trade_file_list_, $ARGV[$i]);
    push (@col_list_, $ARGV[$i + 1]);
    push (@tag_list_, $ARGV[$i + 2]);
    push ( @line_list_ , $ARGV [ $i + 3 ] );
}

open (GP, "|gnuplot -persist ") or PrintStacktraceAndDie ( "no gnuplot" );
# force buffer to flush after each write
use FileHandle;
GP->autoflush(1);

print GP "set xdata time; \n set timefmt \"\%s\"; \n set grid \n";

print GP "plot ";

for (my $i = 0; $i < $#trade_file_list_; $i++) {
    if ( $line_list_ [ $i ] eq "WL" )
    {
	print GP "\'$trade_file_list_[$i]\' u 1:$col_list_[$i] w l t \"$tag_list_[$i]\"";
    }
    else
    {
	print GP "\'$trade_file_list_[$i]\' u 1:$col_list_[$i] with points pointtype 1 pointsize 1 t \"$tag_list_[$i]\"";
	print GP "\'$trade_file_list_[$i]\' u 1:$col_list_[$i] t \"$tag_list_[$i]\"";
    }
    print GP ", ";
}

if ( $line_list_ [ $#line_list_ ] eq "WL" )
{
    print GP "\'$trade_file_list_[$#trade_file_list_]\' u 1:$col_list_[$#col_list_] w l t \"$tag_list_[$#tag_list_]\"";
}
else
{
    print GP "\'$trade_file_list_[$#trade_file_list_]\' u 1:$col_list_[$#col_list_] with points pointtype 1 pointsize 1 t \"$tag_list_[$#tag_list_]\"";
}

print GP " \n";
close GP;
