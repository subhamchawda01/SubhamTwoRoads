// =====================================================================================
//
//       Filename:  assumption.hpp
//
//    Description:  Only hold definitive macros for assumptions across codebase
//
//        Version:  1.0
//        Created:  04/05/2016 03:41:12 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#define NSE_FUTOPT_SEGMENT 'F'
#define NSE_FUTOPT_ADJUSTED_SEGMENT 'A'
#define NSE_CASH_SEGMENT 'E'
#define NSE_CURRENCY_SEGMENT 'C'

#define NSE_MEDIUMTERM_MBAR_DATA_LOCATION "/NAS1/data/NSEBarData/"
#define NSE_MEDIUMTERM_MBAR_DATA_FORMAT_TOKENS 11

#define NSE_FUTOPT_DIR "FUT_BarData/"
#define NSE_FUTOPT_ADJUSTED_DIR "FUT_BarData_Adjusted/"

#define NSE_HFTRAP_SERIALIZED_DATAFILES_DIR "/spare/local/MDSlogs/MT_SPRD_SERIALIZED_DUMP/"
#define NSE_HFTRAP_DEBUG_LOGS "/spare/local/MDSlogs/MT_SPRD_DBGLOGS/"
#define NSE_HFTRAP_NAV_SERIES "/spare/local/MDSlogs/MT_SPRD_NAV_SERIES/"
#define NSE_HFTRAP_TRADE_FILE "/spare/local/MDSlogs/MT_SPRD_TRADES/"
#define NSE_HFTRAP_TRADE_EXEC "/spare/local/MDSlogs/MT_SPRD_EXEC/"
