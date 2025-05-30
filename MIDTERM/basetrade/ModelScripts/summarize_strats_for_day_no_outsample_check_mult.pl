#!/usr/bin/perl

my $USAGE="$0 shortcode timeperiod NUM_PAST_DAYS ... ";
if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $timeperiod_ = $ARGV[1];

my @num_past_days_vec_ = ();

for ( my $ai_ = 2; $ai_ <= $#ARGV; $ai_ ++ )
{
    push ( @num_past_days_vec_, $ARGV[$ai_] );
}

for ( my $ai_ = 0 ; $ai_ <= $#num_past_days_vec_ ; $ai_ ++ )
{
    my $NUM_PAST_DAYS=$num_past_days_vec_[$ai_];
    print "NPD: $NUM_PAST_DAYS\n";
    system ( "~/basetrade/ModelScripts/summarize_strats_for_day_no_outsample_check.pl $shortcode_ $timeperiod_ TODAY-1 $NUM_PAST_DAYS | grep -v MAXDD | awk \'{ if ( NF > 15 ) { printf \"\%d \%d \%.2f \%s $NUM_PAST_DAYS\\n\", \$3, \$5, (\$3 / \$16), \$2 } }' | head -n10" );
}
