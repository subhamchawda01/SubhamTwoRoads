// =====================================================================================
//
//       Filename:  sim_trade_filesource.hpp
//
//    Description:
//
//        Version:  1.0
//        Cre  03/18/2016 10:31:11 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#define BASE_ORSMESSAGES_ORS_MESSAGE_FILESOURCE_H

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/Utils/tcp_server_socket_listener.hpp"

namespace HFSAT {

class SimTradeFileSource : public ExternalDataListener {
protected:
  DebugLogger &dbglogger_;
  SecurityNameIndexer &sec_name_indexer_;
  const unsigned int security_id_;
  const char *exchange_symbol_;

  ExternalTimeListener *p_time_keeper_; ///< only meant for Watch
  std::vector<Utils::TCPServerSocketListener *> order_listeners;

  std::ifstream file_reader_;
  std::string next_event_;

public:
  /** Constructor
   *
   * @param _dbglogger_ for logging errors
   * @param _sec_name_indexer_ to detect if the security is of interest and not
   *to process if not
   * @param _security_id_ also same as _sec_name_indexer_ [ _exchange_symbol_ ]
   * @param _exchange_symbol_ needed to match
   *
   * For now assuming that this file has only data of this symbol and hence
   *_exchange_symbol_ matching is not required
   **/
  SimTradeFileSource(DebugLogger &_dbglogger_,
                     SecurityNameIndexer &_sec_name_indexer_,
                     const unsigned int _security_id_,
                     const char *_exchange_symbol_)
      : dbglogger_(_dbglogger_), sec_name_indexer_(_sec_name_indexer_),
        security_id_(_security_id_), exchange_symbol_(_exchange_symbol_),
        p_time_keeper_(NULL), file_reader_(),
        next_event_(std::move(next_event_)) {
    next_event_timestamp_.tv_sec = 0;
    next_event_timestamp_.tv_usec = 0;

    // find the filename
    // std::string _trades_filename_ = "/home/rgarg/orders_file";
    std::string _trades_filename_ =
        std::string(getenv("HOME")) +
        std::string("/basetrade/Logs_Temp/orders_file");

    file_reader_.open(_trades_filename_.c_str());
  }

  inline int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t r_start_time_, bool &rw_hasevents_);
  void ComputeEarliestDataTimestamp(bool &_hasevents_);
  void ProcessAllEvents();
  void ProcessEventsTill(const ttime_t _endtime_);
  bool SetNextTimeStamp();
  void ProcessThisEvent();

  void NotifyListeners();
  void AddListener(HFSAT::Utils::TCPServerSocketListener *listener_);

  void SetExternalTimeListener(ExternalTimeListener *_new_listener_) {
    p_time_keeper_ = _new_listener_;
  }
};
}
