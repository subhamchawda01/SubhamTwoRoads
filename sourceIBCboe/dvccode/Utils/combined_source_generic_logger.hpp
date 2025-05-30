#ifndef _COMBINED_SOURCE_GENERIC_LOGGER_HPP_
#define _COMBINED_SOURCE_GENERIC_LOGGER_HPP_

#include <set>

#include "dvccode/Utils/mds_logger.hpp"

namespace HFSAT {

class CombinedControlMessageLiveSource;  // Forward Declaration

namespace Utils {

class CombinedSourceGenericLogger {
 public:
  static CombinedSourceGenericLogger& GetUniqueInstance();
  static void ResetUniqueInstance();

  void Log(const HFSAT::MDS_MSG::GenericMDSMessage& message);
  void SubscribeToCombinedControlMessage(HFSAT::CombinedControlMessageLiveSource* p_cmd_control_live_source_);
  void EnableLogging();
  void AddExchSources(std::set<HFSAT::ExchSource_t>* exch_sources);
  bool IsLoggingEnabled();
  void RunLoggerThread();
  void FlushAndCloseFiles();

 protected:
  CombinedSourceGenericLogger();
  ~CombinedSourceGenericLogger();

 private:
  static CombinedSourceGenericLogger* uniqueinstance_;

  MDSLogger<HFSAT::MDS_MSG::GenericMDSMessage>* logger_thread_;
  bool is_logging_enabled_;
};
}
}

#endif
