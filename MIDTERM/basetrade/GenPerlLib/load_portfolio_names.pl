# \file GenPerlLib/load_portfolio_names.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#

use feature "switch"; # for given, when
use File::Basename;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BASETRADEINFODIR="/spare/local/tradeinfo/";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# This function is very fragile. 
# It takes information of portfolios from /spare/local/tradeinfo/PortfolioInfo/portfolio_constituent_info_default.txt
sub LoadPortfolioNames
{
    my @portfolio_shortcodes_ = ();

    my $pca_portfolio_constituent_filename_ = $BASETRADEINFODIR."PCAInfo/portfolio_inputs";
    open PCA_PORT_CONST_FILE, "< $pca_portfolio_constituent_filename_ " or PrintStacktraceAndDie ( "Could not open $pca_portfolio_constituent_filename_\n" );

    while ( my $pcf_line_ = <PCA_PORT_CONST_FILE> )
    { # PLINE PORT_SC CON_1_SC ...
	my @pcf_words_ = split ( ' ', $pcf_line_ );
	if ( $#pcf_words_ >= 1 )
	{
	    push ( @portfolio_shortcodes_, $pcf_words_[1] );
	}
    }

    # my $portfolio_contituent_filename_ = $BASETRADEINFODIR."PortfolioInfo/portfolio_constituent_info_default.txt";
    
    # open PORT_CONST_FILE, "< $portfolio_contituent_filename_ " or PrintStacktraceAndDie ( "Could not open $portfolio_contituent_filename_\n" );

    # while ( my $pcf_line_ = <PORT_CONST_FILE> )
    # { # PORT_SC CON_1_SC ...
    # 	my @pcf_words_ = split ( ' ', $pcf_line_ );
    # 	if ( $#pcf_words_ >= 1 )
    # 	{
    # 	    push ( @portfolio_shortcodes_, $pcf_words_[0] );
    # 	}
    # }

    @portfolio_shortcodes_;
}

1;
