/**
    \file dvccode/Utils/data_daemon_config.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include <tr1/unordered_map>
#include <fstream>
#include <set>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {

class DataDaemonConfig {
 public:
  static DataDaemonConfig& GetUniqueInstance(std::string config_);
  static DataDaemonConfig& GetUniqueInstance();

  bool ExchModeExists(std::string exchange_, FastMdConsumerMode_t mode_);
  bool IsComShm(std::string exchange_);
  bool IsLogger(std::string exchange_);
  std::set<std::string> GetExchSet();
  std::string GetOperatingMode();

 private:
  DataDaemonConfig(std::string config_);

  void AddExchModePair(std::string exchange_, FastMdConsumerMode_t mode_);
  FastMdConsumerMode_t GetModeFromString(std::string mode_str_);
  void ParseConfig();

  std::string config_file_;
  std::tr1::unordered_map<std::string, std::vector<FastMdConsumerMode_t> > exch_mode_;
  static DataDaemonConfig* unique_instance_;
};
}
