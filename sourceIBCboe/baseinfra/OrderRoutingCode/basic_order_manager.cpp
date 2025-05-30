/**
   \file OrderRoutingCode/basic_order_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include <string.h>

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"

#include "baseinfra/OrderRouting/basic_order_manager.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"

namespace HFSAT {

BasicOrderManager::BasicOrderManager(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                     SecurityNameIndexer& t_sec_name_indexer_, BaseTrader& _base_trader_,
                                     SecurityMarketView& t_dep_market_view_, int _first_client_assigned_order_sequence_,
                                     std::map<std::string, std::string>& pos_key_val_map, bool _livetrading_,
                                     bool _is_modify_before_confirmation, bool _is_cancellable_before_confirmation)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      sec_name_indexer_(t_sec_name_indexer_),
      base_trader_(_base_trader_),
      dep_shortcode_(t_dep_market_view_.shortcode()),
      dep_security_id_(t_dep_market_view_.security_id()),
      dep_symbol_(t_dep_market_view_.secname()),
      max_size_multiplier_(1),
      config_max_long_pos_(0),
      config_max_short_pos_(0),
      config_max_long_exposure_(0),
      config_max_short_exposure_(0),
      max_long_pos_(0),
      max_short_pos_(0),
      neg_max_short_pos_(0),
      max_long_exposure_(0),
      max_short_exposure_(0),
      neg_max_short_exposure_(0),
      size_multiplier_(1),
      client_assigned_order_sequence_(_first_client_assigned_order_sequence_),
      baseorder_mempool_(),
      bid_order_id_to_order_map_(),
      ask_order_id_to_order_map_(),
      external_cancel_all_outstanding_orders_(false),
      execution_listener_vec_(),
      order_change_listener_vec_(),
      client_position_(0),
      global_position_(0),
      position_offset_(0),
      bid_open_size_(0),
      ask_open_size_(0),
      trade_volume_(0),
      number_of_consecutive_exchange_rejects_(0),
      last_reject_evaluation_time_(0),
      time_based_total_exchange_rejects_(0),
      last_throttle_evaluation_time_(0),
      time_based_total_throttles_(0),
      total_exchange_rejects_(0),
      reject_based_freeze_timer_(0),
      query_trading_auto_freeze_manager_(HFSAT::BaseUtils::QueryTradingAutoFreezeManager::GetUniqueInstance()),
      is_auto_freeze_active_(true),
      is_magin_breach_risk_checks_hit_(false),
      is_risk_checks_hit_(false),
      is_modify_before_confirmation(_is_modify_before_confirmation),
      is_cancellable_before_confirmation(_is_cancellable_before_confirmation),
      p_next_new_order_(NULL),
      DEP_ONE_LOT_SIZE_(1),
      is_bmf_(HFSAT::kTLocBMF == HFSAT::TradingLocationUtils::GetTradingLocationFromHostname()),
      is_dataentry_freeze(false),
      exchange_margin_freeze_threshold_(90),
      exchange_margin_release_threshold_(85),
      min_throttle_wait_cycle(0),
      num_new_order_(0),
      num_cxl_order_(0),
      num_modify_order_(0),
      server_assigned_client_id_(_base_trader_.GetClientId()),
      livetrading_(_livetrading_)	{

  // **** IMPORTANT : THIS HAS TO BE CALLED BEFORE LoadPositionCheck **** 
  //get one lot size value
  if (HFSAT::SecurityDefinitions::CheckIfContractSpecExists(dep_shortcode_, watch_.YYYYMMDD())){
    DEP_ONE_LOT_SIZE_ = HFSAT::SecurityDefinitions::GetContractMinOrderSize(dep_shortcode_, watch_.YYYYMMDD());
    DBGLOG_CLASS_FUNC_LINE_INFO << "ONE_LOT_SIZE : " << dep_shortcode_ << " DATE : " << watch_.YYYYMMDD() << " " << DEP_ONE_LOT_SIZE_ << DBGLOG_ENDL_FLUSH ;
  }


  LoadPositionCheck(pos_key_val_map);
  watch_.subscribe_BigTimePeriod(this);
  // Pre-fetch the order for the first usage and then keep on allocating the same after
  p_next_new_order_ = baseorder_mempool_.Alloc();
  p_next_new_order_->security_name_ = dep_symbol_;
  p_next_new_order_->size_remaining_ = 0;  // on sending size_remaining_ is 0
  p_next_new_order_->num_open_modified_orders_ = 0;
  p_next_new_order_->queue_size_ahead_ = 0;
  p_next_new_order_->queue_size_behind_ = 0;
  p_next_new_order_->num_events_seen_ = 0;
  p_next_new_order_->placed_msecs_ = 0;
  p_next_new_order_->size_executed_ = 0;
  p_next_new_order_->canceled_ = false;
  p_next_new_order_->modified_ = false;
  p_next_new_order_->replayed_ = false;
  p_next_new_order_->is_reserved_type_ = false;
  p_next_new_order_->this_ptr_ = reinterpret_cast<uint64_t>(p_next_new_order_);

}

bool BasicOrderManager::NotifyExecAndPosListeners(int _position_diff_, const double _trade_price_,
                                                  const int _int_price_, const int _caos_) {
  if (_position_diff_ == 0) {
    return false;
  }
  TradeType_t _implied_buysell_ = (_position_diff_ > 0) ? (kTradeTypeBuy) : (kTradeTypeSell);
  int exec_qty_ = abs(_position_diff_);
  int net_position_ = client_position_ + position_offset_;

  trade_volume_ += exec_qty_;

  for (auto i = 0u; i < execution_listener_vec_.size(); i++) {
    execution_listener_vec_[i]->OnExec(net_position_, exec_qty_, _implied_buysell_, _trade_price_, _int_price_,
                                       dep_security_id_, _caos_);
  }
  return true;
}

void BasicOrderManager::OnTimePeriodUpdate(const int32_t num_pages_to_add) {
  // Rejects-Freeze handling
  if (watch_.msecs_from_midnight() - last_reject_evaluation_time_ >
      query_trading_auto_freeze_manager_.GetTimeBasedRejectsDuration()) {
    last_reject_evaluation_time_ = watch_.msecs_from_midnight();
    time_based_total_exchange_rejects_ = 0;
  }

  // Throttle handling
  if ((livetrading_) && (watch_.msecs_from_midnight() - last_throttle_evaluation_time_ > THROTTLE_COUNT_WINDOW_MSECS)) {
    last_throttle_evaluation_time_ = watch_.msecs_from_midnight();
    for (auto i = 0u; i < throttle_num_listener_vec_.size(); i++) {
      throttle_num_listener_vec_[i]->OnThrottleChange(time_based_total_throttles_, dep_security_id_);
    }
    time_based_total_throttles_ = 0;
  }

  if (reject_based_freeze_timer_ != 0 && (watch_.msecs_from_midnight() - reject_based_freeze_timer_ > 60000)) {
    ResetRejectsBasedFreeze();
    // Timer Has Expired
  }

  if (external_cancel_all_outstanding_orders_) {
    CancelAllOrders();
  }
}

BaseOrder* BasicOrderManager::SendTrade(const double _price_, const int _intpx_, int _size_requested_,
                                        TradeType_t t_buysell_, char placed_at_level_indicator_, OrderType_t order_type,
                                        int _executioner_id_, int _mirror_factor_, bool _is_reserved_type_, double disclosed_qty_factor_) {
  if (_size_requested_ <= 0) {
    DBGLOG_TIME_CLASS_FUNC << "Error BasicOrderManager::SendTrade :: did not send order due to non-positive size: "
                           << _size_requested_ << DBGLOG_ENDL_FLUSH;
    return nullptr;
  }
  uint64_t this_timestamp = GetReadTimeStampCounter();
  if (this_timestamp < min_throttle_wait_cycle && order_type != kOrderIOC && !_is_reserved_type_){
	return NULL;
  }
  if (is_dataentry_freeze){
    return NULL;
  }
  if((is_magin_breach_risk_checks_hit_) && (t_buysell_ == kTradeTypeSell)) {
    return NULL; // No short order is sent in case margin is breached on exchange level
  }

  if (((t_buysell_ == kTradeTypeBuy) &&
       (client_position_ + position_offset_ >= max_long_pos_ ||
        client_position_ + position_offset_ + _size_requested_ * _mirror_factor_ + bid_open_size_ > max_long_exposure_)) ||
      ((t_buysell_ == kTradeTypeSell) &&
       (client_position_ + position_offset_<= neg_max_short_pos_ ||
        (client_position_ + position_offset_ - _size_requested_ * _mirror_factor_ - ask_open_size_) < neg_max_short_exposure_)) ||
      external_cancel_all_outstanding_orders_) {
    // DBGLOG_TIME_CLASS_FUNC << " BUY/SELL LIMIT HIT!! " << _size_requested_ << " pos " << client_position_ << "
    // open_sz " << ask_open_size_ << " limit: " << max_short_pos_ << " exp " << max_short_exposure_ <<
    // DBGLOG_ENDL_FLUSH;
    return NULL;
  }

  p_next_new_order_->buysell_ = t_buysell_;
  p_next_new_order_->price_ = _price_;
  p_next_new_order_->size_requested_ = _size_requested_;  // and the requested size is shown here
  p_next_new_order_->size_disclosed_ = std::max((_size_requested_+9)/10,(int)(_size_requested_*disclosed_qty_factor_));
  p_next_new_order_->is_mirror_order_ = (order_type == kOrderIOC && _mirror_factor_ > 1) ? true : false;
  p_next_new_order_->mirror_factor_ = _mirror_factor_;

  p_next_new_order_->int_price_ = _intpx_;
  p_next_new_order_->order_status_ =
      kORRType_None;  // Sequenced feature is no longer required after shared memory usage

  p_next_new_order_->client_assigned_order_sequence_ = client_assigned_order_sequence_++;
  p_next_new_order_->server_assigned_order_sequence_ =
      0;  // this is intended to be an invalid value and hence all valid
  // server_assigned_order_sequence_ values must be > 0
  p_next_new_order_->executioner_id_ = _executioner_id_;

  p_next_new_order_->is_fok_ = (order_type == kOrderFOK);
  p_next_new_order_->is_ioc_ = (order_type == kOrderIOC);
  p_next_new_order_->placed_at_level_indicator_ = placed_at_level_indicator_;
  p_next_new_order_->is_reserved_type_ = (p_next_new_order_->is_ioc_ || _is_reserved_type_);

  base_trader_.SendTrade(*p_next_new_order_);
  num_new_order_++;

  if (t_buysell_ == kTradeTypeBuy) {
    bid_open_size_ += _size_requested_ * _mirror_factor_;
    bid_order_id_to_order_map_[p_next_new_order_->client_assigned_order_sequence_] = p_next_new_order_;
  } else {
    ask_open_size_ += _size_requested_ * _mirror_factor_;
    ask_order_id_to_order_map_[p_next_new_order_->client_assigned_order_sequence_] = p_next_new_order_;
  }

  // p_next_new_order_->SequenceAtTime(watch_.msecs_from_midnight(), watch_.tv().tv_usec);
  p_next_new_order_->order_status_ = (true == is_bmf_) ? kORRType_None : kORRType_Seqd;
  p_next_new_order_->size_remaining_ = p_next_new_order_->size_requested_;
  p_next_new_order_->seqd_msecs_ = watch_.msecs_from_midnight();
  p_next_new_order_->seqd_usecs_ = watch_.tv().tv_usec;

#ifdef _DBGLOGGER_OM_INFO_
  if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
    DBGLOG_TIME_CLASS << "SENDING " << watch_.msecs_from_midnight() << " " << p_next_new_order_->int_price_ << " "
                      << p_next_new_order_->buysell_ << " " << p_next_new_order_->client_assigned_order_sequence_
                      << DBGLOG_ENDL_FLUSH;
  }
#endif

  if (p_next_new_order_->is_ioc_ && p_next_new_order_->is_mirror_order_) {
    // p_next_new_order_->ioc_mirror_ = new MirrorDesc[_mirror_factor_];
    p_next_new_order_->num_outstanding_mirror_orders_ = _mirror_factor_;
  }

#ifdef _DBGLOGGER_SIM_ORDER_INFO_
  if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO)) {
    if (p_next_new_order_->is_ioc_ && p_next_new_order_->is_mirror_order_) {
      dbglogger_ << watch_.tv() << " OM SENDING NEWORDER IOC MIRROR " << dep_symbol_
                 << " MIRROR_SIZE: " << p_next_new_order_->mirror_factor_
                 << " CAOS: " << p_next_new_order_->client_assigned_order_sequence_
                 << " bs: " << (p_next_new_order_->buysell_ == kTradeTypeBuy ? "BUY" : "SELL")
                 << " sz: " << p_next_new_order_->size_requested_ << " px: " << p_next_new_order_->price_
                 << " intpx: " << p_next_new_order_->int_price_ << '\n';
      dbglogger_.CheckToFlushBuffer();
    } else {
      dbglogger_ << watch_.tv() << " OM SENDING NEWORDER " << dep_symbol_
                 << " CAOS: " << p_next_new_order_->client_assigned_order_sequence_
                 << " bs: " << (p_next_new_order_->buysell_ == kTradeTypeBuy ? "BUY" : "SELL")
                 << " sz: " << p_next_new_order_->size_requested_ << " px: " << p_next_new_order_->price_
                 << " intpx: " << p_next_new_order_->int_price_ << '\n';
      dbglogger_.CheckToFlushBuffer();
      dbglogger_ << "SYM: " << p_next_new_order_->security_name_ << " Px: " << p_next_new_order_->price()
                 << " INTPX: " << p_next_new_order_->int_price()
                 << " BS: " << GetTradeTypeChar(p_next_new_order_->buysell()) << " ST: " << watch_.tv() << " DT: "
                 << watch_.tv() +
                        ttime_t(0, p_next_new_order_->seqd_msecs_ * 1000 + p_next_new_order_->seqd_usecs_ % 1000) -
                        ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                 << " ORR: " << ToString(kORRType_Seqd)
                 << " SAOS: " << p_next_new_order_->server_assigned_order_sequence()
                 << " CAOS: " << p_next_new_order_->client_assigned_order_sequence() << " CLTPOS: " << client_position_
                 << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << global_position_
                 << " SIZE: " << p_next_new_order_->size_remaining() << " SE: " << p_next_new_order_->size_executed()
                 << " MsgSeq : " << 0 << " Seq: " << 0 << DBGLOG_ENDL_FLUSH;
      dbglogger_.CheckToFlushBuffer();
    }
  }
#endif

  // Declare a new ptr so that we can return this and pre-fetch the next new order
  BaseOrder* p_current_new_order_ = p_next_new_order_;

  // Pre-fetch the order for the first usage and then keep on allocating the same after
  p_next_new_order_ = baseorder_mempool_.Alloc();

  // TODO - We can even avoid this long as we can gurantee the size of mempool doesn't increase in live run
  p_next_new_order_->security_name_ = dep_symbol_;

  p_next_new_order_->size_remaining_ = 0;  // on sending size_remaining_ is 0
  p_next_new_order_->num_open_modified_orders_ = 0;
  p_next_new_order_->queue_size_ahead_ = 0;
  p_next_new_order_->queue_size_behind_ = 0;
  p_next_new_order_->num_events_seen_ = 0;
  p_next_new_order_->placed_msecs_ = 0;
  p_next_new_order_->size_executed_ = 0;
  p_next_new_order_->canceled_ = false;
  p_next_new_order_->modified_ = false;
  p_next_new_order_->replayed_ = false;
  p_next_new_order_->this_ptr_ = reinterpret_cast<uint64_t>(p_next_new_order_);

  return p_current_new_order_;
}

bool BasicOrderManager::Modify(BaseOrder* t_this_order_, double _new_price_, int _new_int_price_, int _new_size_,
                               bool _is_reserved_type_, double disclosed_qty_factor_) {
  uint64_t this_timestamp = GetReadTimeStampCounter();
  if (this_timestamp < min_throttle_wait_cycle && !_is_reserved_type_ && !t_this_order_->is_ioc_){
	return false;
  }
  if (t_this_order_->order_status_ == kORRType_Conf && !t_this_order_->canceled_ &&
      (!t_this_order_->num_open_modified_orders_ ||
       (is_modify_before_confirmation &&
        ((t_this_order_->modified_new_int_price_ != _new_int_price_) || (!t_this_order_->modified_)) &&
        (t_this_order_->num_open_modified_orders_ <= NUM_OPEN_ORDERS_MODIFY_ALLOWED) &&
        (t_this_order_->mirror_factor_ <= NUM_EXCH_REJ_FOR_CONS_MODIFY_ALLOWED)))) {
    if (((t_this_order_->buysell_ == kTradeTypeBuy) &&
         (client_position_ + position_offset_ >= max_long_pos_ ||
          client_position_ + position_offset_ + _new_size_ - t_this_order_->size_remaining_ + bid_open_size_ > max_long_exposure_)) ||
        ((t_this_order_->buysell_ == kTradeTypeSell) &&
         (client_position_ + position_offset_<= neg_max_short_pos_ ||
          (client_position_ + position_offset_ - _new_size_ + t_this_order_->size_remaining_ - ask_open_size_) <
              neg_max_short_exposure_))) {
      // DBGLOG_TIME_CLASS_FUNC << " BUY LIMIT HIT!! " << _new_size_ << " pos " << client_position_ << " open_sz " <<
      // bid_open_size_ << " limit: " << max_long_pos_ << " exp " << max_long_exposure_ << DBGLOG_ENDL_FLUSH;
      // DBGLOG_TIME_CLASS_FUNC << " SELL LIMIT HIT!! " << _new_size_ << " pos " << client_position_ << " open_sz " <<
      // ask_open_size_ << " limit: " << max_short_pos_ << " exp " << max_short_exposure_ << DBGLOG_ENDL_FLUSH;
      Cancel(*t_this_order_);
      return false;
    }

    t_this_order_->modified_ = true;
    t_this_order_->size_disclosed_ = std::max((_new_size_+9)/10,(int)(_new_size_*disclosed_qty_factor_));

    t_this_order_->num_open_modified_orders_++;

    t_this_order_->modified_new_size_ = _new_size_;
    t_this_order_->modified_new_price_ = _new_price_;
    t_this_order_->modified_new_int_price_ = _new_int_price_;
    t_this_order_->is_reserved_type_ = _is_reserved_type_;

    /*if (t_this_order_->buysell_ == kTradeTypeBuy) {
      bid_open_size_ += _new_size_ - t_this_order_->size_remaining_;
      } else {
      ask_open_size_ += _new_size_ - t_this_order_->size_remaining_;
      }*/

    // DBGLOG_TIME_CLASS_FUNC << "MODIFY bos " << bid_open_size_ << " aos " << ask_open_size_ << DBGLOG_ENDL_FLUSH;
    base_trader_.Modify(*t_this_order_, _new_price_, _new_int_price_, _new_size_);
    num_modify_order_++;

    if (t_this_order_->num_open_modified_orders_ > 1) {
      dbglogger_ << watch_.tv() << " USECASE: SENDING MODIFY OF ALREADy MODIFIED ORDER" << DBGLOG_ENDL_FLUSH;
    }
