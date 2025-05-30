#!/usr/bin/perl

use strict;
use warnings;
use FileHandle;
use Fcntl qw (:flock);

my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE = "$0 NEW_FILE DEFAULT_FILE";

if( $#ARGV < 1 ) { print "$USAGE\n"; exit( 0 ); }

open( FILE1, "<" , $ARGV[0]) or PrintStacktraceAndDie( "Could Not open file $ARGV[ 0 ]\n" );
my @file1_lines_ = <FILE1>; chomp ( @file1_lines_ );
close( FILE1 );

open( FILE2, "<" , $ARGV[1]) or PrintStacktraceAndDie( "Could Not open file $ARGV[ 1 ]\n");
my @file2_lines_ = <FILE2>; chomp ( @file2_lines_ );
close( FILE2 );
my $summary_ = "LRDB add new pairs summary \n new_file: ".$ARGV[0]."\n default_file: ".$ARGV[1]."WARNS:\n";
my $asummary_ = "ALERTS:\n";

my @already_present_ = ( );
my %already_present_lrdb_ = ( );
my %already_present_corr_ = ( );
my %already_present_time_ = ( );

my @new_present_ = ( );

my $send_email_ = 0 ;
foreach my $file2_line_( @file2_lines_ )
{
    my @file2_line_words_ = split( ' ' , $file2_line_ );
    if( $#file2_line_words_ < 3 )
    {
	next;
    }
    push( @already_present_ , $file2_line_words_[ 0 ] );
    $already_present_lrdb_{$file2_line_words_[ 0 ]} = $file2_line_words_[ 1 ] + 0;
    $already_present_corr_{$file2_line_words_[ 0 ]} = $file2_line_words_[ 2 ] + 0;
    $already_present_time_{$file2_line_words_[ 0 ]} = $file2_line_words_[ 3 ] ;
}

my @new_lines_ = ( );
my @all_new_lines_ = ( );
foreach my $file1_line_( @file1_lines_ )
{
    my @file1_line_words_ = split( ' ' , $file1_line_ );
    if ( $#file1_line_words_ < 3 )
    {
	next;
    }

    push ( @new_present_, $file1_line_words_[0] );

    if ( ! FindItemFromVec ( $file1_line_words_[ 0 ], @already_present_ ) )
    {
	push ( @new_lines_ , $file1_line_ );
	push ( @all_new_lines_ , $file1_line_ );
    }
    else
    {
	my $new_lrdb_ = $file1_line_words_[ 1 ] + 0;
	my $new_corr_ = $file1_line_words_[ 2 ] + 0;
	#if (($new_lrdb_ > 0.95 * $already_present_lrdb_{$file1_line_words_[ 0 ]}) && ($new_lrdb_ < 1.05 * $already_present_lrdb_{$file1_line_words_[ 0 ]}) && ($new_corr_ > 0.95 * $already_present_corr_{$file1_line_words_[ 0 ]}) && ($new_corr_ < 1.05 * $already_present_corr_{$file1_line_words_[ 0 ]}))
	#{
	#	$already_present_lrdb_{$file1_line_words_[ 0 ]} = $new_lrdb_;
	#	$already_present_corr_{$file1_line_words_[ 0 ]} = $new_corr_;
	#	print "In range for ".$file1_line_words_[ 0 ]."\n";
	#}
	my $old_corr_ = $already_present_corr_{$file1_line_words_[0]} ;
	my $old_beta_ = $already_present_lrdb_{$file1_line_words_[0]} ;

	if ( $old_corr_ * $new_corr_ <= 0 )
	{
		$asummary_ = $asummary_."ALERT: Sign Flip : ".$file1_line_words_[ 0 ]." : old beta ".$old_beta_." new beta ".$new_lrdb_." old corr ".$old_corr_." new corr ".$new_corr_."\n";
	}
	elsif ((abs($new_corr_) > 0.75 * abs($already_present_corr_{$file1_line_words_[ 0 ]})) && (abs($new_corr_) < 1.25 * abs($already_present_corr_{$file1_line_words_[ 0 ]})))
	{
		$already_present_lrdb_{$file1_line_words_[ 0 ]} = $new_lrdb_;
		$already_present_corr_{$file1_line_words_[ 0 ]} = $new_corr_;
		#$summary_ = $summary_."In range for ".$file1_line_words_[ 0 ].", so new values added\n";
	}
	else
	{
		$summary_ = $summary_."WARN: Magnitude Drift : ".$file1_line_words_[ 0 ]." : old beta ".$old_beta_." new beta ".$new_lrdb_." old corr ".$old_corr_." new corr ".$new_corr_."\n";
	}
	my $line_ = join( ' ' ,$file1_line_words_[ 0 ],$already_present_lrdb_{$file1_line_words_[ 0 ]},$already_present_corr_{$file1_line_words_[ 0 ]},$file1_line_words_[ 3 ]);
	push ( @new_lines_ , $line_ );

	my $nline_ = join( ' ' ,$file1_line_words_[ 0 ],$new_lrdb_,$new_corr_,$file1_line_words_[ 3 ]);
	push ( @all_new_lines_ , $nline_ ) ;

    }
}

foreach my $pair_ ( @already_present_ )
{
    if ( ! FindItemFromVec ( $pair_ , @new_present_ ) )
    {
	my $line_ = join ( ' ', $pair_, $already_present_lrdb_{$pair_}, $already_present_corr_{$pair_}, $already_present_time_{$pair_} );
	push ( @new_lines_, $line_ );
	push ( @all_new_lines_ , $line_ ) ;
    }
}

`cp $ARGV[1] $ARGV[1]"_backup"`;
open( FILE2, ">" , $ARGV[1]) or PrintStacktraceAndDie( "Could Not open file $ARGV[1]\n" );
flock ( FILE2, LOCK_EX );
foreach my $new_line_( @new_lines_ )
{
    print FILE2 "$new_line_\n";
}
close( FILE2 );

open( FILE2, ">" , $ARGV[1]."_allnew") or PrintStacktraceAndDie( "Could Not open file $ARGV[1]\n" );
flock (FILE2, LOCK_EX );
foreach my $new_line_( @all_new_lines_ )
{
    print FILE2 "$new_line_\n";
}
close( FILE2 );

my $yymmdd=`date +%Y%m%d`;
my $SUB="LRDB_RUN_SUMMARY-".$yymmdd ;


if ( $send_email_ == 1 )
{
open ( MAIL , "|/usr/sbin/sendmail -t" );

print MAIL "To: nseall@tworoads.co.in\n";
print MAIL "From: nseall@tworoads.co.in\n";
print MAIL "Subject: ".$SUB;

print MAIL $summary_ ;
print MAIL $asummary_;

close(MAIL);
}
#`echo $summary_ > summary.txt`;
#my $mail= "/bin/mail -s ".$SUB." nseall@tworoads.co.in < summary.txt";
#`echo $mail`;
exit( 0 );


