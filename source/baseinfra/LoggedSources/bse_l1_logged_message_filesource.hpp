/**
    \file LoggedSources/bse_l1_logged_message_filesource.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/generic_l1_data_struct.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "baseinfra/LoggedSources/bse_l1_logged_message_filenamer.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

namespace HFSAT {

/// @brief reads BSE logged data stored in file for this { symbol, tradingdate }
/// Reads BSE_MDS::BSECommonStruct
///    depending on BSE_MDS::BSECommonStruct::msgType
///    and for msgType == BSE_MDS::BSE_ORDER
///        depending on BSE_MDS::BSECommonStruct::data_::bse_order_::action_
///        calls the listeners, if any, of the security, action_
class BSEL1LoggedMessageFileSource : public ExternalDataListener {
 public:
  /**
   * @param t_dbglogger_ for logging errors
   * @param t_sec_name_indexer_ to detect if the security is of interest and not to process if not. If string matching
   * is more efficient we could use t_exchange_symbol_ as well.
   * @param t_preevent_YYYYMMDD_ tradingdate to load the appropriate file
   * @param t_security_id_ also same as t_sec_name_indexer_ [ t_exchange_symbol_ ]
   * @param t_exchange_symbol_ needed to match
   *
   * For now assuming t_exchange_symbol_ matching is not required
   */
  BSEL1LoggedMessageFileSource(DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                               const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                               const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
                               bool use_todays_data_ = false);

  int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  void ProcessEventsTill(const ttime_t t_endtime_);

  void SetL1DataListener(L1DataListener* listener);

  void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { p_time_keeper_ = t_new_listener_; }

  void SetTimeToSkipUntilFirstEvent(const ttime_t start_time);

  void SetActualDataSkip(bool _actual_data_skip_){
    actual_data_skip_ = _actual_data_skip_;
  }

 private:
  void _ProcessThisMsg();

 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;
  char shortcode_[BSE_DOTEX_OFFLINE_SYMBOL_LENGTH];

  L1DataListener* listener_;
  ExternalTimeListener* p_time_keeper_;

  BulkFileReader bulk_file_reader_;
  GenericL1DataStruct next_event_;
  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;
  bool skip_intermediate_message_;  // Skip the intra-day recovery messages.
  bool actual_data_skip_;
  HFSAT::TradeType_t last_seen_delta_type_;
  bool is_spot_;
};
}
