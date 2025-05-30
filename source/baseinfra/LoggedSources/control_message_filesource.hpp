/**
    \file LoggedSources/control_message_filesource.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"

#include "dvccode/CDef/control_messages.hpp"
#include "dvccode/ORSMessages/control_message_listener.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#define LOGGED_STRUCT_IS_LARGER

namespace HFSAT {

class ControlMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;
  const int query_id_;
  ExternalTimeListener* p_time_keeper_;
  GenericControlRequestStruct next_event_;
  std::vector<ControlMessageListener*> control_message_listener_vec_;
  BulkFileReader bulk_file_reader_;

 public:
  ControlMessageFileSource(DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                           const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                           const char* t_exchange_symbol_, TradingLocation_t t_trading_location_, const int t_query_id_,
                           bool& control_file_present);

  ~ControlMessageFileSource() {}

  inline int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  void ProcessEventsTill(const ttime_t t_endtime_);

  void AddControlMessageListener(ControlMessageListener* _this_control_message_listener_) {
    VectorUtils::UniqueVectorAdd(control_message_listener_vec_, _this_control_message_listener_);
  }
  void RemoveControlMessagListner(ControlMessageListener* _this_control_message_listener_) {
    VectorUtils::UniqueVectorRemove(control_message_listener_vec_, _this_control_message_listener_);
  }

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { p_time_keeper_ = t_new_listener_; }

 private:
  bool _SetNextTimeStamp();
  void _ProcessThisMsg();
};
}
