/**
   \file LoggedSources/real_packets_filenamer.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/


#include <string>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/s3_calls.hpp"
#include "dvccode/Utils/common_files_path.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

namespace HFSAT {

class RealPacketsOrderFileNamer {
 public:
  static std::string GetName(const unsigned int _preevent_YYYYMMDD_, TradingLocation_t& rw_trading_location_) {
    std::string real_packets_dir_name = std::string(HFSAT::FILEPATH::kRealPacketsOrderDirectory);
    const char* real_packets_file_prefix = HFSAT::FILEPATH::kRealPacketsOrderFileName;

    return HFSAT::LoggedMessageFileNamer::GetName(real_packets_dir_name, real_packets_file_prefix, _preevent_YYYYMMDD_,
                                                  rw_trading_location_, kTLocNY4);
  }
};
}

