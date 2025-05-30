# \file GenPerlLib/get_bad_days_for_shortcode.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
use strict;
use warnings;
use feature "switch";

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $STORE="/spare/local/tradeinfo";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
if ( $USER eq "ankit" || $USER eq "kputta" || $USER eq "rkumar" || $USER eq "diwakar" )
{
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/find_item_from_vec_ref.pl"; #FindItemFromVecRef
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_and_tt.pl"; # MakeStratVecFromDirAndTT
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/array_ops.pl";

sub GetBadDaysForShortcode
{
    my $shortcode_ = shift;
    my ($array_ref) = shift;

    my $bad_day_file_ = $STORE."/datageninfo/bad_days.".$shortcode_;

    if ( -e $bad_day_file_ )
    {
	open BAD_DAY_FILE_HANDLE, "< $bad_day_file_ " or PrintStacktraceAndDie ( "$0 Could not open $bad_day_file_\n" );

	while ( my $thisline_ = <BAD_DAY_FILE_HANDLE> ) 
	{
	    chomp ( $thisline_ );
	    my @rwords_ = split ( ' ', $thisline_ );
	    if ( $#rwords_ >= 0 )
	    {
		if ( ( ValidDate ( $rwords_[0] ) ) &&
		     ( ! ( SkipWeirdDate ( $rwords_[0] ) || IsDateHoliday ( $rwords_[0] ) ) ) )
		{
		    push ( @$array_ref, $rwords_[0] );
		}
	    }
	}

	close BAD_DAY_FILE_HANDLE;
    }
}

sub GetBadDaysPoolForShortcode
{
  my $shortcode_ = shift;
  my $timeperiod_ = shift;
  my $start_date_ = shift;
  my $end_date_ = shift;
  my ($array_ref) = shift;
  my $days_percentage_ = shift || 0.2;

  my $strats_cutoff_frac_ = 0.8;

  print "GetBadDaysPoolForShortcode ".$shortcode_." $timeperiod_ $start_date_ $end_date_ $days_percentage_\n";
  my $global_results_dir_path = "/NAS1/ec2_globalresults/";
  my $srv_name_=`hostname | cut -d'-' -f2`; chomp($srv_name_);
  if($srv_name_ =~ "crt")
  {
    $global_results_dir_path = $HOME_DIR."/ec2_globalresults/";
  }

   my @all_strats_in_dir_ = MakeStratVecFromDirAndTT($MODELING_STRATS_DIR."/".$shortcode_, $timeperiod_);
   
   my $cstempfile_ = GetCSTempFileName ( "cstemp_allstrats_".$shortcode_."_".$timeperiod_ );
   open CSTF, "> $cstempfile_" or PrintStacktraceAndDie ( "Could not open $cstempfile_ for writing\n" );
   for(my $i=0; $i <= $#all_strats_in_dir_; $i++){
     my $t_strat_file_ = basename($all_strats_in_dir_[$i]);
     print CSTF "$t_strat_file_\n";
   }
   close CSTF;

   my $exec_cmd_="$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $cstempfile_ $global_results_dir_path $start_date_ $end_date_ INVALIDFILE kCNAPnlSharpe 0 IF 0";
   my @global_res_out_ = `$exec_cmd_`; chomp ( @global_res_out_ );

   my %strat2date2pnl_ = ();
   my %strat2sharpe_ = ();
   my %strat2avgDD_ = ();
   my $strat_name_ = "";
   my @dates_vec_ = ();

   foreach my $exec_output_line_ ( @global_res_out_ )
   {
     if ( $exec_output_line_ eq "\n" ) {
       next;
     }
     my @exec_output_words_ = split(" ", $exec_output_line_);
     if ( $#exec_output_words_ < 0 ) { next; }
     if ( $exec_output_words_[0] eq "STATISTICS" ) {
       $strat2sharpe_{ $strat_name_ } = $exec_output_words_[4];
       $strat2avgDD_{ $strat_name_ } = $exec_output_words_[14];
     }
     elsif ( $exec_output_words_[0] eq "STRATEGYFILEBASE" ) {
       $strat_name_ = $exec_output_words_[1];
     }
     else {
       my $t_date_ = $exec_output_words_[0];
       $strat2date2pnl_ { $strat_name_ }{ $t_date_ } = $exec_output_words_[1];
       if ( ! FindItemFromVecRef ( $t_date_, \@dates_vec_ ) ) {
	       push ( @dates_vec_, $t_date_ );
       }
     }
   }

   my @strats_to_consider_ = grep { $strat2sharpe_{ $_ } > 0 } keys %strat2sharpe_;

   my %dates2scores_ = ();
   foreach my $t_date_ ( @dates_vec_ ) {
     my %strat2pnl_dd_ = ();
     foreach my $strat_name_ ( @strats_to_consider_ ) {
	     if ( exists $strat2date2pnl_ { $strat_name_ }{ $t_date_ } ) {
		     $strat2pnl_dd_{ $strat_name_ } = $strat2date2pnl_ { $strat_name_ }{ $t_date_ } / ( 1 + $strat2avgDD_{ $strat_name_  } );
	     }
     }
     my @strats_sorted_ = sort { $strat2pnl_dd_{ $a } <=> $strat2pnl_dd_{ $b } } keys %strat2pnl_dd_;

   my @sharpe_array_ = @strat2sharpe_{ @strats_sorted_ };
   my $sharpe_sum_ = GetSum ( \@sharpe_array_ );
   my $cutoff_sharpe_pos_ = $strats_cutoff_frac_  * $sharpe_sum_; 
   
  my $t_pos_ = 0;
     foreach my $t_strat_ ( @strats_sorted_ ) { 
       $t_pos_ += $strat2sharpe_{ $t_strat_ }; 
       if ( $t_pos_ > $cutoff_sharpe_pos_ ) { 
         $dates2scores_{ $t_date_ } = $strat2pnl_dd_ { $t_strat_ };
         last;
       }
     }
#     print "score: $t_date_: ".$dates2scores_{ $t_date_ }."\n";
   }

   my @dates_sorted_ = sort { $dates2scores_{ $a } <=> $dates2scores_{ $b } } @dates_vec_;
#   print join(" ", @dates_sorted_)."\n";
   my $num_bad_days_ = ceil ( @dates_sorted_ * $days_percentage_ );
   @$array_ref = @dates_sorted_ [ 0..($num_bad_days_-1) ];
}

1
