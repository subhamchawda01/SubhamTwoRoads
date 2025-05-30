#!/usr/bin/perl

use strict;
use warnings;
use feature "switch"; # for given, when
use Fcntl qw (:flock);
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;
use sigtrap qw(handler signal_handler normal-signals error-signals);


my $HOME_DIR=$ENV{'HOME'};
my $USER=$ENV{'USER'};

my $REPO="basetrade";
my $SHARED_LOG_LOC="/media/shared/ephemeral16/indicatorLogs/";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$BIN_DIR;
my $hostname_ = `hostname`;

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/gen_ind_utils.pl"; #IsProjectedIndicator IsProjectedPairShc ShcHasExchSource
require "$GENPERLLIB_DIR/get_exch_from_shortcode.pl";        # for IsValidShc
require "$GENPERLLIB_DIR/sample_data_utils.pl"; # GetFeatureAverageDays
require "$GENPERLLIB_DIR/is_order_weighted_indicator.pl"; # IsOWIndicator
require "$GENPERLLIB_DIR/has_liffe_source.pl"; # HasLIFFESource
require "$GENPERLLIB_DIR/has_tse_source.pl"; # HasTSESource
require "$GENPERLLIB_DIR/has_espeed_source.pl"; # HasESPEEDSource
require "$GENPERLLIB_DIR/has_quincy_source.pl"; # HasQuincySource
require "$GENPERLLIB_DIR/has_rtsmicex_source.pl"; # HasRTSMICEXSource
require "$GENPERLLIB_DIR/skip_pca_indicator.pl"; # SkipPCAIndicator
require "$GENPERLLIB_DIR/skip_mixed_combo_indicators.pl"; # SkipMixedComboIndicator
require "$GENPERLLIB_DIR/skip_combo_book_indicator.pl"; # SkipComboBookIndicator
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList

my $IWORK_DIR=$HOME_DIR."/indicatorwork";
my $ITEMPLATES_DIR=$HOME_DIR."/modelling/indicatorwork";

my $CALENDAR_DAYS_FOR_DATAGEN=252;
my $MAX_INDICATORS_IN_FILE = 300;

my $is_lock_created_by_this_run_ = 0;
local $| = 1; # sets autoflush in STDOUT


