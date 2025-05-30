/**
   \file OrderRouting/base_order_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASIC_ORDERROUTING_BASE_ORDER_MANAGER_H
#define BASIC_ORDERROUTING_BASE_ORDER_MANAGER_H

#include <map>
#include <vector>
#include <math.h>
#include <set>
#include <regex>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CDef/ors_defines.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/lockfree_simple_mempool.hpp"

#include "dvccode/ORSMessages/ors_message_listener.hpp"
#include "dvccode/Utils/rdtsc_timer.hpp"

#include "dvccode/CommonTradeUtils/watch.hpp"

#include "baseinfra/OrderRouting/base_order.hpp"
#include "baseinfra/OrderRouting/base_trader.hpp"
#include "baseinfra/SmartOrderRouting/base_pnl.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"

#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "dvccode/CommonTradeUtils/throttle_manager.hpp"

#include "baseinfra/BaseUtils/query_trading_auto_freeze_manager.hpp"
#include "dvccode/Utils/tinyexpr.hpp"

#define CXL_WAIT_MSECS 500
#define MODIFY_WAIT_MSECS 500

// Makes no sense to do this in SIM , the large UTSs throw off the calibration.
#define ENABLE_AGG_TRADING_COOLOFF true
#define ENABLE_AGG_TRADING_COOLOFF_ORDER_DISABLE false
#define EMAIL_ON_AGG_COOLOFF false
#define AGG_TIME_FRAME_MSECS 1000
#define AGG_SIZE_THRESHOLD 5
#define RETAIL_SAOS 799997
//#define ORDER_MANAGER_INT_PRICE_RANGE 2048

#define THROTTLE_COUNT_WINDOW_MSECS 60000
#define TIME_TO_CHECK_FOR_DROPS 30000  // Every 30 seconds
#define NUM_OPEN_ORDERS_MODIFY_ALLOWED 5
#define NUM_EXCH_REJ_FOR_CONS_MODIFY_ALLOWED 5
#define DOUBLE_PRECISION 0.000001

namespace HFSAT {

typedef std::map<int, BaseOrder*> BASE_ORDER_MAP;

/// Main class that listens to ORS reply messages either from ORS ( live trading
/// ) or SMM ( sim trading )
class BasicOrderManager : public OrderNotFoundListener,
                          public OrderSequencedListener,
                          public OrderConfirmedListener,
                          public OrderConfCxlReplaceRejectListener,
                          public OrderConfCxlReplacedListener,
                          public OrderCanceledListener,
                          public OrderExecutedListener,
                          public OrderRejectedListener,
                          public TimePeriodListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  SecurityNameIndexer& sec_name_indexer_;
  BaseTrader& base_trader_;
  const std::string dep_shortcode_;
  const unsigned int dep_security_id_;
  const char* dep_symbol_;
  int max_size_multiplier_;
  int config_max_long_pos_;
  int config_max_short_pos_;
  int config_max_long_exposure_;
  int config_max_short_exposure_;
  int max_long_pos_;
  int max_short_pos_;
  int neg_max_short_pos_;
  int max_long_exposure_;
  int max_short_exposure_;
  int neg_max_short_exposure_;
  double size_multiplier_;
  int client_assigned_order_sequence_;

  LockFreeSimpleMempool<BaseOrder> baseorder_mempool_;

  std::map<int, BaseOrder*> bid_order_id_to_order_map_;
  std::map<int, BaseOrder*> ask_order_id_to_order_map_;

  bool external_cancel_all_outstanding_orders_;

  std::vector<ExecutionListener*> execution_listener_vec_;
  std::vector<OrderChangeListener*> order_change_listener_vec_;
  std::vector<OrderRejectListener*> order_reject_listener_vec_;
  std::vector<ExchangeRejectsListener*> exch_rejects_listener_vec_;
  std::vector<ThrottleNumberListener*> throttle_num_listener_vec_;
  std::vector<MarginListener*> margin_listener_vec_;

  int client_position_;
  int global_position_;
  int position_offset_;
  int bid_open_size_;
  int ask_open_size_;
  int trade_volume_;

  int32_t number_of_consecutive_exchange_rejects_;
  int32_t last_reject_evaluation_time_;
  int32_t time_based_total_exchange_rejects_;
  int32_t last_throttle_evaluation_time_;
  int32_t time_based_total_throttles_;
  int32_t total_exchange_rejects_;
  int32_t reject_based_freeze_timer_;
  HFSAT::BaseUtils::QueryTradingAutoFreezeManager& query_trading_auto_freeze_manager_;
  bool is_auto_freeze_active_;
  bool is_magin_breach_risk_checks_hit_;
  bool is_risk_checks_hit_;  // Whether risk checks have been breached
  bool is_modify_before_confirmation;
  bool is_cancellable_before_confirmation;
  BaseOrder* p_next_new_order_;
  uint32_t DEP_ONE_LOT_SIZE_;
  bool is_bmf_;
  bool is_dataentry_freeze;
  uint32_t exchange_margin_freeze_threshold_;
  uint32_t exchange_margin_release_threshold_;
  uint64_t min_throttle_wait_cycle;
  uint32_t num_new_order_;
  uint32_t num_cxl_order_;
  uint32_t num_modify_order_;

 public:
  const int server_assigned_client_id_;
  bool livetrading_;

  BasicOrderManager(DebugLogger& _dbglogger_, const Watch& _watch_, SecurityNameIndexer& _sec_name_indexer_,
                    BaseTrader& _base_trader_, SecurityMarketView& t_dep_market_view_,
                    int _first_client_assigned_order_sequence_, std::map<std::string, std::string>& pos_key_val_map,
                    bool _livetrading_, bool _is_modify_before_confirmation, bool _is_cancellable_before_confirmation);

  virtual ~BasicOrderManager() {}

  inline int trade_volume() const { return trade_volume_; }

  void ResetRejectsBasedFreeze() {
    NotifyResetByManualInterventionOverRejects();

    number_of_consecutive_exchange_rejects_ = 0;
    time_based_total_exchange_rejects_ = 0;
    last_reject_evaluation_time_ = watch_.msecs_from_midnight();
    total_exchange_rejects_ = 0;
    reject_based_freeze_timer_ = 0;
  }

  void ResetRiskBasedFreeze() { is_risk_checks_hit_ = false;}
  void ResetMarginBasedFreeze() { is_magin_breach_risk_checks_hit_= false;}
  void OrderDataEntryFreezeDisable();

  void OnTimePeriodUpdate(const int num_pages_to_add);
  void AddPosition(int exec_quantity, const double price);

  void SetPrevDayPosition(int _position_offset_, double _price_) { 
    position_offset_ = _position_offset_;
    double inv_tick_size_ = 1.0/SecurityDefinitions::GetContractMinPriceIncrement(dep_shortcode_, watch_.YYYYMMDD());
    NotifyExecAndPosListeners(_position_offset_,_price_,int(_price_ * inv_tick_size_ + DOUBLE_PRECISION),0);
  }

  inline void SetBasePNL(BasePNL* t_p_base_pnl_) { AddExecutionListener(t_p_base_pnl_); }

  bool SetSizeMultiplier(double _size_multiplier_) {
    size_multiplier_ = _size_multiplier_;
    if (_size_multiplier_ > max_size_multiplier_) {
      dbglogger_ << watch_.tv() << " ERROR: Requested size multiplier " << _size_multiplier_
                 << " greater than OM limit " << max_size_multiplier_ << " for SYM: " << dep_shortcode_
                 << DBGLOG_ENDL_FLUSH;
      return false;
    } else {
      ChangeSize();
      return true;
    }
  }

  void ChangeSize() {
    max_long_pos_ = int(config_max_long_pos_ * size_multiplier_);
    max_short_pos_ = int(config_max_short_pos_ * size_multiplier_);
    neg_max_short_pos_ = -1 * max_short_pos_;
    max_long_exposure_ = int(config_max_long_exposure_ * size_multiplier_);
    max_short_exposure_ = int(config_max_short_exposure_ * size_multiplier_);
    neg_max_short_exposure_ = -1 * max_short_exposure_;
  }

  //must be an int as positions can be -ve 
  inline int32_t GetRegexValue(std::string _value_){

    std::ostringstream t_oss;
    t_oss << DEP_ONE_LOT_SIZE_;

    std::string ip_regex = std::regex_replace(_value_,std::regex(_DEP_ONE_LOT_SIZE_),t_oss.str().c_str());
    return te_interp(ip_regex.c_str(), 0);

  }

  void LoadPositionCheck(std::map<std::string, std::string>& pos_key_val_map) {

    std::string value = "";

    int max_exposure = 0;
    long close_price = 1;
    int position_tmp = 0;
    std::map<std::string, std::string>::iterator max_exposure_iter = pos_key_val_map.find("MAX_ALLOWED_EXPOSURE");
    if (max_exposure_iter != pos_key_val_map.end()){
      max_exposure = std::stoi(max_exposure_iter->second);
      close_price = std::max(1.0,HFSAT::NSESecurityDefinitions::GetClosePriceFromShortCode(dep_shortcode_));
      position_tmp = max_exposure/close_price;
    }

    std::map<std::string, std::string>::iterator iter = pos_key_val_map.find(dep_shortcode_ + "_MAXLONGPOS");
    if (iter != pos_key_val_map.end()) {

      value = std::string(iter->second);

      //override initial value with regex if there is a regex
      if(value.find(_DEP_ONE_LOT_SIZE_) != std::string::npos){
        config_max_long_pos_ = GetRegexValue(value);
      }else{
        //treat this as default to avoid branching into else 
        config_max_long_pos_ = std::stoi(iter->second);
        if ((max_exposure > 0) && (position_tmp < config_max_long_pos_))
            config_max_long_pos_ = position_tmp;
      }

    }

    iter = pos_key_val_map.find(dep_shortcode_ + "_MAXSHORTPOS");
    if (iter != pos_key_val_map.end()) {

      value = std::string(iter->second);

      //override initial value with regex if there is a regex
      if(value.find(_DEP_ONE_LOT_SIZE_) != std::string::npos){
        config_max_short_pos_ = GetRegexValue(value);
      }else{
        //treat this as default to avoid branching into else
        config_max_short_pos_ = std::stoi(iter->second);
        if ((max_exposure > 0) && (position_tmp < config_max_short_pos_)){
          dbglogger_ << dep_shortcode_ << " CROSSING EXPOSURE LIMIT " << config_max_short_pos_ \
           << " NEW POSLIMIT: " << position_tmp << DBGLOG_ENDL_FLUSH;
          config_max_short_pos_ = position_tmp;
        }
      }
    }

    iter = pos_key_val_map.find(dep_shortcode_ + "_MAXLONGEXPOSURE");
    if (iter != pos_key_val_map.end()) {

      value = std::string(iter->second);

      //override initial value with regex if there is a regex
      if(value.find(_DEP_ONE_LOT_SIZE_) != std::string::npos){
        config_max_long_exposure_ = GetRegexValue(value);
      }else{
        //treat this as default to avoid branching into else
        config_max_long_exposure_ = std::stoi(iter->second);
        if ((max_exposure > 0) && (position_tmp < config_max_long_pos_))
          config_max_long_exposure_ = 2*position_tmp;
      }
    }

    iter = pos_key_val_map.find(dep_shortcode_ + "_MAXSHORTEXPOSURE");
    if (iter != pos_key_val_map.end()) {

      value = std::string(iter->second);

      //override initial value with regex if there is a regex
      if(value.find(_DEP_ONE_LOT_SIZE_) != std::string::npos){
        config_max_short_exposure_ = GetRegexValue(value);
      }else{
        //treat this as default to avoid branching into else 
        config_max_short_exposure_ = std::stoi(iter->second);
        if ((max_exposure > 0) && (position_tmp < config_max_short_pos_))
          config_max_short_exposure_ = 2*position_tmp;
      }
    }

    iter = pos_key_val_map.find("MAX_SIZE_MULTIPLIER");
    if (iter != pos_key_val_map.end()) {
      max_size_multiplier_ = std::stoi(iter->second);
      if (max_size_multiplier_ > 5) {
        dbglogger_ << watch_.tv() << " ERROR: SIZE MULTIPLIER THRESHOLD BREACHED " << max_size_multiplier_
                   << "instead of 5 " << DBGLOG_ENDL_FLUSH;
        max_size_multiplier_ = 5;
      }
    }
    ChangeSize();
    iter = pos_key_val_map.find("MARGIN_FREEZE_THRESHOLD");
    if (iter != pos_key_val_map.end()) {
      exchange_margin_freeze_threshold_ = std::stoi(iter->second);
    }
    iter = pos_key_val_map.find("MARGIN_RELEASE_THRESHOLD");
    if (iter != pos_key_val_map.end()) {
      exchange_margin_release_threshold_ = std::min((int)(exchange_margin_freeze_threshold_-5),std::stoi(iter->second));  // Margin release threshold should be atlease 5% less than freeze threshold
    }
  }

  BaseOrder* SendTrade(const double price, const int intpx, int size_requested, TradeType_t buysell,
                       char placed_at_level_indicator = 'B', OrderType_t order_type = kOrderDay,
                       int _executioner_id_ = -1, int mirror_factor = 1, bool _is_reserved_type_ = false,
                       double disclosed_qty_factor_ = 1.0);

  inline bool Cancel(BaseOrder& t_this_order_, bool _is_reserved_type_ = false) {
    uint64_t this_timestamp = GetReadTimeStampCounter();
    if (this_timestamp < min_throttle_wait_cycle && !_is_reserved_type_ && !t_this_order_.is_ioc_ ){
	return false;
    }
    if ((((t_this_order_.order_status_ == kORRType_Conf) &&
          (!t_this_order_.num_open_modified_orders_ || (is_cancellable_before_confirmation))) ||
         (t_this_order_.order_status_ == kORRType_Seqd && is_cancellable_before_confirmation)) &&
        !t_this_order_.canceled_ && !t_this_order_.is_ioc_) {
      t_this_order_.canceled_ = true;
      t_this_order_.seqd_msecs_ = watch_.msecs_from_midnight();
      t_this_order_.seqd_usecs_ = watch_.tv().tv_usec;
      t_this_order_.is_reserved_type_ = _is_reserved_type_;
      base_trader_.Cancel(t_this_order_);
      num_cxl_order_++;

      if (t_this_order_.modified_) {
        dbglogger_ << watch_.tv() << " USECASE: SENDING CANCEL OF MODIFY" << DBGLOG_ENDL_FLUSH;
      }

      if (t_this_order_.order_status_ == kORRType_Seqd) {
        dbglogger_ << watch_.tv() << " USECASE: SENDING CANCEL OF NEW" << DBGLOG_ENDL_FLUSH;
      }

#ifdef _DBGLOGGER_OM_INFO_
      if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
        dbglogger_ << watch_.tv() << " OM SENDING CANCEL " << dep_symbol_
                   << " CAOS: " << t_this_order_.client_assigned_order_sequence_
                   << " bs: " << ((t_this_order_.buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                   << " sz: " << t_this_order_.size_requested_ << " px: " << t_this_order_.price_
                   << " intpx: " << t_this_order_.int_price_ << DBGLOG_ENDL_FLUSH;
#ifdef _DBGLOGGER_SIM_ORDER_INFO_
        if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO))  // zero logging
        {
          dbglogger_ << "SYM: " << t_this_order_.security_name_ << " Px: " << t_this_order_.price()
                     << " INTPX: " << t_this_order_.int_price() << " BS: " << GetTradeTypeChar(t_this_order_.buysell())
                     << " ST: " << watch_.tv() << " DT: "
                     << watch_.tv() + ttime_t(0, t_this_order_.seqd_msecs_ * 1000 + t_this_order_.seqd_usecs_ % 1000) -
                            ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                     << " ORR: " << ToString(kORRType_CxlSeqd)
                     << " SAOS: " << t_this_order_.server_assigned_order_sequence()
                     << " CAOS: " << t_this_order_.client_assigned_order_sequence() << " CLTPOS: " << client_position_
                     << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << global_position_
                     << " SIZE: " << t_this_order_.size_remaining() << " SE: " << t_this_order_.size_executed()
                     << " MsgSeq : " << 0 << " Seq: " << 0 << DBGLOG_ENDL_FLUSH;
        }
#endif
      }
#endif
      return true;
    }
    return false;
  }

  bool Modify(BaseOrder* t_this_order_, double _new_price_, int _new_int_price_, int _new_size_,
              bool _is_reserved_type_ = false, double disclosed_qty_factor_ = 1.0);

  void CancelAllBidOrders();
  void CancelAllAskOrders();
  void CancelAllOrders();
  inline void UnsetCancelAllOrders() { external_cancel_all_outstanding_orders_ = false; }

  uint64_t SendOrderCount() {return num_new_order_;}
  uint64_t CxlOrderCount() {return num_cxl_order_;}
  uint64_t ModifyOrderCount() {return num_modify_order_;}

  void AddExecutionListener(ExecutionListener* p_this_listener_) {
    if (p_this_listener_ == NULL) return;
    VectorUtils::UniqueVectorAdd(execution_listener_vec_, p_this_listener_);
  }

  void RemoveExecutionLister(ExecutionListener* p_this_listener_) {
    HFSAT::VectorUtils::UniqueVectorRemove(execution_listener_vec_, p_this_listener_);
  }

  int AddOrderChangeListener(OrderChangeListener* p_this_listener_) {
    if (p_this_listener_ == NULL) return -1;
    VectorUtils::UniqueVectorAdd(order_change_listener_vec_, p_this_listener_);
    return order_change_listener_vec_.size() - 1;
  }

  void AddOrderRejectListener(OrderRejectListener* p_this_listener_, unsigned int _unique_id_) {
    if (p_this_listener_ == NULL) return;
    if (order_reject_listener_vec_.size() <= _unique_id_) {
      order_reject_listener_vec_.resize(_unique_id_ + 1);
    }
    order_reject_listener_vec_[_unique_id_] = p_this_listener_;
  }

  void AddExchangeRejectsListeners(ExchangeRejectsListener* p_new_listener) {
    if (p_new_listener == NULL) return;
    VectorUtils::UniqueVectorAdd(exch_rejects_listener_vec_, p_new_listener);
  }

  void AddThrottleNumberListeners(ThrottleNumberListener* p_new_listener) {
    if (p_new_listener == NULL) return;
    VectorUtils::UniqueVectorAdd(throttle_num_listener_vec_, p_new_listener);
  }

  void AddMarginListeners(MarginListener* p_new_listener) {
    if (p_new_listener == NULL) return;
    VectorUtils::UniqueVectorAdd(margin_listener_vec_, p_new_listener);
  }

  void LogFullStatus();

  std::string ShowOrders() const;

  void OrderNotFound(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                     const TradeType_t _buysell_, const int _int_price_, const int32_t server_assigned_message_sequence,
                     const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void OrderSequenced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                      const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                      const int _size_executed_, const int _client_position_, const int _global_position_,
                      const int r_int_price_, const int32_t server_assigned_message_sequence,
                      const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void OrderConfirmed(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                      const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                      const int _size_executed_, const int _client_position_, const int _global_position_,
                      const int _int_price_, const int32_t server_assigned_message_sequence,
                      const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  inline void OrderORSConfirmed(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                                const int _size_executed_, const int _int_price_,
                                const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                const ttime_t time_set_by_server) {
    return OrderConfirmed(_server_assigned_client_id_, _client_assigned_order_sequence_,
                          _server_assigned_order_sequence_, _security_id_, _price_, _buysell_, _size_remaining_,
                          _size_executed_, client_position_, global_position_, _int_price_,
                          server_assigned_message_sequence, exchange_order_id, time_set_by_server);
  }

  void OrderConfCxlReplaced(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                            const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                            const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                            const int _size_executed_, const int _client_position_, const int _global_position_,
                            const int _int_price_, const int32_t server_assigned_message_sequence,
                            const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void OrderConfCxlReplaceRejected(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                   const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                   const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                                   const int _client_position_, const int _global_position_, const int _int_price_,
                                   const int32_t rejection_reason, const int32_t server_assigned_message_sequence,
                                   const uint64_t throttle_wait_cycle, const ttime_t time_set_by_server);

  void OrderCanceled(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t _buysell_, const int _size_remaining_, const int _client_position_,
                     const int _global_position_, const int _int_price_, const int32_t server_assigned_message_sequence,
                     const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void OrderCancelRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                           const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                           const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                           const int _rejection_reason_, const int t_client_position_, const int t_global_position_,
                           const int r_int_price_, const uint64_t throttle_wait_cycle, const ttime_t time_set_by_server);

  void OrderExecuted(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t _buysell_, const int _size_remaining_, const int _size_executed_,
                     const int _client_position_, const int _global_position_, const int _int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const ttime_t time_set_by_server);

  void OrderRejected(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const unsigned int _security_id_, const double _price_, const TradeType_t _buysell_,
                     const int _size_remaining_, const int _rejection_reason_, const int _int_price_,
                     const uint64_t throttle_wait_cycle, const ttime_t time_set_by_server);

  void setMargin (double margin_);

  void UpdateAutoFreezeSystem(bool const& should_enforce_autofreeze) {
    is_auto_freeze_active_ = should_enforce_autofreeze;
    if ((0 != reject_based_freeze_timer_ || is_risk_checks_hit_) && false == should_enforce_autofreeze) {
      ResetRejectsBasedFreeze();
      ResetRiskBasedFreeze();
      ResetMarginBasedFreeze();
    }
  }

 protected:
  bool NotifyExecAndPosListeners(int _position_diff_, const double _trade_price_, const int _int_price_,
                                 const int _caos_);

  inline void NotifyOrderChangeListeners(HFSAT::BaseOrder* _order_) {
    int executioner_id_ = _order_->executioner_id_;
    if (executioner_id_ != -1 && (unsigned)executioner_id_ < order_change_listener_vec_.size()) {
      order_change_listener_vec_[executioner_id_]->OnOrderChange(_order_);
    } else {
      if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
        dbglogger_ << watch_.tv() << " Correct Executioner with id " << executioner_id_ << " not found "
                   << DBGLOG_ENDL_FLUSH;
      }
      // for (auto i = 0u; i < order_change_listener_vec_.size(); i++) {
      //   order_change_listener_vec_[i]->OnOrderChange();
      // }
    }
  }

  inline void NotifyOrderRejectListeners(HFSAT::BaseOrder* _order_) {
    int executioner_id_ = _order_->executioner_id_;
    if (executioner_id_ != -1 && (unsigned)executioner_id_ < order_reject_listener_vec_.size()) {
      order_reject_listener_vec_[executioner_id_]->OnOrderReject(_order_);
    } else {
      if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
        dbglogger_ << watch_.tv() << " Correct Executioner with id " << executioner_id_ << " not found "
                   << DBGLOG_ENDL_FLUSH;
      }
      // for (auto i = 0u; i < order_reject_listener_vec_.size(); i++) {
      //   order_reject_listener_vec_[i]->OnOrderReject();
      // }
    }
  }

  inline void NotifyExchangeRejectsListeners(HFSAT::BaseUtils::FreezeEnforcedReason const& freeze_reason) {
    if (0 == reject_based_freeze_timer_ && true == is_auto_freeze_active_) {
      for (uint32_t listener_counter = 0; listener_counter < exch_rejects_listener_vec_.size(); listener_counter++) {
        exch_rejects_listener_vec_[listener_counter]->OnGetFreezeDueToExchangeRejects(freeze_reason);
      }
    }
  }

  inline void NotifyResetByManualInterventionOverRejects() {
    // Conditional pass so we don't end up notifying multiple times
    if (0 != reject_based_freeze_timer_) {
      for (uint32_t listener_counter = 0; listener_counter < exch_rejects_listener_vec_.size(); listener_counter++) {
        exch_rejects_listener_vec_[listener_counter]->OnResetByManualInterventionOverRejects(HFSAT::BaseUtils::FreezeEnforcedReason::kAllow);
      }
    }
  }
};
}
#endif  // BASIC_ORDERROUTING_BASE_ORDER_MANAGER_H
