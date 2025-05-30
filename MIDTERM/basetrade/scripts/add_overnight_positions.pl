#!/usr/bin/perl

# \file scripts/sync_dir_to_all_machines.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use POSIX;
use List::Util qw[min max]; # max , min

my $USER = $ENV { 'USER' };
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $exec_ = "/home/pengine/prod/live_execs/user_msg ";
my $contract_specs_exec_ = "/home/dvctrader/basetrade_install/bin/get_contract_specs ";
my $shortcode_for_symbol_exec_ = "/home/dvctrader/basetrade_install/bin/get_shortcode_for_symbol ";
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( $#ARGV < 3 ) {
    print "USAGE: $0 <overnight_filename> <dependant> <send_usermsg(0|1)> <query_id_list> \n";
    exit (0);
}
my $OVERNIGHT_FILENAME_=$ARGV[0];
my $dep_ = $ARGV[1];
my $send_usermsg_ = $ARGV[2];
my @query_id_list_ = ();
for( my $i_=3; $i_<=$#ARGV; $i_++ ) 
{
    push ( @query_id_list_, $ARGV[$i_] );
}

my $date_ = `date +%Y%m%d`; chomp ( $date_ ) ;

if ( -s $OVERNIGHT_FILENAME_ ) 
{
    open OVERNIGHT_POSITION_FILE, " < $OVERNIGHT_FILENAME_" or PrintStacktraceAndDie ( "COuld not open overnight position $OVERNIGHT_FILENAME_ \n" );
    my @lines_ = <OVERNIGHT_POSITION_FILE>; chomp(@lines_);
    close OVERNIGHT_POSITION_FILE;
    foreach my $line_ ( @lines_ )  {
        #BAXH16,-60,99.110000
        my @words_ = split ( /\,/, $line_ ) ;
        if ( $#words_ >= 2 )  {
            my $secname_ = $words_[0];
            my $position_ = $words_[1];
            my $price_ = $words_[2];
            $secname_ = substr ( $secname_, 0, 4 ).substr ( $secname_,5,1);
            my $shortcode_ = `$shortcode_for_symbol_exec_  $secname_ $date_ `; chomp ( $shortcode_ ) ;
            next if ( $shortcode_ ne $dep_ );

            my $pos_sign_ = $position_ > 0 ? 1 : -1;
            $position_ = abs($position_);
            next if ( $position_ == 0 );

            my $lotsize_ = `$contract_specs_exec_ $shortcode_ $date_ LOTSIZE `; chomp ( $lotsize_ ) ;
            print "log: $lotsize_\n";
            my $lotsize_residual_ = $position_ % $lotsize_;
            if ( $lotsize_residual_ != 0 )
            {
                print "Position: $position_, LotSize: $lotsize_ : neglecting Residual: $lotsize_residual_\n";
                $position_ -= $lotsize_residual_;
            }
            my $total_lots_ = int ($position_/$lotsize_);
            my $total_queries_ = $#query_id_list_ + 1;
            my $per_query_lots_ = int($total_lots_/$total_queries_);
            my $residual_lots_ = $total_lots_ % $total_queries_;

            for ( my $j_=0;$j_<= $#query_id_list_; $j_++ ) 
            {
                my $t_position_ = $per_query_lots_ * $lotsize_;
                    if ( $residual_lots_ > 0 ) 
                    { 
                        $t_position_ += $lotsize_;
                        $residual_lots_--;
                    }
                    if ( $t_position_ > 0 )
                    {
                        my $exec_cmd_ = "$exec_ --addposition  ".($t_position_*$pos_sign_)." --traderid $query_id_list_[$j_]";
                        print $exec_cmd_."\n";
                      if ( $send_usermsg_ == 1 )
                      {
                          print "Sending UserMsg...\n";
                          `$exec_cmd_`;
                      }
                    }
            }
        }
    }
}
else 
{
    print STDERR "overnight positions file $OVERNIGHT_FILENAME_ empty or does not exist\n";
}
