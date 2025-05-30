#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use POSIX;
use List::Util 'sum';

my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

my $MEDIAN_DELAY_PER_DAY_EXEC = "$HOME_DIR"."/basetrade_install/bin/calculate_median_delay_per_day";
my $cached_dir = "/spare/local/files/Sim_Delays/";
my $cached_file = "$cached_dir"."/delay_file";
`mkdir -p $cached_dir`;


require "$GENPERLLIB_DIR/calc_next_business_day.pl"; #
require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; #

my $USAGE="$0 YYYYMMDD input_file";
if ( $#ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }


my %shortcode_to_communication_delay = ();
my %shortcode_to_delay_1 = ();
my %shortcode_to_delay_2 = ();
my %shortcode_to_new_communication_delay = ();
my %date_to_communication_delay=();

my $count = 0;
my $percentile_cutoff_ = 75;

my $date_ = $ARGV[0];
my $prev_file_ = $ARGV[1];
my $error = "";
my $current_date = CalcPrevBusinessDay ( $date_ ) ;

my @prev_file_lines_ = ();

if ( -e $cached_file )
{
	open CURR_FILE_HANDLE, "< $cached_file" ;
	my @cached_file_lines_ = <CURR_FILE_HANDLE>;
	close CURR_FILE_HANDLE;

	for ( my $itr = 0 ; $itr <= $#cached_file_lines_; $itr ++ )
	{
		my @cwords_ = split ( ' ', $cached_file_lines_[$itr] );

		if ( $#cwords_ < 1 )
		{
			next;
		}
		else{
			my $symbol_date_percent = $cwords_[0];
			chomp ($symbol_date_percent);
			my $delay = $cwords_[1];
			chomp ($delay);
			$date_to_communication_delay{$symbol_date_percent} = $delay;
		}
	}
}


if ( -e $prev_file_ )
{
	open CURR_FILE_HANDLE, "< $prev_file_" ;
	@prev_file_lines_ = <CURR_FILE_HANDLE>;
	close CURR_FILE_HANDLE;

	for ( my $itr = 0 ; $itr <= $#prev_file_lines_; $itr ++ )
	{
		my @cwords_ = split ( ' ', $prev_file_lines_[$itr] );

		if ( $#cwords_ < 3 ){
			next;
		}
		else{
			my $shortcode = $cwords_[0];
			$shortcode_to_communication_delay{$shortcode} = $cwords_[1];
			$shortcode_to_delay_1{$shortcode} = $cwords_[2];
			$shortcode_to_delay_2{$shortcode} = $cwords_[3];
		}
	}
}

else{
	$error = "$error"."$prev_file_ not found. Could not generate sim delays.\n";
}

foreach my $shortcode (keys %shortcode_to_communication_delay) {
	$count = 0;
	my $sum = 0;
	my @delay_array = ();

	$current_date = $date_ ;
	if ( index($shortcode, "NSE_") < 0 ) {
		for(my $i = 0; $i < 30; $i++){
			$current_date = CalcPrevBusinessDay ( $current_date ) ;
			my $symbol_date_percent = $shortcode."_".$current_date."_".$percentile_cutoff_;
			if(! exists $date_to_communication_delay{$symbol_date_percent}){
				my $delay = `$MEDIAN_DELAY_PER_DAY_EXEC $shortcode $current_date $percentile_cutoff_`;
				chomp ($delay);
				my $ret_val = `echo $?`;
	
				if($ret_val == 131){
					$error = "$error"."invalid shortcode : $shortcode    Date: $current_date\n";
					last;
				}
	
				if($ret_val == 2){
					$error = "$error"."confirmation before seq message shortcode : $shortcode    Date: $current_date\n";
				}
	
				if($ret_val != 0){
					next;
				}
	
				if ($delay =~ /^-?\d+$/){ 
					$date_to_communication_delay{$symbol_date_percent} = $delay; 
				}
			}
	
			if(exists $date_to_communication_delay{$symbol_date_percent}){
				push @delay_array, $date_to_communication_delay{$symbol_date_percent};
			}
		}
	}
	my $arrSize = @delay_array;

	if($arrSize == 0){
#Ignoring such cases
#$error = "$error"."Could not get values for $shortcode. Keeping old values.\n";
		$shortcode_to_new_communication_delay{$shortcode} = $shortcode_to_communication_delay{$shortcode};
	}
	else{
#if meadian is used
#my @sorted_numbers = sort { $a <=> $b } @delay_array;
#$shortcode_to_new_communication_delay{$shortcode} = $sorted_numbers[floor($arrSize/2)];

#if average is used
		my $average_delay = sum(@delay_array)/@delay_array;
		my $new_delay = ceil($average_delay);

                if ($new_delay > 100000 || $new_delay < 100) {
                  $error = "$error"."IMPORTANT- boundary limits violated for : $shortcode Date: $current_date . Computed_Value: ".$new_delay.". Retaining old value: ".$shortcode_to_communication_delay{$shortcode}."\n";
                  $shortcode_to_new_communication_delay{$shortcode} = $shortcode_to_communication_delay{$shortcode};
                }
                else {
                  $shortcode_to_new_communication_delay{$shortcode} = $new_delay;
                }
	}

	`> $cached_file`;
	open(my $fh, '>', $cached_file);
	foreach my $key (keys %date_to_communication_delay){
		print $fh "$key $date_to_communication_delay{$key}\n";
	} 
	close $fh;
}

for ( my $itr = 0 ; $itr <= $#prev_file_lines_; $itr ++ )
{
  chomp($prev_file_lines_[$itr]);
  my @cwords_ = split ( ' ', $prev_file_lines_[$itr] );
  if ( $#cwords_ < 3 ){
    print "$prev_file_lines_[$itr]\n";
  }
  else{
    my $shortcode = $cwords_[0];
    print "$shortcode $shortcode_to_new_communication_delay{$shortcode} $shortcode_to_delay_1{$shortcode} $shortcode_to_delay_2{$shortcode}\n";
    if(abs($shortcode_to_new_communication_delay{$shortcode} - $shortcode_to_communication_delay{$shortcode}) >= 0.2 * $shortcode_to_communication_delay{$shortcode}){
      $error = "$error"."change in communication delay values by more than 20%: $shortcode old: $shortcode_to_communication_delay{$shortcode} new: $shortcode_to_new_communication_delay{$shortcode}\n";
    }
  }
}

if ($error ne ""){
	open ( MAIL , "|/usr/sbin/sendmail -t" );
	print MAIL "To: nseall@tworoads.co.in\n";
	print MAIL "Subject: Huge difference in sim delay values \n\n";
	print MAIL "$error";
	close(MAIL);
}