// t_this_order_->SequenceAtTime( watch_.msecs_from_midnight() , watch_.tv().tv_usec);
#ifdef _DBGLOGGER_SIM_ORDER_INFO_
    if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO)) {
      dbglogger_ << watch_.tv() << " OM SENDING MODIFY " << dep_symbol_
                 << " CAOS: " << t_this_order_->client_assigned_order_sequence_
                 << " bs: " << ((t_this_order_->buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                 << " sz: " << t_this_order_->size_requested_ << " px: " << t_this_order_->price_
                 << " intpx: " << t_this_order_->int_price_ << " new_sz: " << _new_size_ << " new_px: " << _new_price_
                 << " new_intpx: " << _new_int_price_ << " is_reserved: " << ((_is_reserved_type_) ? "YES" : "NO")
                 << DBGLOG_ENDL_FLUSH;
      dbglogger_ << "SYM: " << t_this_order_->security_name_ << " Px: " << _new_price_ << " INTPX: " << _new_int_price_
                 << " BS: " << GetTradeTypeChar(t_this_order_->buysell()) << " ST: " << watch_.tv() << " DT: "
                 << watch_.tv() + ttime_t(0, t_this_order_->seqd_msecs_ * 1000 + t_this_order_->seqd_usecs_ % 1000) -
                        ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                 << " ORR: " << ToString(kORRType_CxReSeqd)
                 << " SAOS: " << t_this_order_->server_assigned_order_sequence()
                 << " CAOS: " << t_this_order_->client_assigned_order_sequence() << " CLTPOS: " << client_position_
                 << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << global_position_
                 << " SIZE: " << t_this_order_->size_remaining() << " SE: " << t_this_order_->size_executed()
                 << " MsgSeq : " << 0 << " Seq: " << t_this_order_->num_open_modified_orders_ << DBGLOG_ENDL_FLUSH;
      dbglogger_.CheckToFlushBuffer();
    }
#endif
    return true;
  }
  return false;
}

