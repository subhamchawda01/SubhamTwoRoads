#!/bin/bash

# BRZ
$HOME/basetrade/scripts/get_thin_book_periods.pl BR_DOL_0 /spare/local/tradeinfo/datageninfo/thin_book_periods.BR_DOL_0.US 1200 2000 60 0.1

# CME US
$HOME/basetrade/scripts/get_thin_book_periods.pl ZF_0 /spare/local/tradeinfo/datageninfo/thin_book_periods.ZF_0.US 1200 2000 60 0.1
$HOME/basetrade/scripts/get_thin_book_periods.pl ZN_0 /spare/local/tradeinfo/datageninfo/thin_book_periods.ZN_0.US 1200 2000 60 0.1
$HOME/basetrade/scripts/get_thin_book_periods.pl ZB_0 /spare/local/tradeinfo/datageninfo/thin_book_periods.ZB_0.US 1200 2000 60 0.1

# EUREX US
$HOME/basetrade/scripts/get_thin_book_periods.pl FGBL_0 /spare/local/tradeinfo/datageninfo/thin_book_periods.FGBL_0.US 1200 1700 60 0.1
$HOME/basetrade/scripts/get_thin_book_periods.pl FGBM_0 /spare/local/tradeinfo/datageninfo/thin_book_periods.FGBM_0.US 1200 1700 60 0.1
$HOME/basetrade/scripts/get_thin_book_periods.pl FGBS_0 /spare/local/tradeinfo/datageninfo/thin_book_periods.FGBS_0.US 1200 1700 60 0.1

#OSE EU
$HOME/basetrade/scripts/get_thin_book_periods.pl JGBL_0 /spare/local/tradeinfo/datageninfo/thin_book_periods.JGBL_0.EU 0730 1300 30 0.1

#OSE_US
$HOME/basetrade/scripts/get_thin_book_periods.pl JGBL_0 /spare/local/tradeinfo/datageninfo/thin_book_periods.JGBL_0.US 1300 1730 30 0.1
