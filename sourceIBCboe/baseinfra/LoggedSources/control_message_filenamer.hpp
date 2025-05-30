/**
    \file LoggedSources/control_message_filenamer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_MDSMESSAGES_CONTROL_MESSAGE_FILENAMER_H
#define BASE_MDSMESSAGES_CONTROL_MESSAGE_FILENAMER_H

#include <string>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"

namespace HFSAT {

class ControlMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             const int _query_id_) {
    static std::string logging_dir = "/NAS1/logs/QueryLogs/";
    std::stringstream st_;
    st_ << _preevent_YYYYMMDD_;
    std::string date_ = st_.str();
    std::string year_ = date_.substr(0, 4);
    std::string month_ = date_.substr(4, 2);
    std::string day_ = date_.substr(6, 2);
    st_.clear();
    st_.str("");

    st_ << _query_id_;
    std::string query_id_ = st_.str();
    return logging_dir + "/" + year_ + "/" + month_ + "/" + day_ + "/" + "log." + date_ + "." + query_id_ + ".gz";
  }
};
}

#endif  // BASE_MDSMESSAGES_CME_LOGGED_MESSAGE_FILENAMER_H
