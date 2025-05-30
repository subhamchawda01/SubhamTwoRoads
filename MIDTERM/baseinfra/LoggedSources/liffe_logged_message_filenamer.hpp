/**
    \file LoggedSources/liffe_logged_message_filenamer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_MDSMESSAGES_LIFFE_LOGGED_MESSAGE_FILENAMER_H
#define BASE_MDSMESSAGES_LIFFE_LOGGED_MESSAGE_FILENAMER_H

#include <string>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/Utils/hybrid_sec.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

namespace HFSAT {

/// Returns the name of the data file where LIFFE MDS has logged data
/// Typically it will be of the format /NAS1/data/LIFFELoggedData/LIFFE/2010/12/01/ESM1_20101201
class LIFFELoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_) {
    static std::string logging_dir = "/NAS1/data/LIFFELoggedData/";
    std::string amr_code_filename_ = _exchange_symbol_;
    if (HFSAT::HybridSecurityManager::IsHybridExch(std::string(_exchange_symbol_))) {
      amr_code_filename_ = HFSAT::HybridSecurityManager::GetActualExchFromHybrid(std::string(_exchange_symbol_));
    }
    std::replace(amr_code_filename_.begin(), amr_code_filename_.end(), ' ', '~');

    return HFSAT::LoggedMessageFileNamer::GetName(logging_dir, amr_code_filename_.c_str(), _preevent_YYYYMMDD_,
                                                  rw_trading_location_, kTLocBSL);
  }
};
}

#endif  // BASE_MDSMESSAGES_LIFFE_LOGGED_MESSAGE_FILENAMER_H
