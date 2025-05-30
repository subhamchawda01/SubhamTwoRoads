/**
   \file MDSMessage/aflash_logged_message_file_source.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_MDSMESSAGES_AFLASH_LOGGED_MESSAGE_FILESOURCE_H
#define BASE_MDSMESSAGES_AFLASH_LOGGED_MESSAGE_FILESOURCE_H

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/LoggedSources/aflash_logged_message_filenamer.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_aflash_processor.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#define LOGGED_STRUCT_IS_LARGER

namespace HFSAT {

/// @brief reads AFLASH logged data stored in file for this { symbol, tradingdate }
/// Reads AFLASH_MDS::AFlashCommonStruct
///    depending on AFLASH_MDS::AFlashCommonStruct::msgType
///    and for type_
///        calls the listeners, if any
class AFLASHLoggedMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  CombinedMDSMessagesAflashProcessor* aflashprocessor_;

  ExternalTimeListener* p_time_keeper_;
  HFSAT::PriceLevelGlobalListener* price_level_global_listener_;

  BulkFileReader bulk_file_reader_;
  AFLASH_MDS::AFlashCommonStruct next_event_;
  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;
  ttime_t first_non_intermediate_time_;

  std::vector<AFLASH_MDS::AFlashCommonStruct> event_queue_;
  int events_left_;
  ttime_t next_non_intermediate_time_;
  bool use_fake_faster_data_;

 public:
  AFLASHLoggedMessageFileSource(DebugLogger& t_dbglogger_, const unsigned int t_preevent_YYYYMMDD_,
                                TradingLocation_t r_trading_location_);

  inline int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  void ProcessEventsTill(const ttime_t t_endtime_);

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) {
    p_time_keeper_ = t_new_listener_;
    aflashprocessor_->SetExternalTimeListener(p_time_keeper_);
  }

  void AddPriceLevelGlobalListener(PriceLevelGlobalListener* p_this_listener_);

 private:
  bool _SetNextTimeStamp();
  void _ProcessThisMsg();
  void ComputeFirstNonIntermediateTime(std::string logged_message_filesource_name_);
};
}
#endif  // BASE_MDSMESSAGES_AFLASH_LOGGED_MESSAGE_FILESOURCE_H
