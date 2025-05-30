/**
   \file OrderRouting/ors_message_stats_computer.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_ORSMESSAGES_ORS_MESSAGES_STATS_COMPUTER_H
#define BASE_ORSMESSAGES_ORS_MESSAGES_STATS_COMPUTER_H

#include <sys/types.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "dvccode/ORSMessages/ors_message_listener.hpp"
#include "dvccode/Utils/cxl_seqd_to_conf.hpp"
#include "baseinfra/LoggedSources/ors_conf_market_data_delay_computer.hpp"  //if enabled this guy will add the additional conf-update delay to the seqd-conf numbers
#include "baseinfra/LoggedSources/ors_message_filesource.hpp"

#define PRECOMPUTED_SEQD_TO_CONF_LOCATION \
  "/apps/data/ORSData/SEQCONF"  // Ideally don't want to write to the file - server.

#define PRECOMPUTED_ACCURATE_MARKETDELAY_LOCATION \
  "/apps/data/ORSData/MKTDELAY/dvctrader"  // Ideally don't want to write to the file - server.
#define PRECOMPUTED_ACCURATE_CXL_MARKETDELAY_LOCATION \
  "/apps/data/ORSData/MKTDELAY/dvctrader"  // Ideally don't want to write to the file - server.

namespace HFSAT {

struct ConfirmedOrderStats {
  int server_assigned_client_id_;
  int server_assigned_order_sequence_;
  double price_;
  TradeType_t buysell_;
  int size_remaining_;
  int int_price_;
  ttime_t confirmation_time_;
};

class ORSMessageStatsComputer : public OrderNotFoundListener,
                                public OrderSequencedListener,
                                public OrderConfirmedListener,
                                public OrderConfCxlReplacedListener,
                                public OrderCanceledListener,
                                public OrderExecutedListener,
                                public OrderRejectedListener {
 public:
  ORSMessageStatsComputer(DebugLogger& _dbglogger_, Watch& _watch_, bool t_print_verification_info_ = false)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        print_verification_info_(t_print_verification_info_),
        saos_to_seqd_time_(),
        compute_seqd_to_conf_(false),
        time_to_seqd_to_conf_time_(),
        time_to_conf_market_delay_time_(),
        time_to_cxl_market_delay_time_(),
        compute_rejection_periods_(false),
        time_to_num_rejections_(),
        compute_cxl_rejection_periods_(false),
        time_to_num_cxl_rejections_(),
        compute_accurate_conf_to_market_upadte_delay_(false),
        compute_accurate_cxl_to_market_upadte_delay_(false),
        ors_conf_market_delay_computer_(NULL) {}

  ~ORSMessageStatsComputer() {}

  void ComputeSeqdToConf(const bool t_compute_) { compute_seqd_to_conf_ = t_compute_; }

  void ComputeRejectionPeriods(const bool t_compute_) { compute_rejection_periods_ = t_compute_; }

  void ComputeCxlRejectionPeriods(const bool t_compute_) { compute_cxl_rejection_periods_ = t_compute_; }

  void ComputeAccurateConfMarketUpdate(std::string shortcode_, int yyyymmdd_) {
    //      std::cout << " Computing Dynamic Delay \n" ;
    compute_accurate_conf_to_market_upadte_delay_ = true;

    ors_conf_market_delay_computer_ = new HFSAT::ORSConfMarketDelayComputer(shortcode_, yyyymmdd_);
  }

  void ComputeAccurateCxlMarketUpdate(std::string shortcode_, int yyyymmdd_) {
    compute_accurate_cxl_to_market_upadte_delay_ = true;

    ors_conf_market_delay_computer_ = new HFSAT::ORSConfMarketDelayComputer(shortcode_, yyyymmdd_);
  }

  std::map<ttime_t, ttime_t>& TimeToSeqdToConfTime() { return time_to_seqd_to_conf_time_; }
  std::map<ttime_t, ttime_t>& TimeToConfToMarketDelayTime() { return time_to_conf_market_delay_time_; }
  std::map<ttime_t, ttime_t>& TimeToCxlToMarketDelayTime() { return time_to_cxl_market_delay_time_; }
  std::map<ttime_t, unsigned>& TimeToNumRejections() { return time_to_num_rejections_; }
  std::map<ttime_t, unsigned>& TimeToNumCxlRejections() { return time_to_num_cxl_rejections_; }
  std::map<ttime_t, ConfirmedOrderStats>& TimeToSeqdToConfStats() { return time_to_seqd_to_conf_stats_; }
  std::map<ttime_t, ConfirmedOrderStats>& TimeToConfToMarketDelayStats() { return time_to_conf_market_delay_stats_; }

  void OrderNotFound(const int r_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                     const TradeType_t _buysell_, const int _int_price_, const int32_t server_assigned_message_sequence,
                     const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

  void OrderSequenced(const int r_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                      const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                      const int _size_executed_, const int _client_position_, const int _global_position_,
                      const int _int_price_, const int32_t server_assigned_message_sequence,
                      const uint64_t exchange_order_id, const ttime_t time_set_by_server) { /*
                            if ( ! compute_seqd_to_conf_ && !
                            compute_accurate_conf_to_market_upadte_delay_ && !
                            compute_accurate_cxl_to_market_upadte_delay_ ) { return; }

                            if ( saos_to_seqd_time_.find (
                            _server_assigned_order_sequence_ ) == saos_to_seqd_time_.end
                            ( ) )
                              {
                                saos_to_seqd_time_ [ _server_assigned_order_sequence_ ] =
                            watch_.tv ( );
                              }*/
  }

  void OrderSequencedAtTime(const int r_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                            const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                            const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                            const int _size_executed_, const int _client_position_, const int _global_position_,
                            const int _int_price_, const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
    if (!compute_seqd_to_conf_ && !compute_accurate_conf_to_market_upadte_delay_ &&
        !compute_accurate_cxl_to_market_upadte_delay_) {
      return;
    }

    if (saos_to_seqd_time_.find(_server_assigned_order_sequence_) == saos_to_seqd_time_.end()) {
      saos_to_seqd_time_[_server_assigned_order_sequence_] = time_set_by_server;
    }
  }

  void OrderConfirmed(const int r_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                      const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                      const int _size_executed_, const int _client_position_, const int _global_position_,
                      const int _int_price_, const int32_t server_assigned_message_sequence,
                      const uint64_t exchange_order_id, const ttime_t time_set_by_server) { /*
                         */
  }
  void OrderConfirmedAtTime(const int r_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                            const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                            const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                            const int _size_executed_, const int _client_position_, const int _global_position_,
                            const int _int_price_, const ttime_t _time_set_by_server_,
                            const uint64_t t_exch_assigned_sequence_, const uint64_t exchange_order_id,
                            const ttime_t time_set_by_server) {
    if (!compute_seqd_to_conf_ && !compute_accurate_conf_to_market_upadte_delay_) {
      return;
    }

    std::map<int, ttime_t>::iterator saos_itr_ = saos_to_seqd_time_.find(_server_assigned_order_sequence_);

    HFSAT::ttime_t this_accurate_conf_market_delay_(0, 0);

    if (compute_accurate_conf_to_market_upadte_delay_) {
      this_accurate_conf_market_delay_ = (ors_conf_market_delay_computer_->getAccurateConfToMarketUpdateDelay(
          _time_set_by_server_, _price_, _buysell_, _size_remaining_));
    }

    if (saos_itr_ != saos_to_seqd_time_.end()) {
      time_to_seqd_to_conf_time_[saos_itr_->second] = (_time_set_by_server_ - saos_itr_->second);

      saos_to_seq_conf_time_[_server_assigned_order_sequence_] = time_to_seqd_to_conf_time_[saos_itr_->second];

      if (compute_accurate_conf_to_market_upadte_delay_) {
        time_to_conf_market_delay_time_[saos_itr_->second] = this_accurate_conf_market_delay_;

        //            std::cout << " This Mkt Delay : " << this_accurate_conf_market_delay_.ToString() << "\n" ;
      }

      // DBGLOG_TIME_CLASS_FUNC << "saos=" << _server_assigned_order_sequence_ << " seqd=" << saos_itr_ -> second << "
      // conf=" << watch_.tv ( ) << " seq-to-conf=" << time_to_seqd_to_conf_time_ [ saos_itr_ -> second ] <<
      // DBGLOG_ENDL_FLUSH;
    }
    // These stats will be used by Security Market View Analyser to compute Conf to Update Delay By Finding Exact
    // Matches in the Maeket Feed
    if (saos_itr_ != saos_to_seqd_time_.end()) {
      time_to_seqd_to_conf_stats_[saos_itr_->second].server_assigned_client_id_ = r_server_assigned_client_id_;
      time_to_seqd_to_conf_stats_[saos_itr_->second].server_assigned_order_sequence_ = _server_assigned_order_sequence_;
      time_to_seqd_to_conf_stats_[saos_itr_->second].price_ = _price_;
      time_to_seqd_to_conf_stats_[saos_itr_->second].int_price_ = _int_price_;
      time_to_seqd_to_conf_stats_[saos_itr_->second].size_remaining_ = _size_remaining_;
      time_to_seqd_to_conf_stats_[saos_itr_->second].buysell_ = _buysell_;

      if (compute_accurate_conf_to_market_upadte_delay_) {
        time_to_seqd_to_conf_stats_[saos_itr_->second].confirmation_time_ =
            _time_set_by_server_ + this_accurate_conf_market_delay_;

      } else {
        time_to_seqd_to_conf_stats_[saos_itr_->second].confirmation_time_ = _time_set_by_server_;
      }

      // DBGLOG_TIME_CLASS_FUNC << "saos=" << _server_assigned_order_sequence_ << " seqd=" << saos_itr_ -> second << "
      // conf=" << watch_.tv ( ) << " seq-to-conf=" << time_to_seqd_to_conf_time_ [ saos_itr_ -> second ] <<
      // DBGLOG_ENDL_FLUSH;
    }
  }

  void OrderORSConfirmed(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                         const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                         const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                         const int _size_executed_, const int _int_price_,
                         const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                         const ttime_t time_set_by_server) {}

  void OrderConfCxlReplaced(const int r_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                            const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                            const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                            const int _size_executed_, const int _client_position_, const int _global_position_,
                            const int _int_price_, const int32_t server_assigned_message_sequence,
                            const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

  void OrderCanceled(const int r_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t _buysell_, const int _size_remaining_, const int _client_position_,
                     const int _global_position_, const int _int_price_, const int32_t server_assigned_message_sequence,
                     const uint64_t exchange_order_id, const ttime_t time_set_by_server) { /*
                        */
  }

  /**
   *
   * @param r_server_assigned_client_id_
   * @param _client_assigned_order_sequence_
   * @param _server_assigned_order_sequence_
   * @param _security_id_
   * @param _price_
   * @param _buysell_
   * @param _size_remaining_
   * @param _client_position_
   * @param _global_position_
   * @param _int_price_
   * @param _time_set_by_server_
   * @param t_exch_assigned_sequence_
   */
  void OrderCanceledAtTime(const int r_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                           const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                           const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                           const int _client_position_, const int _global_position_, const int _int_price_,
                           const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
    if (!compute_accurate_cxl_to_market_upadte_delay_) {
      return;
    }

    std::map<int, ttime_t>::iterator saos_itr_ = saos_to_seqd_time_.find(_server_assigned_order_sequence_);

    HFSAT::ttime_t this_accurate_cxl_market_delay_(0, 0);

    if (compute_accurate_cxl_to_market_upadte_delay_) {
      this_accurate_cxl_market_delay_ = (ors_conf_market_delay_computer_->getAccurateCxlToMarketUpdateDelay(
          time_set_by_server, _price_, _buysell_, _size_remaining_));
    }

    if (saos_itr_ != saos_to_seqd_time_.end()) {
      if (compute_accurate_cxl_to_market_upadte_delay_) {
        time_to_cxl_market_delay_time_[saos_itr_->second] = this_accurate_cxl_market_delay_;
      }
    }
  }

  /**
   *
   * @param t_server_assigned_client_id_
   * @param _client_assigned_order_sequence_
   * @param _server_assigned_order_sequence_
   * @param _security_id_
   * @param _price_
   * @param t_buysell_
   * @param _size_remaining_
   * @param _rejection_reason_
   * @param t_client_position_
   * @param t_global_position_
   * @param r_int_price_
   * @param exchange_order_id
   * @param time_set_by_server
   */
  void OrderCancelRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                           const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                           const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                           const int _rejection_reason_, const int t_client_position_, const int t_global_position_,
                           const int r_int_price_, const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  }

  /**
   *
   * @param r_server_assigned_client_id_
   * @param _client_assigned_order_sequence_
   * @param _server_assigned_order_sequence_
   * @param _security_id_
   * @param _price_
   * @param _buysell_
   * @param _size_remaining_
   * @param _size_executed_
   * @param _client_position_
   * @param _global_position_
   * @param _int_price_
   * @param server_assigned_message_sequence
   * @param exchange_order_id
   * @param time_set_by_server
   */
  void OrderExecuted(const int r_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t _buysell_, const int _size_remaining_, const int _size_executed_,
                     const int _client_position_, const int _global_position_, const int _int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const ttime_t time_set_by_server) {}

  /**
   *
   * @param r_server_assigned_client_id_
   * @param _client_assigned_order_sequence_
   * @param _security_id_
   * @param _price_
   * @param _buysell_
   * @param _size_remaining_
   * @param _rejection_reason_
   * @param _int_price_
   * @param exchange_order_id
   * @param time_set_by_server
   */
  void OrderRejected(const int r_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const unsigned int _security_id_, const double _price_, const TradeType_t _buysell_,
                     const int _size_remaining_, const int _rejection_reason_, const int _int_price_,
                     const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
    if (!(compute_rejection_periods_ || compute_cxl_rejection_periods_)) {
      return;
    }

    ttime_t time_secs_ = watch_.tv();
    time_secs_.tv_usec = 0;

    switch (_rejection_reason_) {
      case HFSAT::kORSRejectSecurityNotFound:
      case HFSAT::kORSRejectMarginCheckFailedOrderSizes:
      case HFSAT::kORSRejectMarginCheckFailedMaxPosition:
      case HFSAT::kORSRejectMarginCheckFailedWorstCasePosition:
      case HFSAT::kExchOrderReject:
      case HFSAT::kORSRejectMarginCheckFailedMaxLiveOrders:
      case HFSAT::kORSRejectSelfTradeCheck:
      case HFSAT::kORSRejectNewOrdersDisabled: {
        time_to_num_rejections_[time_secs_]++;
      } break;
      case HFSAT::kORSRejectThrottleLimitReached: {
        time_to_num_rejections_[time_secs_]++;
        time_to_num_cxl_rejections_[time_secs_]++;
      } break;
    }
  }

 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  bool print_verification_info_;

  std::map<int, ttime_t> saos_to_seqd_time_;
  bool compute_seqd_to_conf_;
  std::map<ttime_t, ttime_t> time_to_seqd_to_conf_time_;
  std::map<ttime_t, ttime_t> time_to_conf_market_delay_time_;
  std::map<ttime_t, ttime_t> time_to_cxl_market_delay_time_;
  bool compute_rejection_periods_;
  std::map<ttime_t, unsigned> time_to_num_rejections_;
  bool compute_cxl_rejection_periods_;
  std::map<ttime_t, unsigned> time_to_num_cxl_rejections_;

  std::map<ttime_t, ConfirmedOrderStats> time_to_seqd_to_conf_stats_;
  std::map<ttime_t, ConfirmedOrderStats> time_to_conf_market_delay_stats_;  // market delay adjusted conf time

  std::map<int, ttime_t> saos_to_seq_conf_time_;

  bool compute_accurate_conf_to_market_upadte_delay_;
  bool compute_accurate_cxl_to_market_upadte_delay_;

  HFSAT::ORSConfMarketDelayComputer* ors_conf_market_delay_computer_;

  std::string this_shortcode_;
  int yyyymmdd_;

 public:
  static std::map<HFSAT::ttime_t, HFSAT::ConfirmedOrderStats> GetSeqdToConfStats(
      HFSAT::DebugLogger& dbglogger_, HFSAT::SecurityNameIndexer& sec_name_indexer_, const int tradingdate_,
      const unsigned dep_security_id_, HFSAT::TradingLocation_t dep_trading_location_,
      bool print_verification_info_ = false) {
    // This function is being called from The security_market_view_analyser so as to compute Conf to Update Delay from
    // the ORS Logs
    HFSAT::HistoricalDispatcher historical_dispatcher_;

    HFSAT::ORSMessageFileSource* p_ors_message_file_source_ = new HFSAT::ORSMessageFileSource(
        dbglogger_, sec_name_indexer_, tradingdate_, dep_security_id_,
        sec_name_indexer_.GetSecurityNameFromId(dep_security_id_), dep_trading_location_);

    HFSAT::Watch watch_(dbglogger_, tradingdate_);

    p_ors_message_file_source_->SetExternalTimeListener(&watch_);

    historical_dispatcher_.AddExternalDataListener(p_ors_message_file_source_);

    HFSAT::ORSMessageStatsComputer* p_ors_message_stats_computer_ = NULL;

    p_ors_message_stats_computer_ = new HFSAT::ORSMessageStatsComputer(dbglogger_, watch_);

    p_ors_message_stats_computer_->ComputeSeqdToConf(true);

    p_ors_message_file_source_->AddOrderSequencedListener(p_ors_message_stats_computer_);
    p_ors_message_file_source_->AddOrderConfirmedListener(p_ors_message_stats_computer_);

    try {
      historical_dispatcher_.RunHist();
    } catch (int e) {
    }

    std::map<HFSAT::ttime_t, HFSAT::ConfirmedOrderStats> time_to_seqd_to_conf_stats_ =
        std::map<HFSAT::ttime_t, ConfirmedOrderStats>(p_ors_message_stats_computer_->TimeToSeqdToConfStats());

    return time_to_seqd_to_conf_stats_;
  }

  static std::map<HFSAT::ttime_t, HFSAT::ConfirmedOrderStats> GetConfToMarketDelayStats(
      HFSAT::DebugLogger& dbglogger_, HFSAT::SecurityNameIndexer& sec_name_indexer_, const int tradingdate_,
      const unsigned dep_security_id_, HFSAT::TradingLocation_t dep_trading_location_,
      bool print_verification_info_ = false) {
    // This function is being called from The security_market_view_analyser so as to compute Conf to Update Delay from
    // the ORS Logs
    HFSAT::HistoricalDispatcher historical_dispatcher_;

    HFSAT::ORSMessageFileSource* p_ors_message_file_source_ = new HFSAT::ORSMessageFileSource(
        dbglogger_, sec_name_indexer_, tradingdate_, dep_security_id_,
        sec_name_indexer_.GetSecurityNameFromId(dep_security_id_), dep_trading_location_);

    HFSAT::Watch watch_(dbglogger_, tradingdate_);

    p_ors_message_file_source_->SetExternalTimeListener(&watch_);

    historical_dispatcher_.AddExternalDataListener(p_ors_message_file_source_);

    HFSAT::ORSMessageStatsComputer* p_ors_message_stats_computer_ = NULL;

    p_ors_message_stats_computer_ = new HFSAT::ORSMessageStatsComputer(dbglogger_, watch_);

    p_ors_message_stats_computer_->ComputeAccurateConfMarketUpdate(
        sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_);

    p_ors_message_file_source_->AddOrderSequencedListener(p_ors_message_stats_computer_);
    p_ors_message_file_source_->AddOrderConfirmedListener(p_ors_message_stats_computer_);

    try {
      historical_dispatcher_.RunHist();
    } catch (int e) {
    }

    std::map<HFSAT::ttime_t, HFSAT::ConfirmedOrderStats> time_to_conf_market_delay_time_ =
        std::map<HFSAT::ttime_t, ConfirmedOrderStats>(p_ors_message_stats_computer_->TimeToConfToMarketDelayStats());

    return time_to_conf_market_delay_time_;
  }

  static bool IsPrecomputedSeqdToConfAvailable(const std::string shortcode_, const int tradingdate_) {
    std::string precomputed_seqd_to_conf_filename_ = "";
    {
      std::ostringstream t_oss_;
      t_oss_ << PRECOMPUTED_SEQD_TO_CONF_LOCATION << "/" << shortcode_ << "_" << tradingdate_;

      precomputed_seqd_to_conf_filename_ = t_oss_.str();
    }

    int size_ = 0;
    if (FileUtils::ExistsAndReadable(precomputed_seqd_to_conf_filename_.c_str()) &&
        FileUtils::ExistsWithSize(precomputed_seqd_to_conf_filename_.c_str(), size_)) {
      return true;
    }

    return false;
  }

  static bool IsPrecomputedConfMarketDelayAvailable(const std::string shortcode_, const int tradingdate_) {
    std::string precomputed_conf_market_delay_filename_ = "";
    {
      // TODO - for now using different directory for all users so all can manipulate as needed for testing
      std::ostringstream t_oss_;
      t_oss_ << PRECOMPUTED_ACCURATE_MARKETDELAY_LOCATION << "/" << shortcode_ << "_" << tradingdate_;

      precomputed_conf_market_delay_filename_ = t_oss_.str();
    }
    int size_ = 0;
    if (FileUtils::ExistsAndReadable(precomputed_conf_market_delay_filename_.c_str()) &&
        FileUtils::ExistsWithSize(precomputed_conf_market_delay_filename_.c_str(), size_)) {
      return true;
    }

    return false;
  }

  static bool IsPrecomputedCxlMarketDelayAvailable(const std::string shortcode_, const int tradingdate_) {
    std::string precomputed_cxl_market_delay_filename_ = "";
    {
      // TODO - for now using different directory for all users so all can manipulate as needed for testing
      std::ostringstream t_oss_;
      t_oss_ << PRECOMPUTED_ACCURATE_CXL_MARKETDELAY_LOCATION << "/" << shortcode_ << "_" << tradingdate_ << "_"
             << "CXL";

      precomputed_cxl_market_delay_filename_ = t_oss_.str();
    }
    int size_ = 0;
    if (FileUtils::ExistsAndReadable(precomputed_cxl_market_delay_filename_.c_str()) &&
        FileUtils::ExistsWithSize(precomputed_cxl_market_delay_filename_.c_str(), size_)) {
      return true;
    }

    return false;
  }

  static std::map<HFSAT::ttime_t, HFSAT::ttime_t> GetPrecomputedSeqdToConf(const std::string shortcode_,
                                                                           const int tradingdate_) {
    std::string precomputed_seqd_to_conf_filename_ = "";
    {
      std::ostringstream t_oss_;
      t_oss_ << PRECOMPUTED_SEQD_TO_CONF_LOCATION << "/" << shortcode_ << "_" << tradingdate_;

      precomputed_seqd_to_conf_filename_ = t_oss_.str();
    }

    std::ifstream precomputed_seqd_to_conf_file_;
    precomputed_seqd_to_conf_file_.open(precomputed_seqd_to_conf_filename_.c_str(), std::ifstream::in);

    if (!precomputed_seqd_to_conf_file_.is_open()) {
      std::cerr << " Could not open precomputed_seqd_to_conf_file_=" << precomputed_seqd_to_conf_filename_ << std::endl;
      return std::map<HFSAT::ttime_t, HFSAT::ttime_t>();
    }

    char line_[100];
    memset(line_, 0, sizeof(line_));
    std::map<HFSAT::ttime_t, HFSAT::ttime_t> time_to_seqd_to_conf_time_;

    while (precomputed_seqd_to_conf_file_.getline(line_, sizeof(line_))) {
      const char* p_secs_ = strtok(line_, " \t\n");
      if (p_secs_) {
        const char* p_usecs_ = strtok(NULL, " \t\n");
        if (p_usecs_) {
          const char* p_seqd_to_conf_secs_ = strtok(NULL, " \t\n");
          if (p_seqd_to_conf_secs_) {
            const char* p_seqd_to_conf_usecs_ = strtok(NULL, " \t\n");

            if (p_seqd_to_conf_usecs_) {
              const int secs_ = atoi(p_secs_);
              const int usecs_ = atoi(p_usecs_);
              const int seqd_to_conf_secs_ = atoi(p_seqd_to_conf_secs_);
              const int seqd_to_conf_usecs_ = atoi(p_seqd_to_conf_usecs_);  // using half the seqd-conf

              HFSAT::ttime_t t_time_ = HFSAT::ttime_t(secs_, usecs_);
              HFSAT::ttime_t t_seqd_to_conf_ = HFSAT::ttime_t(seqd_to_conf_secs_, seqd_to_conf_usecs_);

              time_to_seqd_to_conf_time_[t_time_] = t_seqd_to_conf_;
            }
          }
        }
      }
    }

    precomputed_seqd_to_conf_file_.close();

    return time_to_seqd_to_conf_time_;
  }

  static std::map<HFSAT::ttime_t, HFSAT::ttime_t> GetPrecomputedConfMarketDelay(const std::string shortcode_,
                                                                                const int tradingdate_) {
    std::string precomputed_conf_market_delay_filename_ = "";
    {
      std::ostringstream t_oss_;
      t_oss_ << PRECOMPUTED_ACCURATE_MARKETDELAY_LOCATION << "/" << shortcode_ << "_" << tradingdate_;

      precomputed_conf_market_delay_filename_ = t_oss_.str();
    }

    std::ifstream precomputed_conf_market_delay_file_;

    precomputed_conf_market_delay_file_.open(precomputed_conf_market_delay_filename_.c_str(), std::ifstream::in);

    if (!precomputed_conf_market_delay_file_.is_open()) {
      std::cerr << " Could not open precomputed_conf_market_delay_file_=" << precomputed_conf_market_delay_filename_
                << std::endl;
      return std::map<HFSAT::ttime_t, HFSAT::ttime_t>();
    }

    char line_[100];
    memset(line_, 0, sizeof(line_));
    std::map<HFSAT::ttime_t, HFSAT::ttime_t> time_to_conf_market_delay_time_;

    while (precomputed_conf_market_delay_file_.getline(line_, sizeof(line_))) {
      const char* p_secs_ = strtok(line_, " \t\n");
      if (p_secs_) {
        const char* p_usecs_ = strtok(NULL, " \t\n");
        if (p_usecs_) {
          const char* p_conf_market_delay_secs_ = strtok(NULL, " \t\n");
          if (p_conf_market_delay_secs_) {
            const char* p_conf_market_delay_usecs_ = strtok(NULL, " \t\n");

            if (p_conf_market_delay_usecs_) {
              const int secs_ = atoi(p_secs_);
              const int usecs_ = atoi(p_usecs_);
              const int conf_market_delay_secs_ = atoi(p_conf_market_delay_secs_);
              const int conf_market_delay_usecs_ = atoi(p_conf_market_delay_usecs_);

              HFSAT::ttime_t t_time_ = HFSAT::ttime_t(secs_, usecs_);
              HFSAT::ttime_t t_conf_market_delay_ = HFSAT::ttime_t(conf_market_delay_secs_, conf_market_delay_usecs_);

              time_to_conf_market_delay_time_[t_time_] = t_conf_market_delay_;
            }
          }
        }
      }
    }

    precomputed_conf_market_delay_file_.close();

    return time_to_conf_market_delay_time_;
  }

  static std::map<HFSAT::ttime_t, HFSAT::ttime_t> GetPrecomputedCxlMarketDelay(const std::string shortcode_,
                                                                               const int tradingdate_) {
    std::string precomputed_cxl_market_delay_filename_ = "";
    {
      std::ostringstream t_oss_;
      t_oss_ << PRECOMPUTED_ACCURATE_CXL_MARKETDELAY_LOCATION << "/" << shortcode_ << "_" << tradingdate_ << "_CXL";

      precomputed_cxl_market_delay_filename_ = t_oss_.str();
    }

    std::ifstream precomputed_cxl_market_delay_file_;

    precomputed_cxl_market_delay_file_.open(precomputed_cxl_market_delay_filename_.c_str(), std::ifstream::in);

    if (!precomputed_cxl_market_delay_file_.is_open()) {
      std::cerr << " Could not open precomputed_cxl_market_delay_file_=" << precomputed_cxl_market_delay_filename_
                << std::endl;
      return std::map<HFSAT::ttime_t, HFSAT::ttime_t>();
    }

    char line_[100];
    memset(line_, 0, sizeof(line_));
    std::map<HFSAT::ttime_t, HFSAT::ttime_t> time_to_cxl_market_delay_time_;

    while (precomputed_cxl_market_delay_file_.getline(line_, sizeof(line_))) {
      const char* p_secs_ = strtok(line_, " \t\n");
      if (p_secs_) {
        const char* p_usecs_ = strtok(NULL, " \t\n");
        if (p_usecs_) {
          const char* p_cxl_market_delay_secs_ = strtok(NULL, " \t\n");
          if (p_cxl_market_delay_secs_) {
            const char* p_cxl_market_delay_usecs_ = strtok(NULL, " \t\n");

            if (p_cxl_market_delay_usecs_) {
              const int secs_ = atoi(p_secs_);
              const int usecs_ = atoi(p_usecs_);
              const int cxl_market_delay_secs_ = atoi(p_cxl_market_delay_secs_);
              const int cxl_market_delay_usecs_ = atoi(p_cxl_market_delay_usecs_);

              HFSAT::ttime_t t_time_ = HFSAT::ttime_t(secs_, usecs_);
              HFSAT::ttime_t t_cxl_market_delay_ = HFSAT::ttime_t(cxl_market_delay_secs_, cxl_market_delay_usecs_);

              time_to_cxl_market_delay_time_[t_time_] = t_cxl_market_delay_;
            }
          }
        }
      }
    }

    precomputed_cxl_market_delay_file_.close();

    return time_to_cxl_market_delay_time_;
  }

  static void WritePrecomputedSeqdToConf(const std::string shortcode_, const int tradingdate_,
                                         std::map<HFSAT::ttime_t, HFSAT::ttime_t>& time_to_seqd_to_conf_times_) {
    std::string precomputed_seqd_to_conf_filename_ = "";
    {
      std::ostringstream t_oss_;
      t_oss_ << PRECOMPUTED_SEQD_TO_CONF_LOCATION << "/" << shortcode_ << "_" << tradingdate_;

      precomputed_seqd_to_conf_filename_ = t_oss_.str();
    }

    FileUtils::MkdirEnclosing(precomputed_seqd_to_conf_filename_);

    std::ofstream precomputed_seqd_to_conf_file_;
    precomputed_seqd_to_conf_file_.open(precomputed_seqd_to_conf_filename_.c_str(), std::ifstream::out);

    if (!precomputed_seqd_to_conf_file_.is_open()) {
      std::cerr << " Could not create precomputed_seqd_to_conf_file_=" << precomputed_seqd_to_conf_filename_
                << std::endl;
      return;
    }

    for (std::map<HFSAT::ttime_t, HFSAT::ttime_t>::iterator _itr_ = time_to_seqd_to_conf_times_.begin();
         _itr_ != time_to_seqd_to_conf_times_.end(); ++_itr_) {
      precomputed_seqd_to_conf_file_ << _itr_->first.tv_sec << " " << _itr_->first.tv_usec << " "
                                     << _itr_->second.tv_sec << " " << _itr_->second.tv_usec << "\n";
    }

    precomputed_seqd_to_conf_file_.close();
  }

  static void WritePrecomputedConfMarketDelay(
      const std::string shortcode_, const int tradingdate_,
      std::map<HFSAT::ttime_t, HFSAT::ttime_t>& time_to_conf_market_delay_time_) {
    std::string precomputed_conf_market_delay_filename_ = "";
    {
      std::ostringstream t_oss_;
      t_oss_ << PRECOMPUTED_ACCURATE_MARKETDELAY_LOCATION << "/" << shortcode_ << "_" << tradingdate_;

      precomputed_conf_market_delay_filename_ = t_oss_.str();
    }

    FileUtils::MkdirEnclosing(precomputed_conf_market_delay_filename_);

    std::ofstream precomputed_conf_market_delay_file_;
    precomputed_conf_market_delay_file_.open(precomputed_conf_market_delay_filename_.c_str(), std::ifstream::out);

    if (!precomputed_conf_market_delay_file_.is_open()) {
      std::cerr << " Could not create precomputed_conf_market_delay_file_=" << precomputed_conf_market_delay_filename_
                << std::endl;
      return;
    }

    for (std::map<HFSAT::ttime_t, HFSAT::ttime_t>::iterator _itr_ = time_to_conf_market_delay_time_.begin();
         _itr_ != time_to_conf_market_delay_time_.end(); ++_itr_) {
      precomputed_conf_market_delay_file_ << _itr_->first.tv_sec << " " << _itr_->first.tv_usec << " "
                                          << _itr_->second.tv_sec << " " << _itr_->second.tv_usec << "\n";
    }

    precomputed_conf_market_delay_file_.close();
  }

  static void WritePrecomputedCxlMarketDelay(const std::string shortcode_, const int tradingdate_,
                                             std::map<HFSAT::ttime_t, HFSAT::ttime_t>& time_to_cxl_market_delay_time_) {
    std::string precomputed_cxl_market_delay_filename_ = "";
    {
      std::ostringstream t_oss_;
      t_oss_ << PRECOMPUTED_ACCURATE_CXL_MARKETDELAY_LOCATION << "/" << shortcode_ << "_" << tradingdate_ << "_CXL";

      precomputed_cxl_market_delay_filename_ = t_oss_.str();
    }

    FileUtils::MkdirEnclosing(precomputed_cxl_market_delay_filename_);

    std::ofstream precomputed_cxl_market_delay_file_;
    precomputed_cxl_market_delay_file_.open(precomputed_cxl_market_delay_filename_.c_str(), std::ifstream::out);

    if (!precomputed_cxl_market_delay_file_.is_open()) {
      std::cerr << " Could not create precomputed_cxl_market_delay_file_=" << precomputed_cxl_market_delay_filename_
                << std::endl;
      return;
    }

    for (std::map<HFSAT::ttime_t, HFSAT::ttime_t>::iterator _itr_ = time_to_cxl_market_delay_time_.begin();
         _itr_ != time_to_cxl_market_delay_time_.end(); ++_itr_) {
      precomputed_cxl_market_delay_file_ << _itr_->first.tv_sec << " " << _itr_->first.tv_usec << " "
                                         << _itr_->second.tv_sec << " " << _itr_->second.tv_usec << "\n";
    }

    precomputed_cxl_market_delay_file_.close();
  }

  static std::map<HFSAT::ttime_t, HFSAT::ttime_t> GetSeqdToConfTimes(HFSAT::DebugLogger& dbglogger_,
                                                                     HFSAT::SecurityNameIndexer& sec_name_indexer_,
                                                                     const int tradingdate_,
                                                                     const unsigned dep_security_id_,
                                                                     HFSAT::TradingLocation_t dep_trading_location_,
                                                                     bool print_verification_info_ = false) {
    if (IsPrecomputedSeqdToConfAvailable(sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_)) {
      std::map<HFSAT::ttime_t, HFSAT::ttime_t> time_to_seqd_to_conf_times_ =
          GetPrecomputedSeqdToConf(sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_);

      if (print_verification_info_) {
        for (std::map<HFSAT::ttime_t, HFSAT::ttime_t>::iterator _itr_ = time_to_seqd_to_conf_times_.begin();
             _itr_ != time_to_seqd_to_conf_times_.end(); ++_itr_) {
          dbglogger_ << "GetSeqdToConfTimes " << _itr_->first << " " << _itr_->second << DBGLOG_ENDL_FLUSH;
        }
      }

      return time_to_seqd_to_conf_times_;
    }

    HFSAT::HistoricalDispatcher historical_dispatcher_;

    HFSAT::ORSMessageFileSource* p_ors_message_file_source_ = new HFSAT::ORSMessageFileSource(
        dbglogger_, sec_name_indexer_, tradingdate_, dep_security_id_,
        sec_name_indexer_.GetSecurityNameFromId(dep_security_id_), dep_trading_location_);

    HFSAT::Watch watch_(dbglogger_, tradingdate_);

    p_ors_message_file_source_->SetExternalTimeListener(&watch_);

    historical_dispatcher_.AddExternalDataListener(p_ors_message_file_source_);

    HFSAT::ORSMessageStatsComputer* p_ors_message_stats_computer_;

    p_ors_message_stats_computer_ = new HFSAT::ORSMessageStatsComputer(dbglogger_, watch_);

    p_ors_message_stats_computer_->ComputeSeqdToConf(true);

    p_ors_message_file_source_->AddOrderSequencedListener(p_ors_message_stats_computer_);
    p_ors_message_file_source_->AddOrderConfirmedListener(p_ors_message_stats_computer_);

    try {
      historical_dispatcher_.RunHist();
    } catch (int e) {
    }

    std::map<HFSAT::ttime_t, HFSAT::ttime_t> time_to_seqd_to_conf_times_ =
        std::map<HFSAT::ttime_t, HFSAT::ttime_t>(p_ors_message_stats_computer_->TimeToSeqdToConfTime());

    if (print_verification_info_) {
      for (std::map<HFSAT::ttime_t, HFSAT::ttime_t>::iterator _itr_ = time_to_seqd_to_conf_times_.begin();
           _itr_ != time_to_seqd_to_conf_times_.end(); ++_itr_) {
        dbglogger_ << "GetSeqdToConfTimes " << _itr_->first << " " << _itr_->second << DBGLOG_ENDL_FLUSH;
      }
    }

    if (!IsPrecomputedSeqdToConfAvailable(sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_)) {
      WritePrecomputedSeqdToConf(sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_,
                                 time_to_seqd_to_conf_times_);
    }

    return time_to_seqd_to_conf_times_;
  }

  static std::map<HFSAT::ttime_t, HFSAT::ttime_t> GetConfToMarketDelayTimes(
      HFSAT::DebugLogger& dbglogger_, HFSAT::SecurityNameIndexer& sec_name_indexer_, const int tradingdate_,
      const unsigned dep_security_id_, HFSAT::TradingLocation_t dep_trading_location_,
      bool print_verification_info_ = false) {
    if (IsPrecomputedConfMarketDelayAvailable(sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_)) {
      std::map<HFSAT::ttime_t, HFSAT::ttime_t> time_to_conf_market_delay_time_ =
          GetPrecomputedConfMarketDelay(sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_);

      if (print_verification_info_) {
        for (std::map<HFSAT::ttime_t, HFSAT::ttime_t>::iterator _itr_ = time_to_conf_market_delay_time_.begin();
             _itr_ != time_to_conf_market_delay_time_.end(); ++_itr_) {
          dbglogger_ << "GetConfToMarketDelayTimes" << _itr_->first << " " << _itr_->second << DBGLOG_ENDL_FLUSH;
        }
      }

      return time_to_conf_market_delay_time_;
    }

    HFSAT::HistoricalDispatcher historical_dispatcher_;

    HFSAT::ORSMessageFileSource* p_ors_message_file_source_ = new HFSAT::ORSMessageFileSource(
        dbglogger_, sec_name_indexer_, tradingdate_, dep_security_id_,
        sec_name_indexer_.GetSecurityNameFromId(dep_security_id_), dep_trading_location_);

    HFSAT::Watch watch_(dbglogger_, tradingdate_);

    p_ors_message_file_source_->SetExternalTimeListener(&watch_);

    historical_dispatcher_.AddExternalDataListener(p_ors_message_file_source_);

    HFSAT::ORSMessageStatsComputer* p_ors_message_stats_computer_;

    p_ors_message_stats_computer_ = new HFSAT::ORSMessageStatsComputer(dbglogger_, watch_);

    //      std::cout << "Get Conf To Market Delay \n" ;
    // accurate conf to market update delay
    p_ors_message_stats_computer_->ComputeAccurateConfMarketUpdate(
        sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_);

    p_ors_message_file_source_->AddOrderSequencedListener(p_ors_message_stats_computer_);
    p_ors_message_file_source_->AddOrderConfirmedListener(p_ors_message_stats_computer_);

    try {
      historical_dispatcher_.RunHist();
    } catch (int e) {
    }

    std::map<HFSAT::ttime_t, HFSAT::ttime_t> time_to_conf_market_delay_time_ =
        std::map<HFSAT::ttime_t, HFSAT::ttime_t>(p_ors_message_stats_computer_->TimeToConfToMarketDelayTime());

    if (print_verification_info_) {
      for (std::map<HFSAT::ttime_t, HFSAT::ttime_t>::iterator _itr_ = time_to_conf_market_delay_time_.begin();
           _itr_ != time_to_conf_market_delay_time_.end(); ++_itr_) {
        dbglogger_ << "GetConfToMarketDelayTimes" << _itr_->first << " " << _itr_->second << DBGLOG_ENDL_FLUSH;
      }
    }

    if (!IsPrecomputedConfMarketDelayAvailable(sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_)) {
      WritePrecomputedConfMarketDelay(sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_,
                                      time_to_conf_market_delay_time_);
    }

    return time_to_conf_market_delay_time_;
  }

  static std::map<HFSAT::ttime_t, HFSAT::ttime_t> GetCxlToMarketDelayTimes(
      HFSAT::DebugLogger& dbglogger_, HFSAT::SecurityNameIndexer& sec_name_indexer_, const int tradingdate_,
      const unsigned dep_security_id_, HFSAT::TradingLocation_t dep_trading_location_,
      bool print_verification_info_ = false) {
    if (IsPrecomputedCxlMarketDelayAvailable(sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_)) {
      std::map<HFSAT::ttime_t, HFSAT::ttime_t> time_to_cxl_market_delay_time_ =
          GetPrecomputedCxlMarketDelay(sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_);

      if (print_verification_info_) {
        for (std::map<HFSAT::ttime_t, HFSAT::ttime_t>::iterator _itr_ = time_to_cxl_market_delay_time_.begin();
             _itr_ != time_to_cxl_market_delay_time_.end(); ++_itr_) {
          dbglogger_ << "GetCxlToMarketDelayTimes" << _itr_->first << " " << _itr_->second << DBGLOG_ENDL_FLUSH;
        }
      }

      return time_to_cxl_market_delay_time_;
    }

    HFSAT::HistoricalDispatcher historical_dispatcher_;

    HFSAT::ORSMessageFileSource* p_ors_message_file_source_ = new HFSAT::ORSMessageFileSource(
        dbglogger_, sec_name_indexer_, tradingdate_, dep_security_id_,
        sec_name_indexer_.GetSecurityNameFromId(dep_security_id_), dep_trading_location_);

    HFSAT::Watch watch_(dbglogger_, tradingdate_);

    p_ors_message_file_source_->SetExternalTimeListener(&watch_);

    historical_dispatcher_.AddExternalDataListener(p_ors_message_file_source_);

    HFSAT::ORSMessageStatsComputer* p_ors_message_stats_computer_;

    p_ors_message_stats_computer_ = new HFSAT::ORSMessageStatsComputer(dbglogger_, watch_);

    //      std::cout << "Get Conf To Market Delay \n" ;
    // accurate conf to market update delay
    p_ors_message_stats_computer_->ComputeAccurateCxlMarketUpdate(
        sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_);

    p_ors_message_file_source_->AddOrderSequencedListener(p_ors_message_stats_computer_);
    p_ors_message_file_source_->AddOrderCanceledListener(p_ors_message_stats_computer_);

    try {
      historical_dispatcher_.RunHist();
    } catch (int e) {
    }

    std::map<HFSAT::ttime_t, HFSAT::ttime_t> time_to_cxl_market_delay_time_ =
        std::map<HFSAT::ttime_t, HFSAT::ttime_t>(p_ors_message_stats_computer_->TimeToCxlToMarketDelayTime());

    if (print_verification_info_) {
      for (std::map<HFSAT::ttime_t, HFSAT::ttime_t>::iterator _itr_ = time_to_cxl_market_delay_time_.begin();
           _itr_ != time_to_cxl_market_delay_time_.end(); ++_itr_) {
        dbglogger_ << "GetConfToMarketDelayTimes" << _itr_->first << " " << _itr_->second << DBGLOG_ENDL_FLUSH;
      }
    }

    if (!IsPrecomputedCxlMarketDelayAvailable(sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_)) {
      WritePrecomputedCxlMarketDelay(sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_,
                                     time_to_cxl_market_delay_time_);
    }

    return time_to_cxl_market_delay_time_;
  }

  static std::map<HFSAT::ttime_t, HFSAT::ttime_t> GetCxlSeqdToConfTimes(HFSAT::DebugLogger& dbglogger_,
                                                                        HFSAT::SecurityNameIndexer& sec_name_indexer_,
                                                                        const int tradingdate_,
                                                                        const unsigned dep_security_id_,
                                                                        HFSAT::TradingLocation_t dep_trading_location_,
                                                                        bool print_verification_info_ = false) {
    HFSAT::UTILS::CxlConfStats cxl_conf_stats_(sec_name_indexer_.GetShortcodeFromId(dep_security_id_), tradingdate_,
                                               dbglogger_);

    std::map<HFSAT::ttime_t, HFSAT::ttime_t> time_to_cxl_seqd_to_conf_times_ =
        std::map<HFSAT::ttime_t, HFSAT::ttime_t>(cxl_conf_stats_.getCxlConfMap());

    if (print_verification_info_) {
      for (std::map<HFSAT::ttime_t, HFSAT::ttime_t>::iterator _itr_ = time_to_cxl_seqd_to_conf_times_.begin();
           _itr_ != time_to_cxl_seqd_to_conf_times_.end(); ++_itr_) {
        dbglogger_ << "GetCxlSeqdToConfTimes " << _itr_->first << " " << _itr_->second << DBGLOG_ENDL_FLUSH;
      }
    }

    return time_to_cxl_seqd_to_conf_times_;
  }

  static std::map<HFSAT::ttime_t, unsigned> GetTimeToNumRejections(HFSAT::DebugLogger& dbglogger_,
                                                                   HFSAT::SecurityNameIndexer& sec_name_indexer_,
                                                                   const int tradingdate_,
                                                                   const unsigned dep_security_id_,
                                                                   HFSAT::TradingLocation_t dep_trading_location_,
                                                                   bool print_verification_info_ = false) {
    HFSAT::HistoricalDispatcher historical_dispatcher_;

    HFSAT::ORSMessageFileSource* p_ors_message_file_source_ = new HFSAT::ORSMessageFileSource(
        dbglogger_, sec_name_indexer_, tradingdate_, dep_security_id_,
        sec_name_indexer_.GetSecurityNameFromId(dep_security_id_), dep_trading_location_);

    HFSAT::Watch watch_(dbglogger_, tradingdate_);

    p_ors_message_file_source_->SetExternalTimeListener(&watch_);

    historical_dispatcher_.AddExternalDataListener(p_ors_message_file_source_);

    HFSAT::ORSMessageStatsComputer* p_ors_message_stats_computer_ =
        new HFSAT::ORSMessageStatsComputer(dbglogger_, watch_, print_verification_info_);
    p_ors_message_stats_computer_->ComputeRejectionPeriods(true);

    p_ors_message_file_source_->AddOrderRejectedListener(p_ors_message_stats_computer_);

    try {
      historical_dispatcher_.RunHist();
    } catch (int e) {
    }

    std::map<HFSAT::ttime_t, unsigned> time_to_num_rejections_ =
        std::map<HFSAT::ttime_t, unsigned>(p_ors_message_stats_computer_->TimeToNumRejections());

    if (print_verification_info_) {
      for (std::map<HFSAT::ttime_t, unsigned>::iterator _itr_ = time_to_num_rejections_.begin();
           _itr_ != time_to_num_rejections_.end(); ++_itr_) {
        dbglogger_ << "GetTimeToNumRejections " << _itr_->first << " " << _itr_->second << DBGLOG_ENDL_FLUSH;
      }
    }

    return time_to_num_rejections_;
  }

  static std::map<HFSAT::ttime_t, unsigned> GetTimeToNumCxlRejections(HFSAT::DebugLogger& dbglogger_,
                                                                      HFSAT::SecurityNameIndexer& sec_name_indexer_,
                                                                      const int tradingdate_,
                                                                      const unsigned dep_security_id_,
                                                                      HFSAT::TradingLocation_t dep_trading_location_,
                                                                      bool print_verification_info_ = false) {
    HFSAT::HistoricalDispatcher historical_dispatcher_;

    HFSAT::ORSMessageFileSource* p_ors_message_file_source_ = new HFSAT::ORSMessageFileSource(
        dbglogger_, sec_name_indexer_, tradingdate_, dep_security_id_,
        sec_name_indexer_.GetSecurityNameFromId(dep_security_id_), dep_trading_location_);

    HFSAT::Watch watch_(dbglogger_, tradingdate_);

    p_ors_message_file_source_->SetExternalTimeListener(&watch_);

    historical_dispatcher_.AddExternalDataListener(p_ors_message_file_source_);

    HFSAT::ORSMessageStatsComputer* p_ors_message_stats_computer_ =
        new HFSAT::ORSMessageStatsComputer(dbglogger_, watch_, print_verification_info_);
    p_ors_message_stats_computer_->ComputeCxlRejectionPeriods(true);

    p_ors_message_file_source_->AddOrderRejectedListener(p_ors_message_stats_computer_);

    try {
      historical_dispatcher_.RunHist();
    } catch (int e) {
    }

    std::map<HFSAT::ttime_t, unsigned> time_to_num_cxl_rejections_ =
        std::map<HFSAT::ttime_t, unsigned>(p_ors_message_stats_computer_->TimeToNumCxlRejections());

    if (print_verification_info_) {
      for (std::map<HFSAT::ttime_t, unsigned>::iterator _itr_ = time_to_num_cxl_rejections_.begin();
           _itr_ != time_to_num_cxl_rejections_.end(); ++_itr_) {
        dbglogger_ << "GetTimeToNumCxlRejections " << _itr_->first << " " << _itr_->second << DBGLOG_ENDL_FLUSH;
      }
    }

    return time_to_num_cxl_rejections_;
  }
};
}

#endif  // BASE_ORSMESSAGES_ORS_MESSAGES_STATS_COMPUTER_H