void BasicOrderManager::CancelAllBidOrders() {
  for (std::map<int, BaseOrder*>::iterator iter = bid_order_id_to_order_map_.begin();
       iter != bid_order_id_to_order_map_.end(); iter++) {
    Cancel(*(iter->second));
  }
}

void BasicOrderManager::CancelAllAskOrders() {
  for (std::map<int, BaseOrder*>::iterator iter = ask_order_id_to_order_map_.begin();
       iter != ask_order_id_to_order_map_.end(); iter++) {
    Cancel(*(iter->second));
  }
}

void BasicOrderManager::CancelAllOrders() {
  CancelAllBidOrders();
  CancelAllAskOrders();
  external_cancel_all_outstanding_orders_ = true;
}

/**
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param t_server_assigned_order_sequence_
 * @param _security_id_
 * @param t_buysell_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 * @param exchange_order_id
 * @param time_set_by_server
 */
void BasicOrderManager::OrderNotFound(const int t_server_assigned_client_id_,
                                      const int _client_assigned_order_sequence_,
                                      const int t_server_assigned_order_sequence_, const unsigned int _security_id_,
                                      const TradeType_t t_buysell_, const int r_int_price_,
                                      const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                      const ttime_t time_set_by_server) {
  if (t_server_assigned_client_id_ == server_assigned_client_id_) {
    BASE_ORDER_MAP* order_map = NULL;
    if (t_buysell_ == kTradeTypeBuy) {
      order_map = &bid_order_id_to_order_map_;
    } else {
      order_map = &ask_order_id_to_order_map_;
    }

    BaseOrder* p_this_order_ = nullptr; /* will stay nullptr till order found */
    auto iter = order_map->find(_client_assigned_order_sequence_);

    if (iter == order_map->end()) {
      DBGLOG_TIME_CLASS_FUNC << "WeirdOrder -  SACI: " << t_server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " bs: " << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " intpx: " << r_int_price_
                             << DBGLOG_ENDL_FLUSH;
      return;
    }

    p_this_order_ = iter->second;

    if (p_this_order_->is_ioc_ && p_this_order_->is_mirror_order_) {
      if (!t_server_assigned_order_sequence_) {
        p_this_order_->order_status_ = kORRType_Rejc;
        order_map->erase(iter);
        NotifyOrderChangeListeners(p_this_order_);
        baseorder_mempool_.DeAlloc(p_this_order_);
      } else {
        p_this_order_->num_outstanding_mirror_orders_--;
        if (p_this_order_->num_outstanding_mirror_orders_ == 0) {
          p_this_order_->order_status_ = kORRType_Rejc;
          order_map->erase(iter);
          NotifyOrderChangeListeners(p_this_order_);
          baseorder_mempool_.DeAlloc(p_this_order_);
        }
      }
    } else {
      p_this_order_->order_status_ = kORRType_Rejc;
      order_map->erase(iter);
      NotifyOrderChangeListeners(p_this_order_);
      baseorder_mempool_.DeAlloc(p_this_order_);
    }

#ifdef _DBGLOGGER_SIM_ORDER_INFO_
    if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " SACI: " << t_server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " bs: " << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " intpx: " << r_int_price_
                             << DBGLOG_ENDL_FLUSH;
      LogFullStatus();
    }
#endif
  }
}

/**
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param t_server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param t_buysell_
 * @param _size_remaining_
 * @param _size_executed_
 * @param t_client_position_
 * @param t_global_position_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 * @param exchange_order_id
 * @param time_set_by_server
 */
void BasicOrderManager::OrderSequenced(const int t_server_assigned_client_id_,
                                       const int _client_assigned_order_sequence_,
                                       const int t_server_assigned_order_sequence_, const unsigned int _security_id_,
                                       const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                                       const int _size_executed_, const int t_client_position_,
                                       const int t_global_position_, const int r_int_price_,
                                       const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                       const ttime_t time_set_by_server) {
  global_position_ = t_global_position_;

  if (t_server_assigned_client_id_ == server_assigned_client_id_) {  // our order

    BASE_ORDER_MAP* order_map = NULL;
    if (t_buysell_ == kTradeTypeBuy) {
      order_map = &bid_order_id_to_order_map_;
    } else {
      order_map = &ask_order_id_to_order_map_;
    }

    BaseOrder* p_this_order_ = nullptr;
    std::map<int, BaseOrder*>::iterator iter = order_map->find(_client_assigned_order_sequence_);
    if (iter != order_map->end()) {
      p_this_order_ = iter->second;
      p_this_order_->ors_ptr_ = exchange_order_id;
      p_this_order_->seqd_msecs_ = watch_.msecs_from_midnight();
      p_this_order_->seqd_usecs_ = watch_.tv().tv_usec;
      p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;
      p_this_order_->order_status_ = kORRType_Seqd;
    } else {
      DBGLOG_TIME_CLASS_FUNC << "WeirdOrder -  SACI: " << t_server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " bs: " << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " intpx: " << r_int_price_
                             << DBGLOG_ENDL_FLUSH;
      return;
    }
#ifdef _DBGLOGGER_SIM_ORDER_INFO_
    if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO)) {
      LogFullStatus();
    }
#endif
  }
}

/**
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param t_server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param t_buysell_
 * @param _size_remaining_
 * @param _size_executed_
 * @param t_client_position_
 * @param t_global_position_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 * @param exchange_order_id
 * @param time_set_by_server
 */
