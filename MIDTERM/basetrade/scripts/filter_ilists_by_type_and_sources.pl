#!/usr/bin/perl

# \file scripts/merge_three_ilists.pl
#
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 162, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#

use strict;
use warnings;

sub ProcLine ;

#my $SPARE_DIR="/spare/local/basetrade/";
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE="$0 SHORTCODE ilist_filename1_ ilist_filename2_ .... INDICATOR_TYPES .... SOURCES .... ";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV[0];
my @ilist_vec_ = ( );
my @indicator_type_vec_ = ( ) ;
my @sources_vec_ = ( ) ;
my $mode_ = 0;

for ( my $i = 1 ; $i <= $#ARGV; $i ++ )
{
    if ($mode_ == 0 )
    {
	if ( $ARGV[$i] eq "INDICATOR_TYPES" )
	{
	    $mode_ = 1;
	    next;
	}
	else
	{
	    push ( @ilist_vec_ , $ARGV[$i] );
	}
    }
    elsif ( $mode_ == 1 )
    {
	if ( $ARGV[$i] eq "SOURCES" )
	{
	    $mode_ = 2;
	    next;
	}
	else
	{
	    push ( @indicator_type_vec_, $ARGV[$i] );
	}
    }
    else
    {
	push ( @sources_vec_, $ARGV[$i] );
    }
}

my $indicator_types_filename_ = $HOME_DIR."/modelling/stratwork/indicator_types.txt";
my $sources_filename_ = $HOME_DIR."/modelling/stratwork/".$shortcode_."/sources.txt";

open IND_TYPE, "< $indicator_types_filename_" or PrintStacktraceAndDie ( "$0 could not open $indicator_types_filename_\n" );

my @permitted_indicators_ = ( );
$mode_ = "NONE";
my $txt_ ="";
my $txt_to_append_ = "";
while( $txt_ = <IND_TYPE> )
{
    chomp($txt_);
    
    if( substr ( $txt_, 0, 1) eq '#' )
    {
	next;
    }
    if($txt_ eq "BOOK" )
    {
	if ( $txt_ ~~ @indicator_type_vec_  )
	{
	   $txt_to_append_ = $txt_to_append_.".bk";
	   $mode_ = "PICK";
	}
	else
	{
	   $mode_ = "NONE";
	}
	next;
    }
    elsif ($txt_ eq "TRADE" )
    {
	if ( $txt_ ~~ @indicator_type_vec_ )
        {
	   $txt_to_append_ = $txt_to_append_.".trd";
	   $mode_ = "PICK";	
	}
	else
	{
	   $mode_ = "NONE"
	}
	next;
    }
    elsif ($txt_ eq "TREND" )
    {
	if( $txt_ ~~ @indicator_type_vec_ )
        {
	   $txt_to_append_ = $txt_to_append_.".trnd";
	   $mode_ = "PICK";
	}
	else
	{
	   $mode_ = "NONE";
	}
	next;
    }
    elsif ($txt_ eq "ONLINE" )
    {
	if( $txt_ ~~ @indicator_type_vec_ )
	{
	   $txt_to_append_ = $txt_to_append_.".on";
	   $mode_ = "PICK";
	}
	else
	{
	   $mode_ = "NONE";
	}
	next;
    }
    elsif ($txt_ eq "OFFLINE" )
    {
	if( $txt_ ~~ @indicator_type_vec_ )
	{
	   $txt_to_append_ = $txt_to_append_.".off";
	   $mode_ = "PICK";
	}
	else
	{
	   $mode_ = "NONE";
	}
	next;
    }
    elsif ($mode_ eq "NONE" )
    {
	next;
    }
    else
    {
	push ( @permitted_indicators_, $txt_ );
    }
}

close IND_TYPE;

my @permitted_sources_ = ( );
$mode_ = "NONE";

open SOURCES, "< $sources_filename_" or PrintStacktraceAndDie ( "$0 could not open $sources_filename_\n" );

while($txt_ = <SOURCES> )
{
    chomp($txt_);
    if( substr ( $txt_, 0, 1) eq '#' )
    {
        next;
    }
    if($txt_ eq "SELF" )
    {
        $mode_ = "PICK";
        next;
    }
    elsif ($txt_ eq "BMF_MAIN" )
    {
        if ( $txt_ ~~ @sources_vec_ )
        {
	   if ( $txt_to_append_ =~ /bmf/ ) {}
	   else
	   {
	     $txt_to_append_ = $txt_to_append_.".bmf";	   
	   }
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE"
        }
        next;
    }
    elsif ($txt_ eq "BMF_ALL" )
    {
        if($txt_ ~~ @sources_vec_ )
        {
	   if ( $txt_to_append_ =~ /bmf/ ) {}
	   else
	   {
	     $txt_to_append_ = $txt_to_append_.".bmf";
	   }
	   $txt_to_append_ = $txt_to_append_.".bmf";
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE";
        }
        next;
    }
    elsif ($txt_ eq "CME_MAIN" )
    {
        if( $txt_ ~~ @sources_vec_ )
        {
	   if ( $txt_to_append_ =~ /cme/ ) {}
	   else
	   {
	      $txt_to_append_ = $txt_to_append_.".cme";
	   }
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE";
        }
        next;
    }
    elsif ($txt_ eq "CME_ALL" )
    {
        if( $txt_ ~~ @sources_vec_ )
        {
           if ( $txt_to_append_ =~ /cme/ ) {}
           else
           {
              $txt_to_append_ = $txt_to_append_.".cme";
           }
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE";
        }
        next;
    }
    elsif ($txt_ eq "NDQ_MAIN" )
    {
        if( $txt_ ~~ @sources_vec_ )
        {
           if ( $txt_to_append_ =~ /ndq/ ) {}
           else
           {
              $txt_to_append_ = $txt_to_append_.".ndq";
           }
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE";
        }
        next;
    }
    elsif ($txt_ eq "NDQ_ALL" )
    {
        if( $txt_ ~~ @sources_vec_ )
        {
           if ( $txt_to_append_ =~ /ndq/ ) {}
           else
           {
              $txt_to_append_ = $txt_to_append_.".ndq";
           }
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE";
        }
        next;
    }
    elsif ($txt_ eq "EUREX_MAIN" )
    {
        if( $txt_ ~~ @sources_vec_ )
        {
           if ( $txt_to_append_ =~ /eu/ ) {}
           else
           {
              $txt_to_append_ = $txt_to_append_.".eu";
           }	
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE";
        }
        next;
    }
    elsif ($txt_ eq "EUREX_ALL" )
    {
        if( $txt_ ~~ @sources_vec_ )
        {
           if ( $txt_to_append_ =~ /eu/ ) {}
           else
           {
              $txt_to_append_ = $txt_to_append_.".eu";
           }
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE";
        }
        next;
    }
    elsif ($txt_ eq "LIFFE_MAIN" )
    {
        if( $txt_ ~~ @sources_vec_ )
        {
           if ( $txt_to_append_ =~ /lif/ ) {}
           else
           {
              $txt_to_append_ = $txt_to_append_.".lif";
           }	
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE";
        }
        next;
    }
    elsif ($txt_ eq "LIFFE_ALL" )
    {
        if( $txt_ ~~ @sources_vec_ )
        {
           if ( $txt_to_append_ =~ /lif/ ) {}
           else
           {
              $txt_to_append_ = $txt_to_append_.".lif";
           }
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE";
        }
        next;
    }   
    elsif ($txt_ eq "HK_MAIN" )
    {
        if( $txt_ ~~ @sources_vec_ )
        {
           if ( $txt_to_append_ =~ /hk/ ) {}
           else
           {
              $txt_to_append_ = $txt_to_append_.".hk";
           }
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE";
        }
        next;
    }
    elsif ($txt_ eq "HK_ALL" )
    {
        if( $txt_ ~~ @sources_vec_ )
        {
           if ( $txt_to_append_ =~ /hk/ ) {}
           else
           {
              $txt_to_append_ = $txt_to_append_.".hk";
           }
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE";
        }
        next;
    }
    elsif ($txt_ eq "OSE_MAIN" )
    {
        if( $txt_ ~~ @sources_vec_ )
        {
           if ( $txt_to_append_ =~ /ose/ ) {}
           else
           {
              $txt_to_append_ = $txt_to_append_.".ose";
           }	
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE";
        }
        next;
    }
    elsif ($txt_ eq "OSE_ALL" )
    {
        if( $txt_ ~~ @sources_vec_ )
        {
           if ( $txt_to_append_ =~ /ose/ ) {}
           else
           {
              $txt_to_append_ = $txt_to_append_.".ose";
           }
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE";
        }
        next;
    }
    elsif ($txt_ eq "TMX_MAIN" )
    {
        if( $txt_ ~~ @sources_vec_ )
        {
           if ( $txt_to_append_ =~ /tmx/ ) {}
           else
           {
              $txt_to_append_ = $txt_to_append_.".tmx";
           }
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE";
        }
        next;
    }
    elsif ($txt_ eq "TMX_ALL" )
    {
        if( $txt_ ~~ @sources_vec_ )
        {
           if ( $txt_to_append_ =~ /tmx/ ) {}
           else
           {
              $txt_to_append_ = $txt_to_append_.".tmx";
           }
           $mode_ = "PICK";
        }
        else
        {
           $mode_ = "NONE";
        }
        next;
    }
    elsif ($mode_ eq "NONE" )
    {
        next;
    }
    else
    {
        push ( @permitted_sources_, $txt_ );
    }
}

close SOURCES;

my $filtered_ilist_filename_ = "";
my $exec_cmd_ = "";
for ( my $i=0; $i <= $#ilist_vec_ ; $i ++ )
{
        $filtered_ilist_filename_ = $ilist_vec_[$i].$txt_to_append_;
	my $t_filtered_ilist_filename_ = $ilist_vec_[$i].".tmp";
        	
	for (my $j=0; $j <= $#permitted_indicators_ ; $j ++ )
	{	   
	   for (my $k= 0; $k <= $#permitted_sources_ ; $k++ )
	   {
		my $indicator_ = $permitted_indicators_[$j];
		my $source_ = $permitted_sources_[$k];
		
		if ( ( $indicator_ =~ /Online/ || $indicator_ =~ /Offline/ || $indicator_ =~ /PCA/ ) && $source_ =~ /$shortcode_/ )
		{
			next;
		}
		
		$exec_cmd_ = "cat $ilist_vec_[$i] | grep $indicator_ ";
		if ( $indicator_ =~ /"MktEvents"/ )
		{}
		else
		{
		   $exec_cmd_ = $exec_cmd_." | grep -v MktEvents ";
		}
		
	        $exec_cmd_ = $exec_cmd_." | grep $source_ >> $t_filtered_ilist_filename_";
	        `$exec_cmd_`;
	   }
	}

	$exec_cmd_ = "cat $ilist_vec_[$i] | head -n3 > $filtered_ilist_filename_";	
	`$exec_cmd_`;

	$exec_cmd_ = "cat $t_filtered_ilist_filename_ | sort -u >> $filtered_ilist_filename_";
	`$exec_cmd_`;

        $exec_cmd_ = "cat $ilist_vec_[$i] | tail -n1 >> $filtered_ilist_filename_";
        `$exec_cmd_`;

	$exec_cmd_ = "rm -f $t_filtered_ilist_filename_";
	`$exec_cmd_`;

}
