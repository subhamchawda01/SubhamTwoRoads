# \file GenPerlLib/has_liffe_source.pl
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

sub HasLIFFESource
{
    my $shortcode_ = shift;
    my $retval = 0;

    given ( $shortcode_ )
    {
	when ( "EIBORFLY1" ) { $retval = 1; }
	when ( "EIBORFLY2" ) { $retval = 1; }
	when ( "EIBORFLY3" ) { $retval = 1; }
	when ( "EIBORFLY4" ) { $retval = 1; }
	when ( "EIBORFLY5" ) { $retval = 1; }
	when ( "EIBORFLY6" ) { $retval = 1; }
	when ( "EIBORFLY7" ) { $retval = 1; }
	when ( "STERFLY1" ) { $retval = 1; }
	when ( "STERFLY2" ) { $retval = 1; }
	when ( "STERFLY3" ) { $retval = 1; }
	when ( "STERFLY4" ) { $retval = 1; }
	when ( "STERFLY5" ) { $retval = 1; }
	when ( "STERFLY6" ) { $retval = 1; }
	when ( "EIBORFGBS" ) { $retval = 1; }
	when ( "EIBORGRN" ) { $retval = 1; }
	when ( "EIBORMID" ) { $retval = 1; }
	when ( "EIBORRED" ) { $retval = 1; }
	when ( "EIBORWHT" ) { $retval = 1; }
	when ( "EIBOR2Y" ) { $retval = 1; }
	when ( "ELBINDX" ) { $retval = 1; }
	when ( "ELEQUI" ) { $retval = 1; }
	when ( "ELFRNCH" ) { $retval = 1; }
	when ( "ELUEQUI" ) { $retval = 1; }
	when ( "STERGRN" ) { $retval = 1; }
	when ( "STERMID" ) { $retval = 1; }
	when ( "STERRED" ) { $retval = 1; }
	when ( "STERWHT" ) { $retval = 1; }
	when ( "UELBOND" ) { $retval = 1; }
	when ( "UELINDEBND" ) { $retval = 1; }
	when ( "ULEQUI" ) { $retval = 1; }
	when ( "ULWHEAT" ) { $retval = 1; }
	when ( "ULWHEATL" ) { $retval = 1; }
	when ( "LWHEATALL" ) { $retval = 1; }
	when ( "LCOCOWHEAT" ) { $retval = 1; }
	when ( "LCOCO" ) { $retval = 1; }
	when ( "LCOFFCOCOL" ) { $retval = 1; }
	when ( "LCOFFCOCO" ) { $retval = 1; }
	when ( "LCOFFWHEATL" ) { $retval = 1; }
	when ( "LCOFFWHEAT" ) { $retval = 1; }
	when ( "LCOFF" ) { $retval = 1; }
	when ( "LWHEATCOCOCOFF" ) { $retval = 1; }
	when ( "FGBLLFR" ) { $retval = 1; }
	when ( "JFFCE_0" ) { $retval = 1; }
	when ( "JFFCE_1" ) { $retval = 1; }
	when ( "KFFTI_0" ) { $retval = 1; }
	when ( "LFZ_0" ) { $retval = 1; }
	when ( "LFZ_1" ) { $retval = 1; }
	when ( "LFL_0" ) { $retval = 1; }
	when ( "LFL_1" ) { $retval = 1; }
	when ( "LFL_2" ) { $retval = 1; }
	when ( "LFL_3" ) { $retval = 1; }
	when ( "LFL_4" ) { $retval = 1; }
	when ( "LFL_5" ) { $retval = 1; }
	when ( "LFL_6" ) { $retval = 1; }
	when ( "LFL_7" ) { $retval = 1; }
	when ( "LFL_8" ) { $retval = 1; }
	when ( "LFL_9" ) { $retval = 1; }
	when ( "LFL_10" ) { $retval = 1; }
	when ( "LFL_11" ) { $retval = 1; }
	when ( "LFL_12" ) { $retval = 1; }
	when ( "LFR_0" ) { $retval = 1; }
	when ( "LFI_0" ) { $retval = 1; }
	when ( "LFI_1" ) { $retval = 1; }
	when ( "LFI_2" ) { $retval = 1; }
	when ( "LFI_3" ) { $retval = 1; }
	when ( "LFI_4" ) { $retval = 1; }
	when ( "LFI_5" ) { $retval = 1; }
	when ( "LFI_6" ) { $retval = 1; }
	when ( "LFI_7" ) { $retval = 1; }
	when ( "LFI_8" ) { $retval = 1; }
	when ( "LFI_9" ) { $retval = 1; }
	when ( "LFI_10" ) { $retval = 1; }
	when ( "LFI_11" ) { $retval = 1; }
	when ( "LFI_12" ) { $retval = 1; }
	when ( "YFEBM_0" ) { $retval = 1; }
	when ( "YFEBM_1" ) { $retval = 1; }
	when ( "YFEBM_2" ) { $retval = 1; }
	when ( "YFEBM_3" ) { $retval = 1; }
	when ( "YFEBM_4" ) { $retval = 1; }
	when ( "XFW_0" ) { $retval = 1; }
	when ( "XFW_1" ) { $retval = 1; }
	when ( "XFW_2" ) { $retval = 1; }
	when ( "XFW_3" ) { $retval = 1; }
	when ( "XFW_4" ) { $retval = 1; }
	when ( "XFC_0" ) { $retval = 1; }
	when ( "XFC_1" ) { $retval = 1; }
	when ( "XFC_2" ) { $retval = 1; }
	when ( "XFC_3" ) { $retval = 1; }
	when ( "XFC_4" ) { $retval = 1; }
	when ( "XFRC_0" ) { $retval = 1; }
	when ( "XFRC_1" ) { $retval = 1; }
	when ( "XFRC_2" ) { $retval = 1; }
	when ( "XFRC_3" ) { $retval = 1; }
	when ( "XFRC_4" ) { $retval = 1; }
    }
    $retval;
}

1