void BasicOrderManager::OrderConfirmed(const int t_server_assigned_client_id_,
                                       const int _client_assigned_order_sequence_,
                                       const int t_server_assigned_order_sequence_, const unsigned int _security_id_,
                                       const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                                       const int _size_executed_, const int t_client_position_,
                                       const int t_global_position_, const int r_int_price_,
                                       const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                       const ttime_t time_set_by_server) {
  global_position_ = t_global_position_;

  if (t_server_assigned_client_id_ == server_assigned_client_id_) {  // our order
    // Reset the counter for exchange rejects tracker
    number_of_consecutive_exchange_rejects_ = 0;

    BASE_ORDER_MAP* order_map = NULL;
    if (t_buysell_ == kTradeTypeBuy) {
      order_map = &bid_order_id_to_order_map_;
    } else {
      order_map = &ask_order_id_to_order_map_;
    }

    BaseOrder* p_this_order_ = nullptr;
    std::map<int, BaseOrder*>::iterator iter = order_map->find(_client_assigned_order_sequence_);
    if (iter != order_map->end()) {
      p_this_order_ = iter->second;
      p_this_order_->ors_ptr_ = exchange_order_id;
      if (p_this_order_->is_ioc_ && p_this_order_->is_mirror_order_) {
        if (!t_server_assigned_order_sequence_) {
          p_this_order_->order_status_ = kORRType_Rejc;
          order_map->erase(iter);
          NotifyOrderChangeListeners(p_this_order_);
          baseorder_mempool_.DeAlloc(p_this_order_);
        }
      } else {
        p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;
      }
    } else {
      DBGLOG_TIME_CLASS_FUNC << "WeirdOrder -  SACI: " << t_server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " bs: " << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " intpx: " << r_int_price_
                             << DBGLOG_ENDL_FLUSH;
      return;
    }

    if (t_server_assigned_order_sequence_ && !p_this_order_->is_ioc_) {
      // p_this_order_->ConfirmAtTime(watch_.msecs_from_midnight());
      p_this_order_->order_status_ = kORRType_Conf;
      p_this_order_->num_events_seen_ = 0;
      p_this_order_->ConfirmNewSize(_size_remaining_);  // set both size_requested_ and size_remaining_ to this value
      NotifyOrderChangeListeners(p_this_order_);
    }

#ifdef _DBGLOGGER_SIM_ORDER_INFO_
    if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO))  // zero logging
    {
      dbglogger_ << watch_.tv() << " " << sec_name_indexer_.GetShortcodeFromId(_security_id_) << " NEW_CONFIRM"
                 << " SACI: " << t_server_assigned_client_id_ << " CAOS: " << _client_assigned_order_sequence_
                 << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_ << " px: " << _price_
                 << " bs: " << ((t_buysell_ == kTradeTypeBuy) ? "BUY" : "SELL") << " sizeR " << _size_remaining_
                 << " sizeE " << _size_executed_ << " cpos: " << t_client_position_ << " gpos: " << t_global_position_
                 << " intpx: " << r_int_price_ << DBGLOG_ENDL_FLUSH;
      if (p_this_order_ != nullptr)  // zero logging
      {
        dbglogger_ << "SYM: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " Px: " << _price_
                   << " INTPX: " << p_this_order_->int_price() << " BS: " << GetTradeTypeChar(t_buysell_)
                   << " ST: " << watch_.tv() << " DT: "
                   << watch_.tv() + ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                          ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                   << " ORR: " << ToString(kORRType_Conf) << " SAOS: " << t_server_assigned_order_sequence_
                   << " CAOS: " << _client_assigned_order_sequence_ << " CLTPOS: " << t_client_position_
                   << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << t_global_position_
                   << " SIZE: " << _size_remaining_ << " SE: " << 0 << " MsgSeq : " << 0 << " Seq: " << 0
                   << DBGLOG_ENDL_FLUSH;
      }
      LogFullStatus();
    }
#endif
  }
}

/**
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param t_server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param t_buysell_
 * @param _size_remaining_
 * @param _size_executed_
 * @param t_client_position_
 * @param t_global_position_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 */
void BasicOrderManager::OrderConfCxlReplaced(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int t_server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t t_buysell_, const int _size_remaining_, const int _size_executed_, const int t_client_position_,
    const int t_global_position_, const int r_int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  global_position_ = t_global_position_;

  if (t_server_assigned_client_id_ == server_assigned_client_id_) {
    // Reset the counter for exchange rejects tracker
    number_of_consecutive_exchange_rejects_ = 0;

    BASE_ORDER_MAP* order_map = NULL;
    if (t_buysell_ == kTradeTypeBuy) {
      order_map = &bid_order_id_to_order_map_;
    } else {
      order_map = &ask_order_id_to_order_map_;
    }

    BaseOrder* p_this_order_ = nullptr; /* will stay nullptr till order found */
    std::map<int, BaseOrder*>::iterator iter = order_map->find(_client_assigned_order_sequence_);
    if (iter != order_map->end()) {
      p_this_order_ = iter->second;
      // assign the new saos
      p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

      if (t_buysell_ == kTradeTypeBuy) {  // BID order
        bid_open_size_ = bid_open_size_ + (_size_remaining_ - p_this_order_->size_remaining_);
      } else {
        ask_open_size_ = ask_open_size_ + (_size_remaining_ - p_this_order_->size_remaining_);
      }

      p_this_order_->size_remaining_ = _size_remaining_;
      // p_this_order_->ConfirmAtTime(watch_.msecs_from_midnight());
      p_this_order_->order_status_ = kORRType_Conf;
      p_this_order_->num_events_seen_ = 0;

      p_this_order_->num_open_modified_orders_--;

      if (p_this_order_->num_open_modified_orders_ == 0) {
        p_this_order_->modified_ = false;
        // Setting it invalid, showing  order-modified has been confirmed
        p_this_order_->modified_new_size_ = -1;
      }

      p_this_order_->price_ = _price_;
      p_this_order_->int_price_ = r_int_price_;

      NotifyOrderChangeListeners(p_this_order_);
    } else {
      DBGLOG_TIME_CLASS_FUNC << "WeirdOrder -  SACI: " << t_server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " bs: " << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " intpx: " << r_int_price_
                             << DBGLOG_ENDL_FLUSH;
      return;
    }

#ifdef _DBGLOGGER_SIM_ORDER_INFO_
    if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO)) {
      dbglogger_ << watch_.tv() << " " << sec_name_indexer_.GetShortcodeFromId(_security_id_) << " MODIFY_CONFIRM"
                 << " SACI: " << t_server_assigned_client_id_ << " CAOS: " << _client_assigned_order_sequence_
                 << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_ << " px: " << _price_
                 << " bs: " << ((t_buysell_ == kTradeTypeBuy) ? "BUY" : "SELL") << " sizeR " << _size_remaining_
                 << " sizeE " << _size_executed_ << " cpos: " << t_client_position_ << " gpos: " << t_global_position_
                 << " intpx: " << r_int_price_ << DBGLOG_ENDL_FLUSH;
      if (p_this_order_ != nullptr) {
        dbglogger_ << "SYM: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " Px: " << _price_
                   << " INTPX: " << p_this_order_->int_price() << " BS: " << GetTradeTypeChar(t_buysell_)
                   << " ST: " << watch_.tv() << " DT: "
                   << watch_.tv() + ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                          ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                   << " ORR: " << ToString(kORRType_CxRe) << " SAOS: " << t_server_assigned_order_sequence_
                   << " CAOS: " << _client_assigned_order_sequence_ << " CLTPOS: " << t_client_position_
                   << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << t_global_position_
                   << " SIZE: " << _size_remaining_ << " SE: " << p_this_order_->size_executed_ << " MsgSeq : " << 0
                   << " Seq: " << p_this_order_->num_open_modified_orders_ << DBGLOG_ENDL_FLUSH;
      }
      LogFullStatus();
    }
#endif
  }
}

/**
 *
 * @param _server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param _server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param _buysell_
 * @param _size_remaining_
 * @param _client_position_
 * @param _global_position_
 * @param _int_price_
 * @param rejection_reason
 * @param server_assigned_message_sequence
 */
