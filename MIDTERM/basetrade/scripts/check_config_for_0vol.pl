#!/usr/bin/perl
no if ($] >= 5.018), 'warnings' => 'experimental';

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

sub CheckShortcodeForFailedZero
{
  my $shortcode_ = shift;

  my $skip_shc_file_ = "$HOME_DIR/basetrade/scripts/zerovol_skip_shc_list.txt";
  if ( ! -f $skip_shc_file_ ) { print "Error: $skip_shc_file_ NOT present" }
  my @skip_shortcodes_ = `cat $skip_shc_file_ 2>/dev/null`;
  chomp ( @skip_shortcodes_ );
  my @skip_prefixes_ = grep { $_ =~ /\*$/ } @skip_shortcodes_;
  @skip_prefixes_ = map {$_ =~ s/\*$//g; $_} @skip_prefixes_;

  @skip_shortcodes_ = grep { $_ !~ /\*$/ } @skip_shortcodes_;

  if ( $is_ebt_ || $shortcode_ ~~ @skip_shortcodes_ || grep {$shortcode_ =~ /^$_/} @skip_prefixes_ ) {
    return 0;
  }
  return 1;
}

sub CheckConfigForFailedZero
{
  my $config_ = shift;
  my $ebttok_ = `$WF_SCRIPTS_DIR/process_config.py -c $config_ -m VIEW 2>/dev/null | grep -m1 '^EVENT_TOKEN:' | cut -d' ' -f2-`;
  chomp ( $ebttok_ );
  my $is_ebt_ = (defined $ebttok_ and $ebttok_ eq "EBT") ? 1 : 0;

  my $shortcode_ = `$WF_SCRIPTS_DIR/process_config.py -c $config_ -m  2>/dev/null | grep -m1 '^SHORTCODE:' | cut -d' ' -f2-`;
  chomp ( $shortcode_ );

  return (! $is_ebt_) && CheckShortcodeForFailedZero( $shortcode_ );
}

if ( $#ARGV < 1 )
{
  print "USAGE: [C/S (C:config, S:shortcode)] Shortcode/Configname\n";
  exit 0;
}

my $config_or_shc_ = $ARGV[0];
if ( $config_or_shc_ eq 'C' ) {
  print CheckConfigForFailedZero($ARGV[1])."\n";
}
elsif ( $config_or_shc_ eq 'S' ) {
  print CheckShortcodeForFailedZero($ARGV[1])."\n";
}

