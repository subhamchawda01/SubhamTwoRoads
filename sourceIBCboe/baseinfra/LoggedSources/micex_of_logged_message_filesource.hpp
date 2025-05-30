/**
   \file LoggedSources/micex_of_logged_message_filesource.hpp
   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"
#include "dvccode/CommonTradeUtils/trade_time_manager.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/micex_mds_defines.hpp"

namespace HFSAT {

/// @brief reads MICEX-EBS logged data stored in file for this { symbol, tradingdate }
/// Reads MICEX_OF_MDS::MICEXOFCommonStruct
/// depending on MICEX_OF_MDS::MICEXOFCommonStruct::msgType
/// calls the listeners, if any, of the security, action_
class MICEXOFLoggedMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;

  std::vector<OrderGlobalListener<MICEX_OF_MDS::MICEXOFCommonStruct>*> order_feed_global_listener_vec_;
  ExternalTimeListener* p_time_keeper_;

  BulkFileReader bulk_file_reader_;
  MICEX_OF_MDS::MICEXOFCommonStruct next_event_;

  /// To account for multiple locations
  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;
  HFSAT::TradeTimeManager& trade_time_manager_;

  std::vector<MICEX_OF_MDS::MICEXOFCommonStruct> event_queue_;
  int events_left_;
  ttime_t next_non_intermediate_time_;
  bool use_fake_faster_data_;

  // int lot_size_;

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
  MICEXOFLoggedMessageFileSource(DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                                 const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                                 const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
                                 bool t_use_fake_faster_data_ = true);

  inline int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t _endtime_, bool& rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  void ProcessEventsTill(const ttime_t t_endtime_);

  inline bool IsNormalTradeTime(int security_id_, ttime_t tv_) {
    return trade_time_manager_.isValidTimeToTrade(security_id_, tv_.tv_sec % 86400);
  }

  inline void SetOrderLevelGlobalListener(OrderGlobalListener<MICEX_OF_MDS::MICEXOFCommonStruct>* p_new_listener_) {
    AddOrderLevelGlobalListener(p_new_listener_);
  }

  void AddOrderLevelGlobalListener(OrderGlobalListener<MICEX_OF_MDS::MICEXOFCommonStruct>* p_this_listener_) {
    if (p_this_listener_ != NULL) {
      VectorUtils::UniqueVectorAdd(order_feed_global_listener_vec_, p_this_listener_);
    }
  }

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { p_time_keeper_ = t_new_listener_; }

 private:
  bool _SetNextTimeStamp();
  void _ProcessThisMsg();
};
}