void BasicOrderManager::OrderConfCxlReplaceRejected(
    const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t _buysell_, const int _size_remaining_, const int _client_position_, const int _global_position_,
    const int _int_price_, const int32_t rejection_reason, const int32_t server_assigned_message_sequence,
    const uint64_t throttle_wait_cycle, const ttime_t time_set_by_server) {
  if (_server_assigned_client_id_ == server_assigned_client_id_) {
    BASE_ORDER_MAP* order_map = NULL;
    if (_buysell_ == kTradeTypeBuy) {
      order_map = &bid_order_id_to_order_map_;
    } else {
      order_map = &ask_order_id_to_order_map_;
    }

    bool is_exchange_reject_ = false;
    if (HFSAT::kExchCancelReplaceReject == (HFSAT::CxlReplaceRejectReason_t)rejection_reason) {
      number_of_consecutive_exchange_rejects_++;
      time_based_total_exchange_rejects_++;
      total_exchange_rejects_++;
      is_exchange_reject_ = true;
    }

    if (HFSAT::kORSCxReRejectThrottleLimitReached == (HFSAT::CxlReplaceRejectReason_t)rejection_reason) {
      min_throttle_wait_cycle = GetReadTimeStampCounter() + throttle_wait_cycle;
      time_based_total_throttles_++;
    }

    if (HFSAT::kORSCxReRejectSecurityNotFound == (HFSAT::CxlReplaceRejectReason_t)rejection_reason) {
      NotifyExchangeRejectsListeners(HFSAT::BaseUtils::FreezeEnforcedReason::kFreezeOnORSSecNotFound);
      // If we've not reset the rejects and we've breached threshold
      // our freeze timeout starts now
      if (0 == reject_based_freeze_timer_) {
        reject_based_freeze_timer_ = watch_.msecs_from_midnight();
      }
    }

    if (HFSAT::kORSCxReRejectMarginCheckFailedMaxPosition == (HFSAT::CxlReplaceRejectReason_t)rejection_reason) {
      NotifyExchangeRejectsListeners(HFSAT::BaseUtils::FreezeEnforcedReason::kFreezeOnORSMaxPos);
      // ORS Max Position breached
      if (0 == reject_based_freeze_timer_) {
        reject_based_freeze_timer_ = watch_.msecs_from_midnight();
      }
    }
    if (HFSAT::kORSCxReRejectMarginCheckFailedWorstCasePosition == (HFSAT::CxlReplaceRejectReason_t)rejection_reason) {
      NotifyExchangeRejectsListeners(HFSAT::BaseUtils::FreezeEnforcedReason::kFreezeOnORSWorstPos);
      // ORS Worst Position breached
      if (0 == reject_based_freeze_timer_) {
        reject_based_freeze_timer_ = watch_.msecs_from_midnight();
      }
    }

    // Special handling for order not found
    bool order_not_found_rejc_ = false;
    if (HFSAT::kCxlReRejectOrderNotFound == (HFSAT::CxlReplaceRejectReason_t)rejection_reason) {
      order_not_found_rejc_ = true;
    }

    HFSAT::BaseUtils::FreezeEnforcedReason freeze_enforcement = query_trading_auto_freeze_manager_.ShouldAllowOrders(
        number_of_consecutive_exchange_rejects_, time_based_total_exchange_rejects_, total_exchange_rejects_,
        time_based_total_throttles_);

    if (HFSAT::BaseUtils::FreezeEnforcedReason::kAllow != freeze_enforcement) {
      NotifyExchangeRejectsListeners(freeze_enforcement);
      // If we've not reset the rejects and we've breached threshold
      // our freeze timeout starts now
      if (0 == reject_based_freeze_timer_) {
        reject_based_freeze_timer_ = watch_.msecs_from_midnight();
      }
    }

    BaseOrder* p_this_order_ = nullptr; /* will stay nullptr till order found */
    std::map<int, BaseOrder*>::iterator iter = order_map->find(_client_assigned_order_sequence_);
    if (iter != order_map->end()) {
      p_this_order_ = iter->second;

      if (is_exchange_reject_) {
        p_this_order_->mirror_factor_++;  // Counting the modify reject for that order
      }

      if (!order_not_found_rejc_) {  // Throttle
        p_this_order_->modified_ = false;
        p_this_order_->modified_new_size_ = -1;
        p_this_order_->num_open_modified_orders_--;
      } else {
        p_this_order_->order_status_ = kORRType_Exec;  // Or this might be Exec also
        NotifyOrderChangeListeners(p_this_order_);
        order_map->erase(iter);
        baseorder_mempool_.DeAlloc(p_this_order_);
        DBGLOG_TIME_CLASS_FUNC << "Error ModifyOrder not found -  SACI: " << server_assigned_client_id_
                               << " CAOS: " << _client_assigned_order_sequence_
                               << " SAOS: " << _server_assigned_order_sequence_ << " sid: " << _security_id_
                               << " bs: BUY"
                               << " intpx: " << _int_price_ << DBGLOG_ENDL_FLUSH;
      }
    } else {
      DBGLOG_TIME_CLASS_FUNC << "WeirdOrder -  SACI: " << server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << _server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " bs: " << (_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " intpx: " << _int_price_
                             << DBGLOG_ENDL_FLUSH;
      return;
    }

#ifdef _DBGLOGGER_OM_INFO_
    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      dbglogger_ << watch_.tv() << " " << sec_name_indexer_.GetShortcodeFromId(_security_id_) << " MODIFY_REJECT"
                 << " SACI: " << _server_assigned_client_id_ << " CAOS: " << _client_assigned_order_sequence_
                 << " SAOS: " << _server_assigned_order_sequence_ << " sid: " << _security_id_ << " px: " << _price_
                 << " bs: " << ((_buysell_ == kTradeTypeBuy) ? "BUY" : "SELL") << " sizeR " << _size_remaining_
                 << " cpos: " << _client_position_ << " gpos: " << _global_position_ << " intpx: " << _int_price_
                 << DBGLOG_ENDL_FLUSH;
    }
#endif
#ifdef _DBGLOGGER_SIM_ORDER_INFO_
    if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO) && p_this_order_ != nullptr) {
      dbglogger_ << "SYM: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " Px: " << _price_
                 << " INTPX: " << _int_price_ << " BS: " << GetTradeTypeChar(_buysell_) << " ST: " << watch_.tv()
                 << " DT: "
                 << watch_.tv() + ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                        ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                 << " ORR: " << ToString(kORRType_CxReRejc) << " SAOS: " << _server_assigned_order_sequence_
                 << " CAOS: " << _client_assigned_order_sequence_ << " CLTPOS: " << _client_position_
                 << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << _global_position_
                 << " SIZE: " << _size_remaining_ << " SE: " << p_this_order_->size_executed_ << " MsgSeq : " << 0
                 << " Seq: " << 0 << DBGLOG_ENDL_FLUSH;
      LogFullStatus();
    }
#endif
  }
}

/**
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param t_server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param t_buysell_
 * @param _size_remaining_
 * @param t_client_position_
 * @param t_global_position_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 * @param exchange_order_id
 * @param time_set_by_server
 */
void BasicOrderManager::OrderCanceled(const int t_server_assigned_client_id_,
                                      const int _client_assigned_order_sequence_,
                                      const int t_server_assigned_order_sequence_, const unsigned int _security_id_,
                                      const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                                      const int t_client_position_, const int t_global_position_,
                                      const int r_int_price_, const int32_t server_assigned_message_sequence,
                                      const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  global_position_ = t_global_position_;

  if ((!is_magin_breach_risk_checks_hit_) && (2 == server_assigned_message_sequence)) {
      is_magin_breach_risk_checks_hit_ = true;
      for (uint32_t listener_counter = 0; listener_counter < exch_rejects_listener_vec_.size();
        listener_counter++) {
        exch_rejects_listener_vec_[listener_counter]->OnGetFreezeDueToExchangeRejects(
            HFSAT::BaseUtils::FreezeEnforcedReason::kFreezeOnMarginBreach);
        }
    }
  if (t_server_assigned_client_id_ == server_assigned_client_id_) {
    // Reset the counter for exchange rejects tracker
    number_of_consecutive_exchange_rejects_ = 0;

    BASE_ORDER_MAP* order_map = NULL;
    if (t_buysell_ == kTradeTypeBuy) {
      order_map = &bid_order_id_to_order_map_;
    } else {
      order_map = &ask_order_id_to_order_map_;
    }

    BaseOrder* p_this_order_ = nullptr; /* will stay nullptr till order found */
    bool is_ioc_ = false;

    std::map<int, BaseOrder*>::iterator iter = order_map->find(_client_assigned_order_sequence_);
    if (iter != order_map->end()) {
      p_this_order_ = iter->second;

      if (p_this_order_->is_ioc_ && p_this_order_->is_mirror_order_) {
        if (!t_server_assigned_order_sequence_) {
          p_this_order_->order_status_ = kORRType_Rejc;
          order_map->erase(iter);
          NotifyOrderChangeListeners(p_this_order_);
          baseorder_mempool_.DeAlloc(p_this_order_);
        } else {
          p_this_order_->num_outstanding_mirror_orders_--;
          if (t_buysell_ == kTradeTypeBuy) {  // BID order
            bid_open_size_ = bid_open_size_ - _size_remaining_;
          } else {
            ask_open_size_ = ask_open_size_ - _size_remaining_;
          }

          is_ioc_ = p_this_order_->is_ioc_;

          if (p_this_order_->num_outstanding_mirror_orders_ == 0) {
            p_this_order_->order_status_ = kORRType_Cxld;
            NotifyOrderChangeListeners(p_this_order_);
            order_map->erase(iter);
            baseorder_mempool_.DeAlloc(p_this_order_);
          }
        }
      } else {
        p_this_order_->order_status_ = kORRType_Cxld;
        if (t_buysell_ == kTradeTypeBuy) {  // BID order
          bid_open_size_ = bid_open_size_ - _size_remaining_;
        } else {
          ask_open_size_ = ask_open_size_ - _size_remaining_;
        }
        is_ioc_ = p_this_order_->is_ioc_;

        NotifyOrderChangeListeners(p_this_order_);
        order_map->erase(iter);
        baseorder_mempool_.DeAlloc(p_this_order_);
      }
    } else {
      DBGLOG_TIME_CLASS_FUNC << "WeirdOrder -  SACI: " << server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " bs: " << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " intpx: " << r_int_price_
                             << DBGLOG_ENDL_FLUSH;
      return;
    }

#ifdef _DBGLOGGER_OM_INFO_
    if (dbglogger_.CheckLoggingLevel(OM_INFO))  // zero logging
    {
      dbglogger_ << watch_.tv() << " " << sec_name_indexer_.GetShortcodeFromId(_security_id_) << " CANCEL_CONFIRM"
                 << " SACI: " << t_server_assigned_client_id_ << " CAOS: " << _client_assigned_order_sequence_
                 << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_ << " px: " << _price_
                 << " bs: " << ((t_buysell_ == kTradeTypeBuy) ? "BUY" : "SELL") << " sizeR " << _size_remaining_
                 << " cpos: " << t_client_position_ << " gpos: " << t_global_position_ << " intpx: " << r_int_price_
                 << DBGLOG_ENDL_FLUSH;
#ifdef _DBGLOGGER_SIM_ORDER_INFO_
      if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO) && p_this_order_ != nullptr)  // zero logging
      {
        dbglogger_ << "SYM: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " Px: " << _price_
                   << " INTPX: " << p_this_order_->int_price() << " BS: " << GetTradeTypeChar(t_buysell_)
                   << " ST: " << watch_.tv() << " DT: "
                   << watch_.tv() + ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                          ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                   << " ORR: " << ToString(kORRType_Cxld) << " SAOS: " << t_server_assigned_order_sequence_
                   << " CAOS: " << _client_assigned_order_sequence_ << " CLTPOS: " << t_client_position_
                   << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << t_global_position_
                   << " SIZE: " << _size_remaining_ << " SE: " << 0 << " MsgSeq : " << 0 << " Seq: " << 0
                   << DBGLOG_ENDL_FLUSH;
      }
      LogFullStatus();
#endif
    }
