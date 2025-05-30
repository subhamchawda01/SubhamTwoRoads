#include <stdlib.h>

#include "dvccode/CombinedControlUtils/combined_control_message_livesource.hpp"
#include "dvccode/Utils/combined_source_generic_logger.hpp"

namespace HFSAT {

namespace Utils {

CombinedSourceGenericLogger* CombinedSourceGenericLogger::uniqueinstance_ = nullptr;

CombinedSourceGenericLogger& CombinedSourceGenericLogger::GetUniqueInstance() {
  if (uniqueinstance_ == nullptr) {
    uniqueinstance_ = new CombinedSourceGenericLogger();
  }
  return (*uniqueinstance_);
}

void CombinedSourceGenericLogger::ResetUniqueInstance() {
  if (uniqueinstance_ != nullptr) {
    delete uniqueinstance_;
    uniqueinstance_ = nullptr;
  }
}

CombinedSourceGenericLogger::CombinedSourceGenericLogger()
    : logger_thread_(new MDSLogger<HFSAT::MDS_MSG::GenericMDSMessage>("GENERIC")), is_logging_enabled_(false) {
  if (logger_thread_ == nullptr) {
    std::cerr << "Cannot Create MDSLoggerThread in CombinedSourceGenericLogger. Exiting." << std::endl;
    exit(1);
  }

  logger_thread_->EnableAffinity("CombinedSourceGenericLogger");
}

CombinedSourceGenericLogger::~CombinedSourceGenericLogger() {
  if (logger_thread_ != nullptr) {
    logger_thread_->closeFiles();
    delete logger_thread_;
    logger_thread_ = nullptr;
  }
}

void CombinedSourceGenericLogger::Log(const HFSAT::MDS_MSG::GenericMDSMessage& message) {
  // Unconditionally log ORS Replies And Control messages
  if (HFSAT::MDS_MSG::ORS_REPLY == message.mds_msg_exch_ || HFSAT::MDS_MSG::CONTROL == message.mds_msg_exch_ ||
      is_logging_enabled_) {
    logger_thread_->log(message);
  }
}

void CombinedSourceGenericLogger::SubscribeToCombinedControlMessage(
    HFSAT::CombinedControlMessageLiveSource* p_cmd_control_live_source_) {
  if (p_cmd_control_live_source_) {
    p_cmd_control_live_source_->AddCombinedControlMessageListener(logger_thread_);
  }
}

void CombinedSourceGenericLogger::EnableLogging() { is_logging_enabled_ = true; }
bool CombinedSourceGenericLogger::IsLoggingEnabled() { return is_logging_enabled_; }

void CombinedSourceGenericLogger::AddExchSources(std::set<HFSAT::ExchSource_t>* exch_sources) {
  logger_thread_->AddExchSources(exch_sources);
}

void CombinedSourceGenericLogger::RunLoggerThread() { logger_thread_->run(); }
}  // namespace Utils
}  // namespace HFSAT
