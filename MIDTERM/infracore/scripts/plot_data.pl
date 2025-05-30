#!/usr/bin/perl
use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

my $USAGE="$0 SHORTCODE DATE STARTTIME ENDTIME\n";
if ( $#ARGV < 1 ) 
{ 
    print $USAGE; 
    exit ( 0 ); 
}

{ # Generate the data.
    my $data_script_ = $HOME_DIR."/infracore/scripts/process_data.pl";

    my $args_ = join ( ' ', @ARGV );
    `$data_script_ $args_`;
}

my $shortcode_ = $ARGV[0];
my $yyyymmdd_ = $ARGV[1];

my $plot_script_ = $HOME_DIR."/infracore/scripts/plot_multifile_cols.pl";

my $t_dir_ = $HOME_DIR."/plots";
`mkdir -p $t_dir_`;

{ # MDS data plots.
    my $l1_file_name_ = $t_dir_."/".$shortcode_."_".$yyyymmdd_."_l1";
    my $trades_file_name_ = $t_dir_."/".$shortcode_."_".$yyyymmdd_."_trades";

    { # (i) bid_px, ask_px
    	my $t_bid_tag_ = $shortcode_."bid_px";
    	my $t_ask_tag_ = $shortcode_."ask_px";

#    	`$plot_script_ $l1_file_name_ 4 $t_bid_tag_ WL $l1_file_name_ 5 $t_ask_tag_ WL`;
    }

    { # (ii) bid_sz, ask_sz
    	my $t_bid_tag_ = $shortcode_."bid_sz";
    	my $t_ask_tag_ = $shortcode_."ask_sz";

    	`$plot_script_ $l1_file_name_ 2 $t_bid_tag_ NL $l1_file_name_ 7 $t_ask_tag_ NL`;
    }

    { # (iii) avg_bid_ord_sz, avg_ask_ord_sz
    	my $t_bid_tag_ = $shortcode_."bid_avg_ord_sz";
    	my $t_ask_tag_ = $shortcode_."ask_avg_ord_sz";

#    	`$plot_script_ $l1_file_name_ 8 $t_bid_tag_ NL $l1_file_name_ 9 $t_ask_tag_ NL`;
    }

    { # (iv) trade_sz
	my $t_trd_tag_ = $shortcode_."trade_sz";

#	`$plot_script_ $trades_file_name_ 2 $t_trd_tag_ NL`;
    }

    { # (v) trade_px
	my $t_trd_tag_ = $shortcode_."trade_px";

	`$plot_script_ $trades_file_name_ 3 $t_trd_tag_ WL`;
    }

    `rm -f $l1_file_name_ $trades_file_name_`;
}

{ # ORS data plots.
    my $ors_file_name_ = $t_dir_."/".$shortcode_."_".$yyyymmdd_."_ors";

    { # seqd_to_conf
	my $t_ors_tag_ = $shortcode_."seqd_to_conf";

	`$plot_script_ $ors_file_name_ 2 $t_ors_tag_ NL`;

	`rm -f $ors_file_name_`;
    }
}