#endif
    if (!is_ioc_) {
#ifdef _DBGLOGGER_FILL_TIME_INFO_
      if (dbglogger_.CheckLoggingLevel(FILL_TIME_INFO)) {
        dbglogger_ << watch_.tv() << " " << sec_name_indexer_.GetShortcodeFromId(_security_id_) << " CANCEL_CONFIRM"
                   << " SACI: " << t_server_assigned_client_id_ << " CAOS: " << _client_assigned_order_sequence_
                   << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_ << " px: " << _price_
                   << " bs: " << ((t_buysell_ == kTradeTypeBuy) ? "BUY" : "SELL") << " sizeR " << _size_remaining_
                   << " cpos: " << t_client_position_ << " gpos: " << t_global_position_ << " intpx: " << r_int_price_
                   << DBGLOG_ENDL_FLUSH;
      }
#endif
    }
  }
}

/**
 *  handle the executions
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param t_server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param t_buysell_
 * @param _size_remaining_
 * @param _size_executed_
 * @param t_client_position_
 * @param t_global_position_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 * @param exchange_order_id
 * @param time_set_by_server
 */
void BasicOrderManager::OrderExecuted(const int t_server_assigned_client_id_,
                                      const int _client_assigned_order_sequence_,
                                      const int t_server_assigned_order_sequence_, const unsigned int _security_id_,
                                      const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                                      const int _size_executed_, const int t_client_position_,
                                      const int t_global_position_, const int r_int_price_,
                                      const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                      const ttime_t time_set_by_server) {
  global_position_ = t_global_position_;

  if (t_server_assigned_client_id_ == server_assigned_client_id_) {
    // Reset the counter for exchange rejects tracker
    number_of_consecutive_exchange_rejects_ = 0;

    int position_diff_ = t_client_position_ - client_position_;
    client_position_ = t_client_position_;

    BASE_ORDER_MAP* order_map = NULL;
    if (t_buysell_ == kTradeTypeBuy) {
      order_map = &bid_order_id_to_order_map_;
    } else {
      order_map = &ask_order_id_to_order_map_;
    }

    BaseOrder* p_this_order_ = nullptr; /* will stay nullptr till order found */
    std::map<int, BaseOrder*>::iterator iter = order_map->find(_client_assigned_order_sequence_);
    if (iter != order_map->end()) {
      p_this_order_ = iter->second;
      if (p_this_order_->is_ioc_ && p_this_order_->is_mirror_order_) {
        if (!t_server_assigned_order_sequence_) {
          p_this_order_->order_status_ = kORRType_Rejc;
          order_map->erase(iter);
          NotifyOrderChangeListeners(p_this_order_);
          baseorder_mempool_.DeAlloc(p_this_order_);
        } else {
          if (t_buysell_ == kTradeTypeBuy) {
            bid_open_size_ = bid_open_size_ - position_diff_;
          } else {
            ask_open_size_ = ask_open_size_ + position_diff_;
          }
          if (_size_remaining_ <= 0) {
            NotifyExecAndPosListeners(position_diff_, _price_, r_int_price_, _client_assigned_order_sequence_);
            p_this_order_->num_outstanding_mirror_orders_--;
          }
          if (p_this_order_->num_outstanding_mirror_orders_ == 0) {
            p_this_order_->order_status_ = kORRType_Exec;
            NotifyOrderChangeListeners(p_this_order_);
            order_map->erase(iter);
            baseorder_mempool_.DeAlloc(p_this_order_);
          }
        }
      } else {
        // p_this_order_->ConfirmAtTime(watch_.msecs_from_midnight());
        p_this_order_->order_status_ = kORRType_Conf;
        p_this_order_->num_events_seen_ = 0;

        p_this_order_->ConfirmNewSize(_size_remaining_);
        p_this_order_->Execute(_size_executed_);

#ifdef _DBGLOGGER_SIM_ORDER_INFO_
        if (p_this_order_->modified_) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " RARE: Received Bid exec before ModifyConfirm : "
                                      << " exec_px: " << _price_ << " size: " << _size_executed_
                                      << " mod_new_px: " << p_this_order_->modified_new_price_
                                      << " mod_new_sz: " << p_this_order_->modified_new_size_ << DBGLOG_ENDL_FLUSH;
        }
#endif

        if (t_buysell_ == kTradeTypeBuy) {
          bid_open_size_ = bid_open_size_ - position_diff_;
        } else {
          ask_open_size_ = ask_open_size_ + position_diff_;
        }

        if (_size_remaining_ <= 0) {
          p_this_order_->order_status_ = kORRType_Exec;
          NotifyExecAndPosListeners(position_diff_, _price_, r_int_price_, _client_assigned_order_sequence_);
          NotifyOrderChangeListeners(p_this_order_);
          order_map->erase(iter);
          baseorder_mempool_.DeAlloc(p_this_order_);
#ifdef _DBGLOGGER_SIM_ORDER_INFO_
          if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO)) {
            dbglogger_ << "SYM: " << p_this_order_->security_name_ << " Px: " << p_this_order_->price()
                       << " INTPX: " << p_this_order_->int_price()
                       << " BS: " << GetTradeTypeChar(p_this_order_->buysell()) << " ST: " << watch_.tv() << " DT: "
                       << watch_.tv() +
                              ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                              ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                       << " ORR: " << ToString(kORRType_Exec)
                       << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                       << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                       << " CLTPOS: " << t_client_position_ << " SACI: " << server_assigned_client_id_
                       << " GBLPOS: " << t_global_position_ << " SIZE: " << p_this_order_->size_remaining()
                       << " SE: " << p_this_order_->size_executed() << " MsgSeq : " << 0 << " Seq: " << 0
                       << DBGLOG_ENDL_FLUSH;
          }
#endif
        }
      }
    } else {
      DBGLOG_TIME_CLASS_FUNC << "WeirdOrder -  SACI: " << server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " bs: " << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " intpx: " << r_int_price_
                             << DBGLOG_ENDL_FLUSH;
      NotifyExecAndPosListeners(position_diff_, _price_, r_int_price_, _client_assigned_order_sequence_);
      return;
    }

    if (_size_remaining_ > 0) {
      NotifyExecAndPosListeners(position_diff_, _price_, r_int_price_, _client_assigned_order_sequence_);
      NotifyOrderChangeListeners(p_this_order_);  // TODO DO WE NEED TO CALL ORDER CHANGE LISTENERS HERE
#ifdef _DBGLOGGER_SIM_ORDER_INFO_
      if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO)) {
        dbglogger_ << "SYM: " << p_this_order_->security_name_ << " Px: " << p_this_order_->price()
                   << " INTPX: " << p_this_order_->int_price() << " BS: " << GetTradeTypeChar(p_this_order_->buysell())
                   << " ST: " << watch_.tv() << " DT: "
                   << watch_.tv() + ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                          ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                   << " ORR: " << ToString(kORRType_Exec)
                   << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                   << " CAOS: " << p_this_order_->client_assigned_order_sequence() << " CLTPOS: " << t_client_position_
                   << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << t_global_position_
                   << " SIZE: " << p_this_order_->size_remaining() << " SE: " << p_this_order_->size_executed()
                   << " MsgSeq : " << 0 << " Seq: " << 0 << DBGLOG_ENDL_FLUSH;
        LogFullStatus();
      }
#endif
    }

#ifdef _DBGLOGGER_FIL_TIME_INFO_
    if (dbglogger_.CheckLoggingLevel(FILL_TIME_INFO)) {
      std::cout << watch_.tv() << " OM " << sec_name_indexer_.GetShortcodeFromId(_security_id_) << " TRADE"
                 << " SACI: " << t_server_assigned_client_id_ << " CAOS: " << _client_assigned_order_sequence_
                 << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_ << " px: " << _price_
                 << " int(" << r_int_price_ << ")"
                 << " bs: " << GetTradeTypeChar(t_buysell_) << " sizeR " << _size_remaining_ << " sizeE "
                 << _size_executed_ << " cpos: " << t_client_position_ << " exec_id: " << p_this_order_->executioner_id_
                 << " gpos: " << t_global_position_ << std::endl;

      dbglogger_ << watch_.tv() << " OM " << sec_name_indexer_.GetShortcodeFromId(_security_id_) << " TRADE"
                 << " SACI: " << t_server_assigned_client_id_ << " CAOS: " << _client_assigned_order_sequence_
                 << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_ << " px: " << _price_
                 << " int(" << r_int_price_ << ")"
                 << " bs: " << GetTradeTypeChar(t_buysell_) << " sizeR " << _size_remaining_ << " sizeE "
                 << _size_executed_ << " cpos: " << t_client_position_ << " exec_id: " << p_this_order_->executioner_id_
                 << " gpos: " << t_global_position_ << DBGLOG_ENDL_FLUSH;
    }
