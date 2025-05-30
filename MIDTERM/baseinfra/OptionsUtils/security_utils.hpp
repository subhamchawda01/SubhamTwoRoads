/**
    \file OptionsUtils/options_security_utils.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#define NSE_CONTRACT_FILENAME_PREFIX "NSE_Files/ContractFiles/nse_contracts"
#define NSE_DATASOURCE_EXCHSYMBOL_FILE "NSE_Files/datasource_exchsymbol.txt"
#define NSE_INTEREST_RATE_FILE "NSE_Files/interest_rates.txt"
#define NSE_BHAVCOPY_FILENAME_PREFIX "NSE_Files/BhavCopy/fo"
#define NSE_CD_BHAVCOPY_FILENAME_PREFIX "NSE_Files/BhavCopy/cds"

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/holiday_manager.hpp"
#include "dvccode/Utils/holiday_manager_utils.hpp"

// Unique -> Date
// Understands basecode ( NIFTY/RELIANCE/USDINR )

namespace HFSAT {
class OptionsSecurityUtils {
  // given a date and basecode, list all the option contracts based on
  // get (includes otm/itm/atm) high volume call/put contracts
  // get strike distance from ATM in terms of strike step, could be integer or double
  // print the mapping with respective shortcodes, print unmapped for missing shortcodes
  // strike in percentage of ATM
  // for now working only with first maturity
 public:
  static void GetMVInfo(const std::string& shortcode_, int yyyymmdd_, const std::string& r_opt_type_, bool is_currency,
                        int max_ = -1);
  static void GetOTMOptionContracts(const std::string& basecode_, int yyyymmdd_, const std::string& opt_type_,
                                    bool is_currency_, int max_ = -1);
};
}
