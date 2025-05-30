#!/usr/bin/perl

use strict;
use warnings;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME = "/spare/local/".$USER."/";
my $REPO = "basetrade";

my $USAGE = "$0 <event_datfile> <config_file> <output_configfile> <maxpxch_percentile> <thresh_percentile>=0 [<datum1_id>,<datum2_id>..] [<scale_factor=1>]";

if ( $#ARGV < 5 ) {
  print $USAGE."\n";
  exit(0);
}

my $datfile_ = $ARGV[0];
my $config_file_ = $ARGV[1];
my $out_file_ = $ARGV[2];
my $maxpxch_percentile_ = $ARGV[3];
my $thresh_percentile_ = $ARGV[4];
my @ev_ids_ = split(",", $ARGV[5]);

my $scale_fact_ = 1;
$scale_fact_ = $ARGV[6] if $#ARGV > 5;

my $event_id_;

LoadConfigFile ( );

sub PrintForShortcode {
  my ($shc_, $betamins_, $old_maxpxch_, $old_thresh_, $old_getflat_, $old_betastr_) = @_;
  my ($maxpxch_, $thresh_, $getflat_, $betastr_) = (undef,undef,undef,undef);

  if ( $betamins_ !~ /^(2|5|10|15)$/ ) {
    print "WARN: MINUTES_TO_RUN for $shc_ not 2,5,10 or 15\n";
    return ($old_maxpxch_, $old_thresh_, $old_getflat_, $betastr_);
  }
  $betamins_ = 5 if $betamins_ == 15;

  my $findScale_cmd_ = "$HOME_DIR/$REPO/AflashScripts/findScale.R $datfile_ $shc_ $shc_/pxchange.dat $thresh_percentile_,$maxpxch_percentile_";
  my @scale_out_ = `$findScale_cmd_ 2>/dev/null`; chomp(@scale_out_);

  my ($beta_str_) = grep { $_ =~ /^beta$betamins_/ } @scale_out_;
  if ( defined $beta_str_ && $beta_str_ ne "" ) {
    my @beta_wds_ = split(/\s+/, $beta_str_);
    @beta_wds_ = @beta_wds_[1..($#beta_wds_-1)];
    if ( $#beta_wds_ != $#ev_ids_ ) {
      print "WARN: no. of beta coeffs different from no. of datum_ids\n";
      return ($old_maxpxch_, $old_thresh_, $old_getflat_, $betastr_);
    }
    $betastr_ = join(",", map { $ev_ids_[$_].":".($beta_wds_[$_]/$scale_fact_) } 0..$#beta_wds_ );
  }

  my $mins_idx_ = ($betamins_ == 2) ? 1 : ( ($betamins_ == 5) ? 2 : 3 );

  my ($perc_str_) = grep { $_ =~ /^percentile$maxpxch_percentile_/ } @scale_out_;
  if ( $maxpxch_percentile_ > 0 && defined $perc_str_ && $perc_str_ ne "" ) {
    my @perc_wds_ = split(/\s+/, $perc_str_);
    $maxpxch_ = $perc_wds_[$mins_idx_];
  }

  ($perc_str_) = grep { $_ =~ /^percentile$thresh_percentile_/ } @scale_out_;
  if ( $thresh_percentile_ > 0 && defined $perc_str_ && $perc_str_ ne "" ) {
    my @perc_wds_ = split(/\s+/, $perc_str_);
    $thresh_ = $perc_wds_[$mins_idx_];
  }

  if ( defined $old_getflat_ ) {
    $getflat_ = $old_getflat_ * $maxpxch_ / $old_maxpxch_;
  }

  return ($maxpxch_, $thresh_, $getflat_, $betastr_);
}

sub LoadConfigFile
{
  print "Loading Config File ...\n";

  open ( CONFIG_FILE , "<" , $config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $config_file_" );
  my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
  close ( CONFIG_FILE );

  open OUTFILE, "> $out_file_" or PrintStacktraceAndDie ( "Could not open output file $out_file_" );

  my $current_shc_ = "";
  my ($min_to_run_, $old_maxpxch_, $old_thresh_, $old_getflat_, $old_betastr_) = (undef, undef, undef, undef, undef);

  foreach my $cline_ ( @config_file_lines_ )
  {
    next if ( $cline_ =~ /^#/ );  # not ignoring lines with # not at the beginning
    my @t_words_ = split ( ' ' , $cline_ );

    if ( $#t_words_ < 0 ) {
      if ( $current_shc_ ne "" ) {
        my ($maxpxch_, $thresh_, $getflat_, $betastr_) = PrintForShortcode ($current_shc_, $min_to_run_, $old_maxpxch_, $old_thresh_, $old_getflat_, $old_betastr_);
        print OUTFILE "AF_SCALE_BETA $betastr_\n";
        print OUTFILE "PXCH_FOR_MAXORDERSIZE $maxpxch_\n";
        print OUTFILE "AGGRESSIVE_THRESHOLD $thresh_\n" if defined $thresh_;
        print OUTFILE "GETFLAT_MARGIN $getflat_\n" if defined $getflat_;
      }
      print OUTFILE "\n";
      $current_shc_ = "";
      next;
    }
    
    my $param_ = $t_words_[0];

    if ( $current_shc_ eq "" ) {
      print OUTFILE "$cline_\n";
      if ( $param_ eq "SHORTCODE" ) {
        $current_shc_ = $t_words_[1];
        ($min_to_run_, $old_maxpxch_, $old_thresh_, $old_getflat_, $old_betastr_) = (undef, undef, undef, undef, undef);
      }
      elsif ( $param_ eq "EVENT_ID" ) {
        $event_id_ = $t_words_[1];
      }
    }
    else {
      if ( $param_ =~ /^(UTS|MUR|MAXLOSS|DRAWDOWN_SECS)$/ ) {
        print OUTFILE "$cline_\n";
      }
      elsif ( $param_ eq "PXCH_FOR_MAXORDERSIZE" ) {
        $old_maxpxch_ = $t_words_[1];
      }
      elsif ( $param_ eq "AGGRESSIVE_THRESHOLD" ) {
        $old_thresh_ = $t_words_[1];
      }
      elsif ( $param_ eq "GETFLAT_MARGIN" ) {
        $old_getflat_ = $t_words_[1];
      }
      elsif ( $param_ eq "AF_SCALE_BETA" ) {
        $old_betastr_ = $t_words_[1];
      }
      elsif ( $param_ eq "MINUTES_TO_RUN" ) {
        $min_to_run_ = $t_words_[1];
        print OUTFILE "$cline_\n";
      }
      elsif ( $param_ eq "SHORTCODE" ) {
        PrintStacktraceAndDie ( "SHORTCODE should always be First Line of a Section" );
      }
    }
  }
  if ( $current_shc_ ne "" ) {
    my ($maxpxch_, $thresh_, $getflat_, $betastr_) = PrintForShortcode ($current_shc_, $min_to_run_, $old_maxpxch_, $old_thresh_, $old_getflat_, $old_betastr_);
    print OUTFILE "AF_SCALE_BETA $betastr_\n";
    print OUTFILE "PXCH_FOR_MAXORDERSIZE $maxpxch_\n";
    print OUTFILE "AGGRESSIVE_THRESHOLD $thresh_\n" if defined $thresh_;
    print OUTFILE "GETFLAT_MARGIN $getflat_\n" if defined $getflat_;
  }

  close (OUTFILE);
}