#endif
  }
}

/**
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param t_buysell_
 * @param _size_remaining_
 * @param _rejection_reason_
 * @param r_int_price_
 * @param exchange_order_id
 * @param time_set_by_server
 */
void BasicOrderManager::OrderRejected(const int t_server_assigned_client_id_,
                                      const int _client_assigned_order_sequence_, const unsigned int _security_id_,
                                      const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                                      const int _rejection_reason_, const int r_int_price_,
                                      const uint64_t throttle_wait_cycle, const ttime_t time_set_by_server) {
  if (t_server_assigned_client_id_ == server_assigned_client_id_) {  // the rejected order was sent by me :(

    BASE_ORDER_MAP* order_map = NULL;
    if (t_buysell_ == kTradeTypeBuy) {
      order_map = &bid_order_id_to_order_map_;
    } else {
      order_map = &ask_order_id_to_order_map_;
    }

    // Start/Increment Counter
    if (HFSAT::kExchOrderReject == (HFSAT::ORSRejectionReason_t)_rejection_reason_) {
      number_of_consecutive_exchange_rejects_++;
      time_based_total_exchange_rejects_++;
      total_exchange_rejects_++;
    } else if (HFSAT::kExchDataEntryOrderReject == (HFSAT::ORSRejectionReason_t)_rejection_reason_){
      // Freeze Strat Here... 
      number_of_consecutive_exchange_rejects_++;
      time_based_total_exchange_rejects_++;
      total_exchange_rejects_++;
      is_dataentry_freeze = true;
     }


    if (HFSAT::kORSRejectThrottleLimitReached == (HFSAT::ORSRejectionReason_t)_rejection_reason_) {
      min_throttle_wait_cycle = GetReadTimeStampCounter() + throttle_wait_cycle;
      time_based_total_throttles_++;
    }

    // Special handling considered for auto freeze
    if (HFSAT::kORSRejectSecurityNotFound == (HFSAT::ORSRejectionReason_t)_rejection_reason_) {
      NotifyExchangeRejectsListeners(HFSAT::BaseUtils::FreezeEnforcedReason::kFreezeOnORSSecNotFound);
      // If we've not reset the rejects and we've breached threshold
      // our freeze timeout starts now
      if (0 == reject_based_freeze_timer_) {
        reject_based_freeze_timer_ = watch_.msecs_from_midnight();
      }
    }

    if (HFSAT::kORSRejectMarginCheckFailedMaxPosition == (HFSAT::ORSRejectionReason_t)_rejection_reason_) {
      NotifyExchangeRejectsListeners(HFSAT::BaseUtils::FreezeEnforcedReason::kFreezeOnORSMaxPos);
      // ORS Max Position breached
      if (0 == reject_based_freeze_timer_) {
        reject_based_freeze_timer_ = watch_.msecs_from_midnight();
      }
    }

    if (HFSAT::kORSRejectMarginCheckFailedWorstCasePosition == (HFSAT::ORSRejectionReason_t)_rejection_reason_) {
      NotifyExchangeRejectsListeners(HFSAT::BaseUtils::FreezeEnforcedReason::kFreezeOnORSWorstPos);
      // ORS Worst Position breached
      if (0 == reject_based_freeze_timer_) {
        reject_based_freeze_timer_ = watch_.msecs_from_midnight();
      }
    }

    if (!is_risk_checks_hit_) {
      if ((HFSAT::kORSRejectFailedPnlCheck == (HFSAT::ORSRejectionReason_t)_rejection_reason_) ||
          (HFSAT::kORSRejectFailedGrossMarginCheck == (HFSAT::ORSRejectionReason_t)_rejection_reason_) ||
          (HFSAT::kORSRejectFailedNetMarginCheck == (HFSAT::ORSRejectionReason_t)_rejection_reason_)) {
        is_risk_checks_hit_ = true;
        if (is_auto_freeze_active_) {
          for (uint32_t listener_counter = 0; listener_counter < exch_rejects_listener_vec_.size();
               listener_counter++) {
            exch_rejects_listener_vec_[listener_counter]->OnGetFreezeDueToExchangeRejects(
                HFSAT::BaseUtils::FreezeEnforcedReason::kFreezeOnRiskCheckHit);
          }
        }
      }
    }

    HFSAT::BaseUtils::FreezeEnforcedReason freeze_enforcement = query_trading_auto_freeze_manager_.ShouldAllowOrders(
        number_of_consecutive_exchange_rejects_, time_based_total_exchange_rejects_, total_exchange_rejects_,
        time_based_total_throttles_);

    if (HFSAT::BaseUtils::FreezeEnforcedReason::kAllow != freeze_enforcement) {
      NotifyExchangeRejectsListeners(freeze_enforcement);
      // If we've not reset the rejects and we've breached threshold
      // our freeze timeout starts now
      if (0 == reject_based_freeze_timer_) {
        reject_based_freeze_timer_ = watch_.msecs_from_midnight();
      }
    }

#ifdef _DBGLOGGER_OM_INFO_
    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      dbglogger_ << watch_.tv() << " " << sec_name_indexer_.GetShortcodeFromId(_security_id_) << " ORDER_REJECT"
                 << " SACI: " << t_server_assigned_client_id_ << " CAOS: " << _client_assigned_order_sequence_
                 << " sid: " << _security_id_ << " px: " << _price_ << " bs: " << GetTradeTypeChar(t_buysell_)
                 << " sizeR " << _size_remaining_ << " rejR "
                 << ORSRejectionReasonStr((ORSRejectionReason_t)_rejection_reason_) << " intpx: " << r_int_price_
                 << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;  // to make sure we see rejects
    }
#endif

    BaseOrder* p_this_order_ = nullptr; /* will stay nullptr till order found */
    auto iter = order_map->find(_client_assigned_order_sequence_);

    if (iter == order_map->end()) {
      DBGLOG_TIME_CLASS_FUNC << "WeirdOrder -  SACI: " << server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_ << " sid: " << _security_id_
                             << " bs: " << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " intpx: " << r_int_price_
                             << DBGLOG_ENDL_FLUSH;

      return;
    }

    p_this_order_ = iter->second;
    if (p_this_order_->is_ioc_ && p_this_order_->is_mirror_order_) {
      p_this_order_->num_outstanding_mirror_orders_--;
      if (t_buysell_ == kTradeTypeBuy) {
        bid_open_size_ = bid_open_size_ - _size_remaining_;
      } else {
        ask_open_size_ = ask_open_size_ - _size_remaining_;
      }
      if (p_this_order_->num_outstanding_mirror_orders_ == 0) {
        p_this_order_->order_status_ = kORRType_Rejc;
        NotifyOrderRejectListeners(p_this_order_);
        order_map->erase(iter);
        baseorder_mempool_.DeAlloc(p_this_order_);
      }
    } else {
      p_this_order_->order_status_ = kORRType_Rejc;
      if (t_buysell_ == kTradeTypeBuy) {
        bid_open_size_ = bid_open_size_ - _size_remaining_;
      } else {
        ask_open_size_ = ask_open_size_ - _size_remaining_;
      }
      NotifyOrderRejectListeners(p_this_order_);
      order_map->erase(iter);
      baseorder_mempool_.DeAlloc(p_this_order_);
    }

#ifdef _DBGLOGGER_SIM_ORDER_INFO_
    if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO)) {
      LogFullStatus();
    }
#endif
  }
}

void BasicOrderManager::OrderDataEntryFreezeDisable(){
  DBGLOG_TIME_CLASS_FUNC << "OrderDataEntryFreezeEnabled: " << dep_shortcode_ << " SecID: "
        << dep_security_id_ << " Freeze: " << is_dataentry_freeze << "\n";
  is_dataentry_freeze = false;
  DBGLOG_TIME_CLASS_FUNC << "OrderDataEntryFreezeDisable: " << dep_shortcode_ << " SecID: " 
	<< dep_security_id_ << " Freeze: " << is_dataentry_freeze << "\n";
}