my $USAGE = "USAGE: ProdConfig Ilist";
if ( $#ARGV < 1 ) 
{ 
    print $USAGE."\n";
    exit(0);
}

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $prodconfig_ = $ARGV[0];
my $ilist = $ARGV[1];
my @indicator_body_vec_ = () ;
my $self_shortcode_ = "-na-";

my $datagen_start_hhmm_ = "EST_815";
my $datagen_end_hhmm_ = "EST_1500";

my $dep_base_pricetype_ = "MktSizeWPrice";
my $dep_pred_pricetype_ = "MktSizeWPrice";

my $e_txt_ = " ";
my @source_vec_ = ();
my @noselfsource_vec_ = ();
my @sourcecombo_vec_ = ();
my @port_sourcecombo_vec_= ();
my @trade_indepsource_vec_ = ();
my @curve_indep_shc_vec_vec_ = (); #stores shc1 and sch2 for Curve indicators

my $read_all_template_files_ = 0;
my $use_core_files_ = 1;
my @custom_template_files_ = ( );
my @custom_ilist_files_ = ( );

my $config_contents = "";
LoadInstructionFile ( );

GenerateIndicatorBodyVec ( );






my $ilist_header_ = "MODELINIT DEPBASE $self_shortcode_ $dep_base_pricetype_ $dep_pred_pricetype_\n";
  $ilist_header_ .= "MODELMATH LINEAR CHANGE\nINDICATORSTART";
my @indicators_vec_ = map { "INDICATOR 1.00 ".$indicator_body_vec_[ $_ ] } 0..(scalar @indicator_body_vec_-1);
open ILIST,   ">", $ilist or PrintStacktraceAndDie( "Could not open $ilist\n" );
print ILIST $ilist_header_."\n".join("\n", @indicators_vec_)."\nINDICATOREND\n";
close ILIST;


sub ReadCoreTemplateFiles
{
  my $core_files_ref_ = shift;

  my $CORE_FOLDER = $HOME_DIR."/".$REPO."/IndicatorStats/core_lists";

  foreach my $tag_ ( qw( NOSELFSOURCE SELF SOURCE SOURCECOMBO ) ) {
    my $core_file_ = $CORE_FOLDER."/core_list_".$tag_;
    open COREFHANDLE, "< $core_file_" or PrintStacktraceAndDie ( "Could not open $core_file_ for reading" );
    my @lines_vec_ = <COREFHANDLE>; chomp( @lines_vec_ );
    @lines_vec_ = map { "indicator_list_".$_."_".$tag_ } @lines_vec_;
    push ( @$core_files_ref_, @lines_vec_ );
  }
}

sub GenerateIndicatorBodyVec
{
  my @files_vec_ = ( );

  if ( $read_all_template_files_ ) {
    @files_vec_ = `ls -1 $ITEMPLATES_DIR | grep \^indicator_list_`;
    chomp ( @files_vec_ );
  }
  else {
    ReadCoreTemplateFiles ( \@files_vec_ ) if $use_core_files_;
    push ( @files_vec_, @custom_template_files_ );
  }
  @files_vec_ = map { $ITEMPLATES_DIR."/".$_ } @files_vec_;
#print "Template Files:\n".join("\n", @files_vec_)."\n";

# SELF: Only Self based Indicators
# SOURCE: Indicators that take any Shortcode source, could be SELF
# NOSELFSOURCE: Any non-self Shortcode source
# SOURCECOMBO: Any Portfolio Source ( Combo and Port indicators )
# CURVE: only Curve based indicators
# TR: only indep TR indicators

  my @self_files_ = grep { /indicator_list_.*_SELF$/ } @files_vec_;
  my @source_files_ = grep { /indicator_list_.*_SOURCE$/ } @files_vec_;
  my @noselfsource_files_ = grep { /indicator_list_.*_NOSELFSOURCE$/ } @files_vec_;
  my @sourcecombo_files_ = grep { /indicator_list_.*_SOURCECOMBO$/ } @files_vec_;
  my @curve_files_ = grep { /indicator_list_.*_CURVE$/ } @files_vec_;
  my @indep_trade_files_ = grep { /TR/ } @files_vec_;

  my @full_indicator_body_vec_ = ();


  my @levels_options_ = (2,3,4,5,6,8,10,15,20,25,30,35,40,50);
  my @sizes_to_seek_options_ = (5,10,20,50,200,400,800,2000,4000);
  my ($avgl1sz_, undef) = GetFeatureAverageDays ( $self_shortcode_, $yyyymmdd_, 100, "L1SZ", [], $datagen_start_hhmm_, $datagen_end_hhmm_ );
  my ($avgstdev_, undef) = GetFeatureAverageDays ( $self_shortcode_, $yyyymmdd_, 100, "STDEV", [], $datagen_start_hhmm_, $datagen_end_hhmm_ );
  my ($avgvol_, undef) = GetFeatureAverageDays ( $self_shortcode_, $yyyymmdd_, 100, "VOL", [], $datagen_start_hhmm_, $datagen_end_hhmm_ );
  my $ticksize_ = `$BIN_DIR/get_min_price_increment $self_shortcode_ $yyyymmdd_ 2>/dev/null`; chomp ( $ticksize_ );
  my @num_levels_ = ( );
  my @num_levels_init_ = map { $_ * $avgstdev_ / $ticksize_ } (1,2,3);
  foreach my $num_level_ ( @num_levels_init_ ) {
    $num_level_ = (grep $_ >= $num_level_ , @levels_options_)[0];
    if ( defined $num_level_ && ! FindItemFromVec ( $num_level_, @num_levels_ ) ) { push ( @num_levels_, $num_level_ ); }
  }

  my @sizes_to_seek_ = ( );
  my @sizes_to_seek_init_ = map { $_ * $avgl1sz_ } (1,1.5);
  foreach my $tsize_ ( @sizes_to_seek_init_ ) {
    $tsize_ = (grep $_ >= $tsize_ , @sizes_to_seek_options_)[0];
    if ( defined $tsize_ && ! FindItemFromVec ( $tsize_, @sizes_to_seek_ ) ) { push ( @sizes_to_seek_, $tsize_ ); }
  }

  my @decay_factor_vec_ = (0.4,0.6,0.8);

# SELF indicators
  foreach my $this_self_indicator_filename_ ( @self_files_ ) {
    next if ( ! ExistsWithSize ( $this_self_indicator_filename_ ) ) ;

    open THIS_ILIST_FILEHANDLE, "< $this_self_indicator_filename_ " or PrintStacktraceAndDie ( "Could not open $this_self_indicator_filename_\n" );
    my @ilines_ = <THIS_ILIST_FILEHANDLE>; chomp ( @ilines_ );
    close THIS_ILIST_FILEHANDLE;

    foreach my $this_indicator_body_ ( @ilines_ ) {
      next if ( $this_indicator_body_ !~ m/INDICATOR 1.00 / );
      $this_indicator_body_ =~ s/INDICATOR 1.00\s+// ;

      my @ind_parts = split(/\s+/, $this_indicator_body_);
      next if ( IsOWIndicator($ind_parts[0]) && OrderInfoAbsent($self_shortcode_) );

      $this_indicator_body_ =~ s/\bSELF\b/$self_shortcode_/g;
      $this_indicator_body_ =~ s/\bBASEPRICETYPE\b/$dep_base_pricetype_/g;

      my @this_indicator_body_vec_ = ( $this_indicator_body_ );
      if ( $this_indicator_body_ =~ /\bLEVELS\b/ ) {
        @this_indicator_body_vec_ = ( );
        foreach my $num_level_ ( @num_levels_ ) {
           my $this_indicator_body_t_ = $this_indicator_body_;
           $this_indicator_body_t_ =~ s/\bLEVELS\b/$num_level_/g;
           push ( @this_indicator_body_vec_, $this_indicator_body_t_ );
        }
      }

      my @this_indicator_body_temp_vec_ = @this_indicator_body_vec_;
      @this_indicator_body_vec_ = ( );
      foreach my $this_indicator_body_ ( @this_indicator_body_temp_vec_ ) {
        if ( $this_indicator_body_ =~ /\bLSIZE\b/ ) {
          foreach my $tsize_ ( @sizes_to_seek_ ) {
            my $this_indicator_body_t_ = $this_indicator_body_;
            $this_indicator_body_t_ =~ s/\bLSIZE\b/$tsize_/g;
            push ( @this_indicator_body_vec_, $this_indicator_body_t_ );
          }
        }
        else {
          push ( @this_indicator_body_vec_, $this_indicator_body_ );
        }
      }

      @this_indicator_body_temp_vec_ = @this_indicator_body_vec_;
      @this_indicator_body_vec_ = ( );
      foreach my $this_indicator_body_ ( @this_indicator_body_temp_vec_ ) {
        if ( $this_indicator_body_ =~ /\bDECAY_FACTOR\b/ ) {
          foreach my $decay_factor_ ( @decay_factor_vec_ ) {
            my $this_indicator_body_t_ = $this_indicator_body_;
            $this_indicator_body_t_ =~ s/\bDECAY_FACTOR\b/$decay_factor_/g;
            push ( @this_indicator_body_vec_, $this_indicator_body_t_ );
          }
        }
        else {
          push ( @this_indicator_body_vec_, $this_indicator_body_ );
        }
      }

      push ( @full_indicator_body_vec_, @this_indicator_body_vec_ );
    }
  }

# SOURCE indicators
  foreach my $this_source_indicator_filename_ ( @source_files_ ) {
    next if ( ! ExistsWithSize ( $this_source_indicator_filename_ ) ) ;

    open THIS_ILIST_FILEHANDLE, "< $this_source_indicator_filename_ " or PrintStacktraceAndDie ( "Could not open $this_source_indicator_filename_\n" );
    my @ilines_ = <THIS_ILIST_FILEHANDLE>; chomp ( @ilines_ );
    close THIS_ILIST_FILEHANDLE;

    foreach my $this_indicator_body_ ( @ilines_ ) {
      next if ( $this_indicator_body_ !~ m/INDICATOR 1.00 / );
      $this_indicator_body_ =~ s/INDICATOR 1.00\s+// ;

      my @ind_parts = split(/\s+/, $this_indicator_body_);
      my $is_ow_ind = IsOWIndicator($ind_parts[0]);
      my $is_projected_indicator_ = IsProjectedIndicator($this_indicator_body_);

      $this_indicator_body_ =~ s/\bSELF\b/$self_shortcode_/g;
      $this_indicator_body_ =~ s/\bBASEPRICETYPE\b/$dep_base_pricetype_/g;

      foreach my $this_source_shortcode_ ( @source_vec_ ) {
        next if ( IsOWIndicator($ind_parts[0]) && OrderInfoAbsent($this_source_shortcode_) );
        next if ( ($is_projected_indicator_) && (!IsProjectedPairShc($self_shortcode_, $this_source_shortcode_)) );

        my $t_this_indicator_body_ = $this_indicator_body_;
        $t_this_indicator_body_ =~ s/\bSOURCE\b/$this_source_shortcode_/g;

        push ( @full_indicator_body_vec_, $t_this_indicator_body_ );
      }
    }
  }

# NOSELFSOURCE indicators
  foreach my $this_source_indicator_filename_ ( @noselfsource_files_ ) {
    next if ( ! ExistsWithSize ( $this_source_indicator_filename_ ) ) ;

    open THIS_ILIST_FILEHANDLE, "< $this_source_indicator_filename_ " or PrintStacktraceAndDie ( "Could not open $this_source_indicator_filename_\n" );
    my @ilines_ = <THIS_ILIST_FILEHANDLE>; chomp ( @ilines_ );
    close THIS_ILIST_FILEHANDLE;

    foreach my $this_indicator_body_ ( @ilines_ ) {
      next if ( $this_indicator_body_ !~ m/INDICATOR 1.00 / );
      $this_indicator_body_ =~ s/INDICATOR 1.00\s+// ;

      my @ind_parts = split(/\s+/, $this_indicator_body_);
      my $is_ow_ind = IsOWIndicator($ind_parts[0]);
      my $is_projected_indicator_ = IsProjectedIndicator($this_indicator_body_);

      $this_indicator_body_ =~ s/\bSELF\b/$self_shortcode_/g;
      $this_indicator_body_ =~ s/\bBASEPRICETYPE\b/$dep_base_pricetype_/g;

      foreach my $this_source_shortcode_ ( @noselfsource_vec_ ) {
        next if ( IsOWIndicator($ind_parts[0]) && OrderInfoAbsent($this_source_shortcode_) );
        next if ( ($is_projected_indicator_) && (!IsProjectedPairShc($self_shortcode_, $this_source_shortcode_)) );

        my $t_this_indicator_body_ = $this_indicator_body_;
        $t_this_indicator_body_ =~ s/\bNOSELFSOURCE\b/$this_source_shortcode_/g;

        push ( @full_indicator_body_vec_, $t_this_indicator_body_ );
      }
    }
  }

# SOURCECOMBO indicators
  foreach my $this_sourcecombo_indicator_filename_ ( @sourcecombo_files_ ) {
    next if ( ! ExistsWithSize ( $this_sourcecombo_indicator_filename_ ) ) ;

    open THIS_ILIST_FILEHANDLE, "< $this_sourcecombo_indicator_filename_ " or PrintStacktraceAndDie ( "Could not open $this_sourcecombo_indicator_filename_\n" );
    my @ilines_ = <THIS_ILIST_FILEHANDLE>; chomp ( @ilines_ );
    close THIS_ILIST_FILEHANDLE;

    foreach my $this_indicator_body_ ( @ilines_ ) {
      next if ( $this_indicator_body_ !~ m/INDICATOR 1.00 / );
      $this_indicator_body_ =~ s/INDICATOR 1.00\s+// ;

      my @ind_parts = split(/\s+/, $this_indicator_body_);
      my $is_ow_ind = IsOWIndicator($ind_parts[0]);
      my $is_projected_indicator_ = IsProjectedIndicator($this_indicator_body_);

      $this_indicator_body_ =~ s/\bSELF\b/$self_shortcode_/g;
      $this_indicator_body_ =~ s/\bBASEPRICETYPE\b/$dep_base_pricetype_/g;

      foreach my $this_sourcecombo_shortcode_ ( @sourcecombo_vec_ )
      {
# Check if shc is !win && !ind && ind ~ mult.*combo then skip
# Skip PCA Indicators if dep is not in portfolio
# Skip Combo indicators if the portfolio has products with mixed correlations (such as ZN_0 and ES_0 ) except the Offline ones
          next if ( ( SkipComboBookIndicator ( $self_shortcode_, $this_sourcecombo_shortcode_ , $this_sourcecombo_indicator_filename_ ) ) ||
                   ( SkipPCAIndicator ( $self_shortcode_, $this_sourcecombo_shortcode_, $this_sourcecombo_indicator_filename_ ) ) ||
                   ( SkipMixedComboIndicator ( $self_shortcode_, $this_sourcecombo_shortcode_, $this_sourcecombo_indicator_filename_ ) ) ||
                   ( SkipVolPCAIndicator ( $self_shortcode_, $this_sourcecombo_shortcode_, $this_sourcecombo_indicator_filename_ ) )
                  ) ;

        next if ( IsOWIndicator($ind_parts[0]) && OrderInfoAbsent($this_sourcecombo_shortcode_) );
        next if ( ($is_projected_indicator_) && (!IsProjectedPairShc($self_shortcode_, $this_sourcecombo_shortcode_)) );

        my $t_this_indicator_body_ = $this_indicator_body_;
        $t_this_indicator_body_ =~ s/\bSOURCECOMBO\b/$this_sourcecombo_shortcode_/g;

        push ( @full_indicator_body_vec_, $t_this_indicator_body_ );
      }

      if ( $this_indicator_body_ =~ m/Port\b/ ) {
        foreach my $this_sourcecombo_shortcode_ ( @sourcecombo_vec_ )
        {
          next if ( ( SkipPCAIndicator ( $self_shortcode_, $this_sourcecombo_shortcode_, $this_sourcecombo_indicator_filename_ ) ) ||
              ( SkipVolPCAIndicator ( $self_shortcode_, $this_sourcecombo_shortcode_, $this_sourcecombo_indicator_filename_ ) )
              ) ;

          next if ( IsOWIndicator($ind_parts[0]) && OrderInfoAbsent($this_sourcecombo_shortcode_) );
          next if ( ($is_projected_indicator_) && (!IsProjectedPairShc($self_shortcode_, $this_sourcecombo_shortcode_)) );

          my $t_this_indicator_body_ = $this_indicator_body_;
          $t_this_indicator_body_ =~ s/\bSOURCECOMBO\b/$this_sourcecombo_shortcode_/g;

          push ( @full_indicator_body_vec_, $t_this_indicator_body_ );
        }
      }
    }
  }

#CURVE indicators
  foreach my $this_curve_indicator_filename_ ( @curve_files_ ) {
    next if ( ! ExistsWithSize ( $this_curve_indicator_filename_ ) ) ;

    open THIS_ILIST_FILEHANDLE, "< $this_curve_indicator_filename_ " or PrintStacktraceAndDie ( "Could not open $this_curve_indicator_filename_\n" );
    my @ilines_ = <THIS_ILIST_FILEHANDLE>; chomp ( @ilines_ );
    close THIS_ILIST_FILEHANDLE;

    foreach my $this_indicator_body_ ( @ilines_ ) {
      next if ( $this_indicator_body_ !~ m/INDICATOR 1.00 / );
      $this_indicator_body_ =~ s/INDICATOR 1.00\s+// ;

      $this_indicator_body_ =~ s/\bSELF\b/$self_shortcode_/g;
      $this_indicator_body_ =~ s/\bBASEPRICETYPE\b/$dep_base_pricetype_/g;

      foreach my $indeps_ ( @curve_indep_shc_vec_vec_ ) {
        my $t_this_indicator_body_ = $this_indicator_body_;
        $t_this_indicator_body_ =~ s/\bINDEP1\b/$$indeps_[0]/g;
        $t_this_indicator_body_ =~ s/\bINDEP2\b/$$indeps_[1]/g;
        push ( @full_indicator_body_vec_, $t_this_indicator_body_ );
      }
    }
  }

#INDEP_TRADE indicators
  foreach my $this_trade_indicator_filename_ ( @indep_trade_files_ ) {
    next if ( ! ExistsWithSize ( $this_trade_indicator_filename_ ) ) ;

    open THIS_ILIST_FILEHANDLE, "< $this_trade_indicator_filename_ " or PrintStacktraceAndDie ( "Could not open $this_trade_indicator_filename_\n" );
    my @ilines_ = <THIS_ILIST_FILEHANDLE>; chomp ( @ilines_ );
    close THIS_ILIST_FILEHANDLE;

    foreach my $this_indicator_body_ ( @ilines_ ) {
      next if ( $this_indicator_body_ !~ m/INDICATOR 1.00 / );
      $this_indicator_body_ =~ s/INDICATOR 1.00\s+// ;

      $this_indicator_body_ =~ s/\bBASEPRICETYPE\b/$dep_base_pricetype_/g;

      foreach my $this_source_shortcode_ ( @trade_indepsource_vec_ ) {
        my $t_this_indicator_body_ = $this_indicator_body_;
        $t_this_indicator_body_ =~ s/\bSELF\b/$this_source_shortcode_/g;
        push ( @full_indicator_body_vec_, $t_this_indicator_body_ );
      }
    }
  }

# USER provided custom ilist files
  foreach my $this_ilist_file_ ( @custom_ilist_files_ ) {
    next if ( ! ExistsWithSize ( $this_ilist_file_ ) );

    open THIS_ILIST_FILEHANDLE, "< $this_ilist_file_ " or PrintStacktraceAndDie ( "Could not open $this_ilist_file_\n" );
    my @ilines_ = <THIS_ILIST_FILEHANDLE>; chomp ( @ilines_ );
    close THIS_ILIST_FILEHANDLE;

    foreach my $this_indicator_body_ ( @ilines_ ) {
      next if ( $this_indicator_body_ !~ m/INDICATOR 1.00 / );
      $this_indicator_body_ =~ s/INDICATOR 1.00\s+// ;
      push ( @full_indicator_body_vec_, $this_indicator_body_ );
    }
  }

  # replace multiple whitespaces in each indicator with single space
  map { s/\s+/ /g } @full_indicator_body_vec_;

  @indicator_body_vec_ = GetUniqueList ( @full_indicator_body_vec_ );
}

sub LoadInstructionFile
{
  my $end_date_ = GetIsoDateFromStrMin1 ( "TODAY-1" );
  my $numdays_ = $CALENDAR_DAYS_FOR_DATAGEN;

  open PRODCONFIGHANDLE, "< $prodconfig_ " or PrintStacktraceAndDie ( "Could not open $prodconfig_\n" );
  while ( my $thisline_ = <PRODCONFIGHANDLE> )
  {
      chomp ( $thisline_ );
      $config_contents = $config_contents.$thisline_."\n";
      my $comment_idx_ = index( $thisline_, '#' );
      $thisline_ = substr($thisline_, 0, $comment_idx_) if $comment_idx_ >= 0;

      my @this_words_ = split ( /\s+/, $thisline_ );

      if ($#this_words_ >= 1)
      {
          given ($this_words_[0])
          {
              when("DATAGEN_BASE_FUT_PAIR")
              {
                  if ($#this_words_ < 2)
                  {
                      PrintStacktraceAndDie ( "DATAGEN_BASE_FUT_PAIR line should be like DATAGEN_BASE_FUT_PAIR MktSizeWPrice MktSizeWPrice" );
                  }
                  $dep_base_pricetype_ = $this_words_[1];
                  $dep_pred_pricetype_ = $this_words_[2];
              }
              when ("SELF")
              {
                  $self_shortcode_ = $this_words_[1];

                  if (!IsValidShc($self_shortcode_ ))
                  {
                      PrintStacktraceAndDie ( "SELF Not valid shortcode: ".$self_shortcode_ );
                  }
                  if (IsStdevPresentForShc($self_shortcode_) == 0)
                  {
                      PrintStacktraceAndDie ( "STDEV for SELF shortcode ".$self_shortcode_." does not exist.\nAdd stdev to /spare/local/tradeinfo/PCAInfo/shortcode_stdev_DEFAULT.txt" );
                  }
              }
              when ("SOURCE")
              {
                  foreach my $source_ (@this_words_)
                  {
                      next if FindItemFromVec ( $source_, @source_vec_ );

                      if (!IsValidShc($source_))
                      {

                          $e_txt_ = $e_txt_."Not valid shortcode: ".$source_."\n";
                          next;
                      }
                      if (IsStdevPresentForShc( $source_ ) == 0)
                      {
                          $e_txt_ = $e_txt_."Stdev for source_shortcode_: $source_ does not exist.\nAdd stdev to /spare/local/tradeinfo/PCAInfo/shortcode_stdev_DEFAULT.txt\n";
                          next;
                      }
                      push ( @source_vec_, $source_ );

                      next if $source_ eq $self_shortcode_;
                      if (!IsLRDBPairPresent ( $self_shortcode_, $source_ ))
                      {
                          $e_txt_ = $e_txt_."LRDB value absent for: ".$self_shortcode_.", ".$source_."\n";
                          next;
                      }
                      push ( @noselfsource_vec_, $source_ );
                  }
              }
              when ("NOSELFSOURCE")
              {
                  foreach my $source_ (@this_words_)
                  {
                      next if FindItemFromVec ( $source_, @source_vec_ );
                      next if $source_ eq $self_shortcode_;

                      if (!IsValidShc($source_))
                      {
                          $e_txt_ = $e_txt_."Not valid shortcode: ".$source_."\n";
                          next;
                      }
                      if (!IsLRDBPairPresent ( $self_shortcode_, $source_ ))
                      {
                          $e_txt_ = $e_txt_."LRDB value absent for: ".$self_shortcode_.", ".$source_."\n";
                          next;
                      }
                      if (IsStdevPresentForShc( $source_ ) == 0)
                      {
                          $e_txt_ = $e_txt_."Stdev for source_shortcode_: $source_ does not exist.\nAdd stdev to /spare/local/tradeinfo/PCAInfo/shortcode_stdev_DEFAULT.txt\n";
                          next;
                      }

                      push ( @noselfsource_vec_, $source_ );
                  }
              }
              when ("SOURCECOMBO")
              {
                  foreach my $port_ (@this_words_)
                  {
                      if (!IsValidPort($port_))
                      {
                          $e_txt_ = $e_txt_."Not valid shortcode: ".$port_."\n";
                          next;
                      }
                      my $lrdb_present_ = 1;

                      foreach my $constituent_shc_ (GetPortConstituents( $port_ )) {
                          next if $constituent_shc_ eq $self_shortcode_;

                          if (!IsLRDBPairPresent ( $self_shortcode_, $constituent_shc_ ))
                          {
                              $e_txt_ = $e_txt_."LRDB value absent for: ".$self_shortcode_.", ".$constituent_shc_."\n";
                              $lrdb_present_ = 0;
                              last;
                          }
                      }
                      next if ( !$lrdb_present_ );

                      if (IsStdevPresentForShc( $port_) == 0)
                      {
                          $e_txt_ = $e_txt_."Stdev for sourcecombo_shortcode_: $port_ does not exist.\nAdd stdev to /spare/local/tradeinfo/PCAInfo/shortcode_stdev_DEFAULT.txt\n";
                          next;
                      }
                      push ( @sourcecombo_vec_, $port_ );
                  }
              }
              when ("PORT")
              {
                  foreach my $port_ (@this_words_) 
                  {
                      if (!IsValidPort($port_))
                      {
                          $e_txt_ = $e_txt_."Not valid shortcode: ".$port_."\n";
                          next;
                      }
                      if (!IsLRDBPairPresent ( $self_shortcode_, $port_ ))
                      {
                          $e_txt_ = $e_txt_."LRDB value absent for: ".$self_shortcode_.", ".$port_."\n";
                          next;
                      }
                      if (IsStdevPresentForShc($port_) == 0)
                      {
                          $e_txt_ = $e_txt_."Stdev for port_shortcode_: $port_ does not exist.\nAdd stdev to /spare/local/tradeinfo/PCAInfo/shortcode_stdev_DEFAULT.txt\n";
                          next;
                      }
                      push ( @port_sourcecombo_vec_, $port_ );
                  }
              }
              when ("TRADEINDEP")
              {
                  foreach my $source_ (@this_words_)
                  {
                      if (!IsValidShc($source_))
                      {
                          $e_txt_ = $e_txt_."Not valid shortcode: ".$source_."\n";
                          next;
                      }
                      push ( @trade_indepsource_vec_, $source_ );
                  }
              }
          }
      }
  } }