// @ramkris: Implementation of OrderCancelReject in the Base Order Manager
// depending on different reasons of rejection
// 0 - Too late to reject
// 1 - Unknown Order
// 2-  Rejected from the ORS , so just set the cancel= false; again so that it can be sent to the ORS again
// 3 - Order already in the pending cancel or pending replace
// 6 - Duplicate ClOrdID received
void BasicOrderManager::OrderCancelRejected(const int t_server_assigned_client_id_,
                                            const int _client_assigned_order_sequence_,
                                            const int t_server_assigned_order_sequence_,
                                            const unsigned int _security_id_, const double _price_,
                                            const TradeType_t t_buysell_, const int _size_remaining_,
                                            const int _rejection_reason_, const int t_client_position_,
                                            const int t_global_position_, const int r_int_price_,
                                            const uint64_t throttle_wait_cycle, const ttime_t time_set_by_server) {
  global_position_ = t_global_position_;

  if (t_server_assigned_client_id_ == server_assigned_client_id_) {
    BASE_ORDER_MAP* order_map = NULL;
    if (t_buysell_ == kTradeTypeBuy) {
      order_map = &bid_order_id_to_order_map_;
    } else {
      order_map = &ask_order_id_to_order_map_;
    }

    // Start/Increment Counter
    if (HFSAT::kExchCancelReject == (HFSAT::CxlRejectReason_t)_rejection_reason_) {
      number_of_consecutive_exchange_rejects_++;
      time_based_total_exchange_rejects_++;
      total_exchange_rejects_++;
    }

    if (HFSAT::kCxlRejectReasonThrottle == (HFSAT::CxlRejectReason_t)_rejection_reason_) {
      min_throttle_wait_cycle = GetReadTimeStampCounter() + throttle_wait_cycle;
      time_based_total_throttles_++;
    }

    HFSAT::BaseUtils::FreezeEnforcedReason freeze_enforcement = query_trading_auto_freeze_manager_.ShouldAllowOrders(
        number_of_consecutive_exchange_rejects_, time_based_total_exchange_rejects_, total_exchange_rejects_,
        time_based_total_throttles_);

    if (HFSAT::BaseUtils::FreezeEnforcedReason::kAllow != freeze_enforcement) {
      NotifyExchangeRejectsListeners(freeze_enforcement);
      // If we've not reset the rejects and we've breached threshold
      // our freeze timeout starts now
      if (0 == reject_based_freeze_timer_) {
        reject_based_freeze_timer_ = watch_.msecs_from_midnight();
      }
    }

    BaseOrder* p_this_order_ = nullptr; /* will stay nullptr till order found */
    auto iter = order_map->find(_client_assigned_order_sequence_);

    if (iter == order_map->end()) {
      DBGLOG_TIME_CLASS_FUNC << "WeirdOrder -  SACI: " << server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_ << " sid: " << _security_id_
                             << " bs: " << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " intpx: " << r_int_price_
                             << DBGLOG_ENDL_FLUSH;

      return;
    }

    p_this_order_ = iter->second;
    if (_rejection_reason_ == kCxlRejectReasonTooLate || _rejection_reason_ == kCxlRejectReasonUnknownOrder) {
      // TODO This case should never trigger. We should just unset the canceled_ flag here
      order_map->erase(iter);
      p_this_order_->order_status_ = kORRType_Exec;
      NotifyOrderChangeListeners(p_this_order_);
      baseorder_mempool_.DeAlloc(p_this_order_);
    } else if (_rejection_reason_ == kCxlRejectReasonOrderNotConfirmed ||
               _rejection_reason_ == kCxlRejectReasonOrderAlreadyInPendingQueue ||
               _rejection_reason_ == kCxlRejectReasonDuplicateClOrdID ||
               _rejection_reason_ == kCxlRejectReasonThrottle || _rejection_reason_ == kCxlRejectReasonTAPThrottle ||
               _rejection_reason_ == kCxlRejectReasonMarketClosed || _rejection_reason_ == kCxlRejectReasonOther || 
	       _rejection_reason_ == HFSAT::kExchCancelReject) {
      p_this_order_ = iter->second;
      p_this_order_->canceled_ = false;
    }

#ifdef _DBGLOGGER_OM_INFO_
    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      dbglogger_ << watch_.tv() << " " << sec_name_indexer_.GetShortcodeFromId(_security_id_) << " CANCEL_REJECT"
                 << " SACI: " << t_server_assigned_client_id_ << " CAOS: " << _client_assigned_order_sequence_
                 << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_ << " px: " << _price_
                 << " bs: " << GetTradeTypeChar(t_buysell_) << " sizeR " << _size_remaining_ << " CrejR "
                 << CancelRejectReasonStr((CxlRejectReason_t)_rejection_reason_) << " cpos: " << t_client_position_
                 << " gpos: " << t_global_position_ << " intpx: " << r_int_price_ << DBGLOG_ENDL_FLUSH;
#ifdef _DBGLOGGER_SIM_ORDER_INFO_
      if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO) && p_this_order_ != nullptr)  // zero logging
      {
        dbglogger_ << "SYM: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " Px: " << _price_
                   << " INTPX: " << p_this_order_->int_price() << " BS: " << GetTradeTypeChar(t_buysell_)
                   << " ST: " << watch_.tv() << " DT: "
                   << watch_.tv() + ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                          ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                   << " ORR: " << ToString(kORRType_CxlRejc) << " SAOS: " << t_server_assigned_order_sequence_
                   << " CAOS: " << _client_assigned_order_sequence_ << " CLTPOS: " << t_client_position_
                   << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << t_global_position_
                   << " SIZE: " << _size_remaining_ << " SE: " << 0 << " MsgSeq : " << 0 << " Seq: " << 0
                   << DBGLOG_ENDL_FLUSH;
      }
      LogFullStatus();
#endif
    }
#endif
  }
}

std::string BasicOrderManager::ShowOrders() const {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << "Bids" << std::endl;

  for (auto base_order_ : bid_order_id_to_order_map_) {
    BaseOrder* p_this_order_ = base_order_.second;
    if (p_this_order_ != nullptr) {
      if (p_this_order_->is_ioc_ && p_this_order_->is_mirror_order_) {
        t_temp_oss_ << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price() << " "
                    << " Outstanding mirror orders " << p_this_order_->num_outstanding_mirror_orders_
                    << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                    << " sA: " << p_this_order_->queue_size_ahead() << " sB: " << p_this_order_->queue_size_behind()
                    << std::endl;
      } else {
        t_temp_oss_ << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price() << " "
                    << p_this_order_->size_remaining() << " " << ToString(p_this_order_->order_status())
                    << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                    << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                    << " MP: " << p_this_order_->modified_new_price_ << " Modified: " << p_this_order_->modified_
                    << " Canceled: " << p_this_order_->canceled_
                    << " NumOpen: " << p_this_order_->num_open_modified_orders_ << std::endl;
      }
    }
  }

  t_temp_oss_ << "Asks" << std::endl;

  for (auto base_order_ : ask_order_id_to_order_map_) {
    BaseOrder* p_this_order_ = base_order_.second;
    if (p_this_order_ != nullptr) {
      if (p_this_order_->is_ioc_ && p_this_order_->is_mirror_order_) {
        t_temp_oss_ << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price() << " "
                    << " Outstanding mirror orders " << p_this_order_->num_outstanding_mirror_orders_
                    << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                    << " sA: " << p_this_order_->queue_size_ahead() << " sB: " << p_this_order_->queue_size_behind()
                    << std::endl;
      } else {
        t_temp_oss_ << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price() << " "
                    << p_this_order_->size_remaining() << " " << ToString(p_this_order_->order_status())
                    << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                    << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                    << " MP: " << p_this_order_->modified_new_price_ << " Modified: " << p_this_order_->modified_
                    << " Canceled: " << p_this_order_->canceled_
                    << " NumOpen: " << p_this_order_->num_open_modified_orders_ << std::endl;
      }
    }
  }

  return t_temp_oss_.str();
}

void BasicOrderManager::LogFullStatus() {
  dbglogger_ << watch_.tv() << " "
             << " ShowOrders: " << ShowOrders() << DBGLOG_ENDL_FLUSH;
}
void BasicOrderManager::setMargin( double margin_) {
  dbglogger_ << watch_.tv() << "  current margin:: "<< margin_ << "\n" <<DBGLOG_ENDL_FLUSH;

  if((!is_magin_breach_risk_checks_hit_) && (margin_ >= exchange_margin_freeze_threshold_)) {
    is_magin_breach_risk_checks_hit_ = true;
    for (uint32_t listener_counter = 0; listener_counter < exch_rejects_listener_vec_.size();
          listener_counter++) {
      exch_rejects_listener_vec_[listener_counter]->OnGetFreezeDueToExchangeRejects(
      HFSAT::BaseUtils::FreezeEnforcedReason::kFreezeOnMarginBreach);
    }
  } else if((is_magin_breach_risk_checks_hit_) && (margin_ <= exchange_margin_release_threshold_)) {
    is_magin_breach_risk_checks_hit_ = false;
    for (uint32_t listener_counter = 0; listener_counter < exch_rejects_listener_vec_.size();
          listener_counter++) {
      exch_rejects_listener_vec_[listener_counter]->OnResetByManualInterventionOverRejects(
      HFSAT::BaseUtils::FreezeEnforcedReason::kFreezeOnMarginBreach);
    } 
  } else {
    for (uint32_t listener_counter = 0; listener_counter < margin_listener_vec_.size();
          listener_counter++) {
      margin_listener_vec_[listener_counter]->OnMarginChange(margin_);
    } 
  }

//  std::cout<<"current margin:: "<< margin_ << std::endl;
}
}
