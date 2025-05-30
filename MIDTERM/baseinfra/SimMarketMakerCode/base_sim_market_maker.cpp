/**
   \file OrderRoutingCode/base_sim_market_maker.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/SimMarketMaker/base_sim_market_maker.hpp"

namespace HFSAT {

std::string BaseSimMarketMaker::GetSameGatewayShortcodeIfRequired(const std::string& dep_shortcode, int tradingdate) {
  ExchSource_t t_exch_source_ = HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate)
                                    .GetContractSpecification(dep_shortcode, tradingdate)
                                    .exch_source_;
  TradingLocation_t dep_trading_location = TradingLocationUtils::GetTradingLocationExch(t_exch_source_);

  auto trading_location_file_read = ORSMessageFileNamer::GetName(
      ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(dep_shortcode), tradingdate, dep_trading_location);

  if (FileUtils::ExistsAndReadable(trading_location_file_read)) {
    return dep_shortcode;
  }

  std::vector<std::string> shortcode_list_ = SecurityDelayStats::GetShortCodesInSameGateway(dep_shortcode);

  for (auto& shortcode : shortcode_list_) {
    // dep_shortcode trading_location is same as shortcode trading-location
    TradingLocation_t _dep_trading_location = TradingLocationUtils::GetTradingLocationExch(t_exch_source_);
    auto trading_location_file_read = ORSMessageFileNamer::GetName(
        ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(shortcode), tradingdate, _dep_trading_location);

    if (FileUtils::ExistsAndReadable(trading_location_file_read)) {
      return shortcode;
    }
  }

  return dep_shortcode;
}

BaseSimMarketMaker::BaseSimMarketMaker(DebugLogger& dbglogger, Watch& watch, SecurityMarketView& dep_market_view,
                                       MarketModel market_model, HFSAT::SimTimeSeriesInfo& sim_time_series_info)
    : dbglogger_(dbglogger),
      watch_(watch),
      smv_(dep_market_view),
      dep_shortcode_(dep_market_view.shortcode()),
      dep_security_id_(dep_market_view.security_id()),
      dep_symbol_(dep_market_view.secname()),
      market_model_(market_model),
      process_mkt_updates_(true),
      is_cancellable_before_confirmation(GetIsCancellableBeforeConfirmation()),
      sim_time_series_info_(sim_time_series_info),
      basesimorder_mempool_(),
      all_requests_(),
      pending_requests_(),
      all_requests_busy_(false),
      server_assigned_order_sequence_(1),
      order_not_found_listener_vec_(),
      order_sequenced_listener_vec_(),
      order_confirmed_listener_vec_(),
      order_conf_cxlreplaced_listener_vec_(),
      order_canceled_listener_vec_(),
      order_executed_listener_vec_(),
      client_position_map_(),
      global_position_(0),
      global_position_to_send_map_(),

      sid_to_time_to_seqd_to_conf_times_(sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_),
      sid_to_time_to_conf_to_market_delay_times_(sim_time_series_info_.sid_to_time_to_conf_to_market_delay_times_),
      sid_to_time_to_cxl_seqd_to_conf_times_(sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_),

      sid_to_sim_config_(sim_time_series_info_.sid_to_sim_config_),
      retail_offer_map_(),
      current_matching_algo_(kFIFO),
      fifo_matching_fraction_(1.0),
      security_delay_stats_(nullptr),
      config_(sid_to_sim_config_[dep_security_id_]),
      real_saci_map_(),
      saci_to_secid_(),
      current_secid_(-1),
      saos_to_wakeup_map_() {
  lower_bound_usecs_ = market_model.com_usecs_ / 20;
  if (config_.order_level_sim_delay_low_ != 0) {
    lower_bound_usecs_ = config_.order_level_sim_delay_low_;
  }

  upper_bound_usecs_ = market_model.com_usecs_ * 20;
  if (config_.order_level_sim_delay_high_ != 250000 && config_.order_level_sim_delay_high_ != 0) {
    upper_bound_usecs_ = config_.order_level_sim_delay_high_;
  }

  normal_usecs_ = market_model.com_usecs_;
  SecurityDelayStats::LoadGatewayShcMapping(watch.YYYYMMDD());
  std::string shortcode = GetSameGatewayShortcodeIfRequired(dep_shortcode_, watch_.YYYYMMDD());
  SecurityNameIndexer& sec_name_indexer_ = SecurityNameIndexer::GetUniqueInstance();
  int security_id = sec_name_indexer_.GetIdFromString(shortcode);

  // Security id does not exist
  if (security_id == -1) {
    sec_name_indexer_.AddString(ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(shortcode), shortcode);
    security_id = sec_name_indexer_.GetIdFromString(shortcode);
  }

  security_delay_stats_ =
      &SecurityDelayStats::GetUniqueInstance(&dep_market_view, watch_,
                                             TradingLocationUtils::GetTradingLocationExch(smv_.exch_source()),
                                             market_model.com_usecs_);

  median_conf_delay_ = security_delay_stats_->GetMedianConfDelay();
  median_mkt_delay_ = security_delay_stats_->GetMedianMktDelay();
  if (dbglogger_.CheckLoggingLevel(ORS_DATA_INFO)) {
    security_delay_stats_->PrintAllDelays();
  }

  smv_.subscribe_MktStatus(this);
}

void BaseSimMarketMaker::OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_) {
  switch (_new_market_status_) {
    case kMktTradingStatusOpen: {
      process_mkt_updates_ = true;
    } break;
    case kMktTradingStatusClosed: {
      process_mkt_updates_ = false;

    } break;
    case kMktTradingStatusPreOpen:
    case kMktTradingStatusReserved: {
      process_mkt_updates_ = false;
    } break;
    case kMktTradingStatuFinalClosingCall: {
    } break;
    case kMktTradingStatusUnknown:
    case kMktTradingStatusForbidden: {
      process_mkt_updates_ = false;
    } break;
    default: { break; }
  }
}

void BaseSimMarketMaker::SimulateRetailTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                             const int t_trade_size_, const TradeType_t t_buysell_) {
  // currently not being used, but can be called from OnTimPeriodUpdate or some other class to simulate a retail-flow

  // here t_buysell_ is from our perspective and not from broker's

  if (t_security_id_ != dep_security_id_ || fporder_executed_listener_vec_.empty()) {
    return;
  }

  double order_placing_prob_ = sid_to_sim_config_[t_security_id_].retail_order_placing_prob_;

  // can be defined by derived classes
  // for order execution
  if (!RandomNumberGenerator::GetSuccess(order_placing_prob_) ||
      t_trade_size_ > sid_to_sim_config_[t_security_id_].retail_max_order_size_to_execute_) {
    return;
  }

  HFSAT::SecurityMarketView* p_smv_ =
      HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance().GetSecurityMarketView(dep_shortcode_);
  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "t_security_id_: " << t_security_id_ << " t_trade_price_: " << t_trade_price_
                           << " t_trade_size_: " << t_trade_size_ << " t_buysell_: " << t_buysell_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME_CLASS_FUNC << "mkt: " << p_smv_->bestbid_size() << " X " << p_smv_->bestbid_price() << " "
                           << p_smv_->bestask_price() << " X " << p_smv_->bestask_size() << DBGLOG_ENDL_FLUSH;
  }

  int retail_server_assigned_client_id_ = 0;
  double selected_price_;
  int trade_size_ = -1;

  for (std::map<int, HFSAT::CDef::RetailOffer>::iterator it = retail_offer_map_.begin(); it != retail_offer_map_.end();
       it++) {
    retail_server_assigned_client_id_ = it->first;
    HFSAT::CDef::RetailOffer this_retail_offer_ = it->second;

    if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "SACI: " << retail_server_assigned_client_id_
                             << " offer: " << this_retail_offer_.ToString() << DBGLOG_ENDL_FLUSH;
    }

    if (t_buysell_ == kTradeTypeSell) {
      if (p_smv_->DblPxCompare(this_retail_offer_.offered_ask_price_, p_smv_->ask_price(0))) {
        if (t_trade_price_ >= this_retail_offer_.offered_ask_price_) {
          if (t_trade_size_ <= this_retail_offer_.offered_ask_size_) {
            selected_price_ = this_retail_offer_.offered_ask_price_;
            trade_size_ = t_trade_size_;
          }
        }
      }
    } else if (t_buysell_ == kTradeTypeBuy) {
      if (p_smv_->DblPxCompare(this_retail_offer_.offered_bid_price_, p_smv_->bid_price(0))) {
        if (t_trade_price_ <= this_retail_offer_.offered_bid_price_) {
          if (t_trade_size_ <= this_retail_offer_.offered_bid_size_) {
            selected_price_ = this_retail_offer_.offered_bid_price_;
            trade_size_ = t_trade_size_;
          }
        }
      }
    }

    if (trade_size_ > 0) {
      for (size_t i = 0; i < fporder_executed_listener_vec_.size(); i++) {
        fporder_executed_listener_vec_[i]->FPOrderExecuted(dep_symbol_, selected_price_, t_buysell_, trade_size_);
      }
    }
  }
}

void BaseSimMarketMaker::OnRetailOfferUpdate(unsigned int _security_id_, const std::string& _shortcode_,
                                             const std::string& _secname_, const int _server_assigned_client_id_,
                                             const HFSAT::CDef::RetailOffer& _retail_offer_) {
  if (_security_id_ != dep_security_id_) {
    return;  ///< should never be here
  }

  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "SACI: " << _server_assigned_client_id_ << " for sec: " << _secname_
                           << " offer: " << _retail_offer_.ToString() << DBGLOG_ENDL_FLUSH;
  }

  retail_offer_map_[_server_assigned_client_id_] = _retail_offer_;
}

int BaseSimMarketMaker::GetSizeAtIntPrice(TradeType_t buysell, int int_price) {
  if (buysell == kTradeTypeBuy) {
    return smv_.bid_size_at_int_price(int_price);
  } else if (buysell == kTradeTypeSell) {
    return smv_.ask_size_at_int_price(int_price);
  }

  return 0;
}

const ttime_t BaseSimMarketMaker::GetComTimeAtTime(const ttime_t t_time_) {
  ttime_t t_com_time_(0, market_model_.com_usecs_);

  DelayOutput seqd_delay;
  // GetSendConfDelay will return nullptr in case data is not available
  if (sid_to_sim_config_[dep_security_id_].use_accurate_seqd_to_conf_ &&
      security_delay_stats_->GetSendConfDelay(t_time_, seqd_delay,
                                              sid_to_sim_config_[dep_security_id_].seq2conf_multiplier_,
                                              sid_to_sim_config_[dep_security_id_].seq2conf_addend_)) {
    t_com_time_ = seqd_delay.delay;
  }

  ttime_t max_com_time_ = ttime_t(0, upper_bound_usecs_);
  ttime_t min_com_time_ = ttime_t(0, lower_bound_usecs_);

  if (t_com_time_ < min_com_time_) {
    t_com_time_ = min_com_time_;
  }
  if (max_com_time_ < t_com_time_) {
    t_com_time_ = max_com_time_;
  }

  return t_com_time_;
}

const ttime_t BaseSimMarketMaker::GetCxlComTimeAtTime(const ttime_t t_time_) {
  ttime_t t_com_time_ = ttime_t(0, market_model_.com_usecs_);
  if (!sid_to_sim_config_[dep_security_id_].use_accurate_seqd_to_conf_) {
    t_com_time_ = ttime_t(0, market_model_.com_usecs_);
  } else {
    t_com_time_ = GetComTimeAtTime(t_time_);
  }

  ttime_t max_com_time_ = ttime_t(0, upper_bound_usecs_);
  ttime_t min_com_time_ = ttime_t(0, lower_bound_usecs_);

  if (t_com_time_ < min_com_time_) {
    t_com_time_ = min_com_time_;
  }
  if (max_com_time_ < t_com_time_) {
    t_com_time_ = max_com_time_;
  }

  return t_com_time_;
}

const ttime_t BaseSimMarketMaker::GetConfMarketDelayAtTime(const ttime_t t_time_) {
  ttime_t t_com_time_(0, market_model_.com_usecs_);
  if (!sid_to_time_to_conf_to_market_delay_times_[dep_security_id_].empty()) {
    std::map<ttime_t, ttime_t>& dep_time_to_conf_to_market_delay_times_ =
        sid_to_time_to_conf_to_market_delay_times_[dep_security_id_];

    std::map<ttime_t, ttime_t>::iterator _itr_ = dep_time_to_conf_to_market_delay_times_.begin();

    for (; _itr_ != dep_time_to_conf_to_market_delay_times_.end() && _itr_->first <= t_time_;) {
      dep_time_to_conf_to_market_delay_times_.erase(_itr_);
      _itr_ = dep_time_to_conf_to_market_delay_times_.begin();
    }

    if (_itr_ != dep_time_to_conf_to_market_delay_times_.begin() &&
        _itr_ != dep_time_to_conf_to_market_delay_times_.end()) {
      --_itr_;
    }

    if (_itr_ != dep_time_to_conf_to_market_delay_times_.end()) {
      t_com_time_ = _itr_->second;
    }
  }

  ttime_t max_com_time_ = ttime_t(0, 12 * market_model_.com_usecs_);
  ttime_t min_com_time_ = ttime_t(0, (int)(market_model_.com_usecs_ / 10.0));

  if (t_com_time_ < min_com_time_) {
    t_com_time_ = min_com_time_;
  }
  if (max_com_time_ < t_com_time_) {
    t_com_time_ = max_com_time_;
  }

  if (!sid_to_sim_config_[dep_security_id_].use_accurate_seqd_to_conf_) {
    t_com_time_ = ttime_t(0, market_model_.com_usecs_);
  }

  return t_com_time_;
}

void BaseSimMarketMaker::BroadcastRejection(const int saci, const BaseSimOrder* sim_order,
                                            ORSRejectionReason_t reject_reason) {
  for (auto rejc_listener : order_rejected_listener_vec_) {
    rejc_listener->OrderRejected(saci, sim_order->client_assigned_order_sequence(), dep_security_id_,
                                 sim_order->price(), sim_order->buysell(), sim_order->size_remaining(), reject_reason,
                                 sim_order->int_price_, 0, watch_.tv());
  }
}

void BaseSimMarketMaker::BroadcastSequenced(const int saci, const BaseSimOrder* sim_order) {
  for (auto seqd_listener : order_sequenced_listener_vec_) {
    seqd_listener->OrderSequenced(saci, sim_order->client_assigned_order_sequence(),
                                  sim_order->server_assigned_order_sequence(), dep_security_id_, sim_order->price(),
                                  sim_order->buysell(), sim_order->size_remaining(), sim_order->size_executed(),
                                  client_position_map_[saci], global_position_to_send_map_[saci], sim_order->int_price_,
                                  0, 0, watch_.tv());
  }
}

void BaseSimMarketMaker::BroadcastConfirm(const int saci, const BaseSimOrder* sim_order) {
  for (auto conf_listener : order_confirmed_listener_vec_) {
    conf_listener->OrderConfirmed(sim_order->server_assigned_client_id(), sim_order->client_assigned_order_sequence(),
                                  sim_order->server_assigned_order_sequence(), dep_security_id_, sim_order->price(),
                                  sim_order->buysell(), sim_order->size_remaining(), sim_order->size_executed(),
                                  client_position_map_[saci], global_position_to_send_map_[saci], sim_order->int_price_,
                                  0, 0, watch_.tv());
  }
}

void BaseSimMarketMaker::BroadcastCancelNotification(const int saci, const BaseSimOrder* sim_order) {
  for (auto cxl_listener : order_canceled_listener_vec_) {
    cxl_listener->OrderCanceled(sim_order->server_assigned_client_id(), sim_order->client_assigned_order_sequence(),
                                sim_order->server_assigned_order_sequence(), dep_security_id_, sim_order->price(),
                                sim_order->buysell(), sim_order->size_remaining(), client_position_map_[saci],
                                global_position_to_send_map_[saci], sim_order->int_price_, 0, 0, watch_.tv());
  }
}

void BaseSimMarketMaker::BroadcastExecNotification(const int saci, const BaseSimOrder* sim_order) {
  for (auto exec_listener : order_executed_listener_vec_) {
    exec_listener->OrderExecuted(sim_order->server_assigned_client_id(), sim_order->client_assigned_order_sequence(),
                                 sim_order->server_assigned_order_sequence(), dep_security_id_, sim_order->price(),
                                 sim_order->buysell(), sim_order->size_remaining(), sim_order->size_executed(),
                                 client_position_map_[saci], global_position_to_send_map_[saci], sim_order->int_price_,
                                 0, 0, watch_.tv());
  }
}

void BaseSimMarketMaker::BroadcastOrderNone(const int saci, const int caos, const int saos, const char* exchange_symbol,
                                            const double price, const int int_price, const TradeType_t buysell,
                                            const int size_remaining, const int size_executed) {}

void BaseSimMarketMaker::BroadcastOrderModifyRejectNotification(
    const int server_assigned_client_id, const int client_assigned_order_sequence,
    const int server_assigned_order_sequence, const unsigned int security_id, const double price,
    const TradeType_t buysell, const int size_remaining, const int client_position, const int global_position,
    const int intprice, const int32_t server_assigned_message_sequence) {
  for (auto cxl_replace_rejected_listener : order_conf_cxlreplace_rejected_listener_vec_) {
    cxl_replace_rejected_listener->OrderConfCxlReplaceRejected(
        server_assigned_client_id, client_assigned_order_sequence, server_assigned_order_sequence, security_id, price,
        buysell, size_remaining, client_position, global_position, intprice, 0, 0, 0, watch_.tv());
  }
}

void BaseSimMarketMaker::BroadcastOrderModifyNotification(const int _server_assigned_client_id_,
                                                          const BaseSimOrder* p_sim_order_, const double _new_price_,
                                                          const int _new_int_price_, const int _new_size_) {
  for (auto cxl_replaced_listener_ : order_conf_cxlreplaced_listener_vec_) {
    cxl_replaced_listener_->OrderConfCxlReplaced(
        _server_assigned_client_id_, p_sim_order_->client_assigned_order_sequence(),
        p_sim_order_->server_assigned_order_sequence(), dep_security_id_, _new_price_, p_sim_order_->buysell(),
        _new_size_, p_sim_order_->size_executed(), client_position_map_[_server_assigned_client_id_],
        global_position_to_send_map_[_server_assigned_client_id_], _new_int_price_, 0, 0, watch_.tv());
  }
}

void BaseSimMarketMaker::GetMatchingAlgoForShortcode(std::string shortcode, int tradingdate) {
  if (SecurityDefinitions::IsTimeProRataSimShortcode(shortcode, tradingdate)) {
    current_matching_algo_ = kTimeProRata;
  } else if (SecurityDefinitions::IsSimpleProRataSimShortcode(shortcode, tradingdate)) {
    current_matching_algo_ = kSimpleProRata;
  } else if (HFSAT::SecurityDefinitions::IsNewTimeProRataSimShortcode(shortcode, tradingdate)) {
    current_matching_algo_ = kNewTimeProRata;
  } else if (HFSAT::SecurityDefinitions::IsSplitFIFOProRataSimShortcode(shortcode, tradingdate)) {
    current_matching_algo_ = kSplitFIFOProRata;
    fifo_matching_fraction_ = SecurityDefinitions::GetFIFOPercentageForSimShortcode(shortcode, tradingdate);
  } else if (HFSAT::SecurityDefinitions::IsICEProRataSimShortcodeLFI(shortcode, tradingdate)) {
    current_matching_algo_ = kTimeProRata;
  } else if (HFSAT::SecurityDefinitions::IsICEProRataSimShortcodeLFL(shortcode, tradingdate)) {
    current_matching_algo_ = kNewTimeProRata;
  }
}

void BaseSimMarketMaker::addRequest(SimGenRequest& genrequest) {
  if (all_requests_busy_) {
    pending_requests_.push_back(genrequest);
  } else {
    all_requests_.push_back(genrequest);
    std::stable_sort(all_requests_.begin(), all_requests_.end());
  }
}

void BaseSimMarketMaker::SendOrderExch(const int saci, const char* security_name, const TradeType_t buysell,
                                       const double price, const int size_requested, const int int_price,
                                       const int caos, bool is_fok, bool is_ioc) {
  BaseSimOrder* new_order = basesimorder_mempool_.Alloc();

  new_order->security_name_ = security_name;
  new_order->buysell_ = buysell;
  new_order->price_ = price;

  new_order->size_remaining_ = size_requested;

  new_order->int_price_ = int_price;

  // not needed in SimMarketMaker so set to anything
  new_order->order_status_ = kORRType_None;
  new_order->queue_size_ahead_ = 0;
  new_order->queue_size_behind_ = 0;
  new_order->num_events_seen_ = 0;

  // used in replying to SendTrade
  new_order->client_assigned_order_sequence_ = caos;

  // should be made unique if multiple SMMs for same security.
  // Right now single SMM for every security
  new_order->server_assigned_order_sequence_ = server_assigned_order_sequence_++;

  // only place where BaseSimOrder::server_assigned_client_id_ is used
  new_order->server_assigned_client_id_ = saci;

  // For products like DOL IND we set min_order_size to be great than 1
  new_order->min_order_size_ = smv_.min_order_size();
  new_order->size_partial_executed_ = 0;
  new_order->size_executed_ = 0;

  // could be used to see when not to allow cancel
  new_order->alone_above_best_market_ = false;

  new_order->order_sequenced_time_ = watch_.tv();

  new_order->is_fok_ = is_fok;
  new_order->is_ioc_ = is_ioc;

  // Check for size_requested_ being a multiple of
  // min_order_size_ of the security
  if (size_requested % smv_.min_order_size() != 0) {
    BroadcastRejection(saci, new_order, kSendOrderRejectNotMinOrderSizeMultiple);
  }

  BroadcastSequenced(saci, new_order);

  SimGenRequest genrequest;

  ttime_t mkt_delay = GetSendOrderDelay(watch_.tv());
  genrequest.wakeup_time_ = watch_.tv() + mkt_delay;
  saos_to_wakeup_map_[new_order->server_assigned_order_sequence_] = genrequest.wakeup_time_;

  genrequest.orq_request_type_ = ORQ_SEND;
  genrequest.sreq_.ssor_.p_new_order_ = new_order;
  genrequest.postponed_once_ = false;
  genrequest.server_assigned_client_id_ = saci;

  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "SACI: " << saci << " " << security_name << " "
                           << (buysell == kTradeTypeBuy ? "BUY" : "SELL") << " " << price << " " << size_requested
                           << " intpx: " << int_price << " CAOS: " << caos << " Delay: " << mkt_delay
                           << " Wake Up: " << genrequest.wakeup_time_ << DBGLOG_ENDL_FLUSH;
  }

  addRequest(genrequest);
}

void BaseSimMarketMaker::CancelOrderExch(const int saci, const int saos, const TradeType_t buysell,
                                         const int int_price) {
  SimGenRequest genrequest;

  ttime_t mkt_delay = GetCancelOrderDelay(watch_.tv());
  genrequest.wakeup_time_ = watch_.tv() + mkt_delay;
  genrequest.orq_request_type_ = ORQ_CANCEL;

  std::unordered_map<int, ttime_t>::iterator it;
  it = saos_to_wakeup_map_.find(saos);
  if (it != saos_to_wakeup_map_.end()) {
    genrequest.wakeup_time_ = std::max(genrequest.wakeup_time_, it->second);
  }

  genrequest.sreq_.scor_.buysell_ = buysell;
  genrequest.sreq_.scor_.int_price_ = int_price;
  genrequest.sreq_.scor_.server_assigned_order_sequence_ = saos;

  genrequest.server_assigned_client_id_ = saci;
  genrequest.postponed_once_ = false;

  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "SACI: " << saci << " SAOS: " << saos << " "
                           << (buysell == kTradeTypeBuy ? "BUY" : "SELL") << " intpx: " << int_price
                           << " px: " << smv_.GetDoublePx(int_price) << " Wake Up: " << genrequest.wakeup_time_
                           << " cxl_delay: " << mkt_delay << DBGLOG_ENDL_FLUSH;
  }

  addRequest(genrequest);
}

void BaseSimMarketMaker::CancelReplaceOrderExch(const int saci, const int saos, const TradeType_t buysell,
                                                const double _old_price_, const int _old_int_price_,
                                                const int _old_size_, const double _new_price_,
                                                const int _new_int_price_, const int _new_size_) {
  SimGenRequest genrequest;
  genrequest.wakeup_time_ = watch_.tv() + GetCancelReplaceOrderDelay(watch_.tv());

  genrequest.orq_request_type_ = ORQ_CXLREPLACE;

  genrequest.sreq_.sxor_.server_assigned_order_sequence_ = saos;
  genrequest.sreq_.sxor_.buysell_ = buysell;
  genrequest.sreq_.sxor_.new_int_price_ = _new_int_price_;
  genrequest.sreq_.sxor_.new_size_ = _new_size_;
  genrequest.sreq_.sxor_.new_price_ = _new_price_;
  genrequest.sreq_.sxor_.old_int_price_ = _old_int_price_;
  genrequest.sreq_.sxor_.old_price_ = _old_price_;
  genrequest.sreq_.sxor_.old_size_ = _old_size_;

  genrequest.server_assigned_client_id_ = saci;
  genrequest.postponed_once_ = false;

  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "SACI: " << saci << " SAOS: " << saos << " "
                           << (buysell == kTradeTypeBuy ? "BUY" : "SELL") << " npx: " << _new_price_
                           << " nintpx: " << _new_int_price_ << " nsz: " << _new_size_
                           << " wakeup: " << genrequest.wakeup_time_ << DBGLOG_ENDL_FLUSH;
  }
  addRequest(genrequest);
}

void BaseSimMarketMaker::ReplayOrderExch(const int saci, const int caos, const TradeType_t buysell, const int int_price,
                                         const int saos) {
  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "SACI: " << saci << " CAOS: " << caos << " SAOS: " << saos << " "
                           << (buysell == kTradeTypeBuy ? "BUY" : "SELL") << " intpx: " << int_price
                           << DBGLOG_ENDL_FLUSH;
  }

  SimGenRequest genrequest;
  genrequest.wakeup_time_ = watch_.tv() + GetReplayOrderDelay(watch_.tv());
  genrequest.orq_request_type_ = ORQ_REPLAY;

  genrequest.sreq_.sror_.client_assigned_order_sequence_ = caos;
  genrequest.sreq_.sror_.buysell_ = buysell;
  genrequest.sreq_.sror_.int_price_ = int_price;
  genrequest.sreq_.sror_.server_assigned_order_sequence_ = saos;

  genrequest.server_assigned_client_id_ = saci;
  genrequest.postponed_once_ = false;

  addRequest(genrequest);
}

int BaseSimMarketMaker::ProcessBidSendRequest(BaseSimOrder* sim_order, bool re_enqueue) {
  int saci = sim_order->server_assigned_client_id();
  double prev_conf_price = sim_order->price();
  const int int_price = sim_order->int_price_;
  const int size = GetSizeAtIntPrice(kTradeTypeBuy, int_price);
  if (re_enqueue) {
    sim_order->queue_size_behind_ = 0;
    sim_order->queue_size_ahead_ = size;
  }
  sim_order->order_id_ = 0;

  // Check if this is an aggressive buy order
  if (int_price >= smv_.bestask_int_price()) {
    for (int this_int_price = smv_.bestask_int_price(); this_int_price <= int_price; this_int_price++) {
      // Execute the order, update position, send EXEC
      int ask_size = GetSizeAtIntPrice(kTradeTypeSell, this_int_price);

      if (intpx_agg_bid_size_map_[saci].find(this_int_price) != intpx_agg_bid_size_map_[saci].end()) {
        ask_size = ask_size - intpx_agg_bid_size_map_[saci][this_int_price].second;
      }

      // If we have already used this price level for some other aggressive order, skip this level
      if (ask_size <= 0) {
        continue;
      }

      sim_order->price_ = smv_.GetDoublePx(this_int_price);
      int size_executed = std::min(sim_order->size_remaining_, ask_size);

      if (size_executed == sim_order->size_remaining_) {
        // Full exec
        sim_order->ExecuteRemaining();
      } else {
        // Partial match and append sim order to the order_vec_
        size_executed = sim_order->MatchPartial(size_executed);
      }

      if (size_executed > 0) {
        LogExec(sim_order, size_executed);

        client_position_map_[saci] += size_executed;
        global_position_to_send_map_[saci] += size_executed;
        intpx_agg_bid_size_map_[saci][this_int_price].first = GetSizeAtIntPrice(kTradeTypeSell, this_int_price);
        intpx_agg_bid_size_map_[saci][this_int_price].second += size_executed;
        BroadcastExecNotification(saci, sim_order);
      }
    }
  }

  if (sim_order->size_remaining() <= 0) {
    return 0;
  }

  if (sim_order->is_ioc_) {
    BroadcastCancelNotification(saci, sim_order);
    return 0;
  }

  // Restore the order price for the remaining size
  sim_order->price_ = prev_conf_price;

  // Push to the order vector
  //  std::cout<<"Pushing the order in intpx_bid_order_map_ for int price: "<<int_price<<"\n";
  intpx_bid_order_map_[int_price].push_back(sim_order);
  //  std::cout<<"Size after pushing the order in intpx_bid_order_map_ for int price:
  //  "<<intpx_bid_order_map_[int_price].size()<<"\n";

  if (int_price > smv_.bestbid_int_price()) {
    sim_order->alone_above_best_market_ = true;
  }

  return 0;
}

int BaseSimMarketMaker::ProcessAskSendRequest(BaseSimOrder* sim_order, bool re_enqueue) {
  int saci = sim_order->server_assigned_client_id();
  double prev_conf_price = sim_order->price();
  const int int_price = sim_order->int_price_;
  const int size = GetSizeAtIntPrice(kTradeTypeSell, int_price);
  if (re_enqueue) {
    sim_order->queue_size_behind_ = 0;
    sim_order->queue_size_ahead_ = size;
  }
  sim_order->order_id_ = 0;

  // Check if this is an aggressive sell order
  if (int_price <= smv_.bestbid_int_price()) {
    for (int this_int_price = smv_.bestbid_int_price(); this_int_price >= int_price; this_int_price--) {
      // Execute the order, update position, send EXEC
      int bid_size = GetSizeAtIntPrice(kTradeTypeBuy, this_int_price);

      if (intpx_agg_ask_size_map_[saci].find(this_int_price) != intpx_agg_ask_size_map_[saci].end()) {
        bid_size = bid_size - intpx_agg_ask_size_map_[saci][this_int_price].second;
      }

      // If we have already used this price level for some other aggressive order, skip this level
      if (bid_size <= 0) {
        continue;
      }

      sim_order->price_ = smv_.GetDoublePx(this_int_price);

      int size_executed = std::min(sim_order->size_remaining_, bid_size);

      if (size_executed == sim_order->size_remaining_) {
        sim_order->ExecuteRemaining();
      } else {
        size_executed = sim_order->MatchPartial(size_executed);
      }

      if (size_executed > 0) {
        LogExec(sim_order, size_executed);
        client_position_map_[saci] -= size_executed;
        global_position_to_send_map_[saci] -= size_executed;
        intpx_agg_ask_size_map_[saci][this_int_price].first = GetSizeAtIntPrice(kTradeTypeBuy, this_int_price);
        intpx_agg_ask_size_map_[saci][this_int_price].second += size_executed;
        BroadcastExecNotification(saci, sim_order);
      }
    }
  }

  if (sim_order->size_remaining() <= 0) {
    return 0;
  }

  if (sim_order->is_ioc_) {
    BroadcastCancelNotification(saci, sim_order);
    return 0;
  }

  // Restore the order price for the remaining size
  sim_order->price_ = prev_conf_price;

  // Push to the order vector

  intpx_ask_order_map_[int_price].push_back(sim_order);

  if (int_price < smv_.bestask_int_price()) {
    sim_order->alone_above_best_market_ = true;
  }

  return 0;
}

int BaseSimMarketMaker::ProcessSendRequest(SimGenRequest& sim_genrequest) {
  int to_postpone = 0;
  BaseSimOrder* new_sim_order = sim_genrequest.sreq_.ssor_.p_new_order_;

  int saci = sim_genrequest.server_assigned_client_id_;
  if (saci >= int(saci_to_secid_.size()) ||
      find(saci_to_secid_[saci].begin(), saci_to_secid_[saci].end(), current_secid_) == saci_to_secid_[saci].end())
    return -1;

  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    int caos = new_sim_order->client_assigned_order_sequence();
    int saos = new_sim_order->server_assigned_order_sequence();
    DBGLOG_TIME_CLASS_FUNC << "ProcSendReq " << smv_.secname() << " " << GetTradeTypeChar(new_sim_order->buysell_)
                           << " CAOS: " << caos << " SAOS: " << saos << " SACI: " << saci << " at mkt: [ "
                           << smv_.bestbid_ordercount() << " " << smv_.bestbid_size() << " " << smv_.bestbid_price()
                           << " * " << smv_.bestask_price() << " " << smv_.bestask_size() << " "
                           << smv_.bestask_ordercount() << " ] " << DBGLOG_ENDL_FLUSH;
  }

  new_sim_order->Confirm();
  new_sim_order->order_confirmation_time_ = watch_.tv();

  BroadcastConfirm(saci, new_sim_order);

  switch (new_sim_order->buysell_) {
    case kTradeTypeBuy: {
      to_postpone = ProcessBidSendRequest(new_sim_order, true);
      break;
    }
    case kTradeTypeSell: {
      to_postpone = ProcessAskSendRequest(new_sim_order, true);
      break;
    }

    default:
      break;
  }

  return to_postpone;
}

int BaseSimMarketMaker::ProcessCancelRequest(SimGenRequest& sim_genrequest) {
  int to_postpone = 0;

  const int saos = sim_genrequest.sreq_.scor_.server_assigned_order_sequence_;
  const int saci = sim_genrequest.server_assigned_client_id_;

  if (saci >= int(saci_to_secid_.size()) ||
      find(saci_to_secid_[saci].begin(), saci_to_secid_[saci].end(), current_secid_) == saci_to_secid_[saci].end())
    return -1;
  const TradeType_t buysell = sim_genrequest.sreq_.scor_.buysell_;
  const int int_price = sim_genrequest.sreq_.scor_.int_price_;

  // find the order by server_assigned_order_sequence
  BaseSimOrder* sim_order = FetchOrder(buysell, int_price, saos);

  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO) && sim_order != NULL) {
    const int caos = sim_order->server_assigned_order_sequence();
    DBGLOG_TIME_CLASS_FUNC << "ProcCxlReq " << smv_.secname() << " " << GetTradeTypeChar(sim_order->buysell_)
                           << " CAOS: " << caos << " SAOS: " << saos << " SACI: " << saci << " at mkt: [ "
                           << smv_.bestbid_ordercount() << " " << smv_.bestbid_size() << " " << smv_.bestbid_int_price()
                           << " * " << smv_.bestask_int_price() << " " << smv_.bestask_size() << " "
                           << smv_.bestask_ordercount() << " ] " << DBGLOG_ENDL_FLUSH;
  }

  if (sim_order == NULL) {
    if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "Could not find order to cancel by " << GetTradeTypeChar(buysell) << " intpx "
                             << int_price << " SAOS: " << saos << " SACI: " << saci << DBGLOG_ENDL_FLUSH;
    }

    BroadcastOrderNone(saci, 0, saos, smv_.secname(), smv_.GetDoublePx(int_price), int_price, buysell, 0, 0);

    return to_postpone;
  }

  if (sim_order->server_assigned_client_id() != saci) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR: different saci: " << sim_order->server_assigned_client_id() << " " << saci
                           << DBGLOG_ENDL_FLUSH;

    return to_postpone;
  }

  if (buysell == kTradeTypeBuy) { /* BidSide order */

    if (!sim_genrequest.postponed_once_) {  // if never postponed then see if it needs to be postponed

      if (sim_order->int_price() < smv_.bestbid_int_price()) {          // not best market
      } else if (sim_order->int_price() == smv_.bestbid_int_price()) {  // at best market
        if (!is_cancellable_before_confirmation && (sim_order->num_events_seen() < 1) &&
            (!sim_order->IsConfirmed())) {  // order has seen less than 1 event
          // or is not even confirmed yet
          to_postpone = 1;  ///< postpone
        }
      } else {  // above best market

        if (!is_cancellable_before_confirmation && sim_order->num_events_seen() < 4) {
          to_postpone = 2;  ///< postpone
        } else {
          if (smv_.spread_increments() > 2 * smv_.normal_spread_increments()) {
            to_postpone = 3;  ///< postpone
          }
        }
      }
    }

    if (sim_order->IsExecuted()) {  // Prematurely executed , just waiting on the OnTradePrint call.
      to_postpone = 4;
    }

    if (to_postpone == 0) {
      VectorUtils::UniqueVectorRemove(intpx_bid_order_map_[sim_order->int_price()], sim_order);  ///< remove from map
      BroadcastCancelNotification(saci, sim_order);  ///< send cancel notification
      basesimorder_mempool_.DeAlloc(sim_order);
    } else if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      int caos = sim_order->server_assigned_order_sequence();
      DBGLOG_TIME_CLASS_FUNC << "Postponed cancelation " << to_postpone << " of " << sim_order->security_name() << ' '
                             << GetTradeTypeChar(sim_order->buysell()) << ' ' << sim_order->price() << ' '
                             << sim_order->size_remaining() << " sE:" << sim_order->size_executed() << ' '
                             << ToString(sim_order->order_status()) << " [" << sim_order->queue_size_ahead() << "-"
                             << sim_order->queue_size_behind() << '-' << sim_order->num_events_seen() << ']'
                             << " CAOS: " << caos << " SAOS: " << saos << " SACI: " << saci << DBGLOG_ENDL_FLUSH;
    }
  } else {
    if (!sim_genrequest.postponed_once_) {  // if never postponed then see if it needs to be postponed
      if (sim_order->int_price() > smv_.bestask_int_price()) {          // not best market
      } else if (sim_order->int_price() == smv_.bestask_int_price()) {  // at best market
        if (!is_cancellable_before_confirmation && (sim_order->num_events_seen() < 1) &&
            (!sim_order->IsConfirmed())) {  // order has seen less than 1 event
          // or is not even confirmed yet
          to_postpone = 1;  ///< postpone
        }
      } else {  // above best market
        if (!is_cancellable_before_confirmation && sim_order->num_events_seen() < 4) {
          to_postpone = 2;  ///< postpone
        } else {
          if (smv_.spread_increments() > 2 * smv_.normal_spread_increments()) {
            to_postpone = 3;  ///< postpone
          }
        }
      }
    }

    if (sim_order->IsExecuted()) {  // Prematurely executed , just waiting on the OnTradePrint call.
      to_postpone = 4;
    }

    if (to_postpone == 0) {
      ///< remove from map
      VectorUtils::UniqueVectorRemove(intpx_ask_order_map_[sim_order->int_price()], sim_order);

      ///< send cancel notification
      BroadcastCancelNotification(saci, sim_order);
      basesimorder_mempool_.DeAlloc(sim_order);
    } else if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      int caos_ = sim_order->server_assigned_order_sequence();
      DBGLOG_TIME_CLASS_FUNC << "Postponed cancelation " << to_postpone << " of " << sim_order->security_name() << ' '
                             << GetTradeTypeChar(sim_order->buysell()) << ' ' << sim_order->price() << ' '
                             << sim_order->size_remaining() << " sE:" << sim_order->size_executed() << ' '
                             << ToString(sim_order->order_status()) << " [" << sim_order->queue_size_ahead() << "-"
                             << sim_order->queue_size_behind() << '-' << sim_order->num_events_seen() << ']'
                             << " CAOS: " << caos_ << " SAOS: " << saos << " SACI: " << saci << DBGLOG_ENDL_FLUSH;
    }
  }

  return to_postpone;
}

BaseSimOrder* BaseSimMarketMaker::FetchOrder(const TradeType_t buysell, const int int_price, const int saos) {
  switch (buysell) {
    case kTradeTypeBuy: {
      if (intpx_bid_order_map_.find(int_price) != intpx_bid_order_map_.end()) {
        for (auto order : intpx_bid_order_map_[int_price]) {
          if (order->server_assigned_order_sequence() == saos) {
            return order;
          }
        }
      }

      for (auto& order_vec_pair : intpx_bid_order_map_) {
        for (auto order : order_vec_pair.second) {
          if (order->server_assigned_order_sequence() == saos) {
            return order;
          }
        }
      }
      break;
    }
    case kTradeTypeSell: {
      if (intpx_ask_order_map_.find(int_price) != intpx_ask_order_map_.end()) {
        for (auto order : intpx_ask_order_map_[int_price]) {
          if (order->server_assigned_order_sequence() == saos) {
            return order;
          }
        }
      }

      for (auto& order_vec_pair : intpx_bid_order_map_) {
        for (auto order : order_vec_pair.second) {
          if (order->server_assigned_order_sequence() == saos) {
            return order;
          }
        }
      }
      break;
    }
    default: { break; }
  }

  return NULL;
}

int BaseSimMarketMaker::ProcessCxlReplaceRequest(SimGenRequest& t_sim_genrequest_) {
  int to_postpone_ = 0;

  const int saos_ = t_sim_genrequest_.sreq_.sxor_.server_assigned_order_sequence_;
  const TradeType_t _buysell_ = t_sim_genrequest_.sreq_.sxor_.buysell_;
  const double _price_ = t_sim_genrequest_.sreq_.sxor_.new_price_;
  const int _int_price_ = t_sim_genrequest_.sreq_.sxor_.new_int_price_;
  const int _new_size_ = t_sim_genrequest_.sreq_.sxor_.new_size_;
  const int _old_int_price_ = t_sim_genrequest_.sreq_.sxor_.old_int_price_;
  int saci_ = t_sim_genrequest_.server_assigned_client_id_;

  if (saci_ >= int(saci_to_secid_.size()) ||
      find(saci_to_secid_[saci_].begin(), saci_to_secid_[saci_].end(), current_secid_) == saci_to_secid_[saci_].end())
    return -1;
  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    int caos = -1;
    int saos = saos_;
    int saci = saci_;

    DBGLOG_TIME_CLASS_FUNC << "ProcCxlReplaceReq " << smv_.secname() << " " << GetTradeTypeChar(_buysell_)
                           << " CAOS: " << caos << " SAOS: " << saos << " SACI: " << saci << " at mkt: [ "
                           << smv_.bestbid_ordercount() << " " << smv_.bestbid_size() << " " << smv_.bestbid_price()
                           << " * " << smv_.bestask_price() << " " << smv_.bestask_size() << " "
                           << smv_.bestask_ordercount() << " ] " << DBGLOG_ENDL_FLUSH;
  }

  BaseSimOrder* p_sim_order_ = FetchOrder(_buysell_, _old_int_price_, saos_);
  if ((p_sim_order_ != NULL) &&
      (p_sim_order_->server_assigned_client_id() == t_sim_genrequest_.server_assigned_client_id_)) {
    if (_buysell_ == kTradeTypeBuy) {
      // if never postponed then see if it needs to be postponed
      if (!t_sim_genrequest_.postponed_once_) {
        if (p_sim_order_->int_price() < smv_.bestbid_int_price()) {          // not best market
        } else if (p_sim_order_->int_price() == smv_.bestbid_int_price()) {  // at best market

          if ((p_sim_order_->num_events_seen() < 1) &&
              (!p_sim_order_->IsConfirmed())) {  // order has seen less than 1 event
            // or is not even confirmed yet
            to_postpone_ = 1;  ///< postpone
          } else {
          }
        } else {  // above best market
          if (p_sim_order_->num_events_seen() < 5) {
            to_postpone_ = 2;  ///< postpone
          } else {
            if (smv_.spread_increments() > 2 * smv_.normal_spread_increments()) {
              to_postpone_ = 3;  ///< postpone
            } else {
            }
          }
        }
      }

      if (to_postpone_ == 0) {
        if (_new_size_ > (int)p_sim_order_->size_remaining()) {
          p_sim_order_->ResetQueue();
        }
        VectorUtils::UniqueVectorRemove(intpx_bid_order_map_[p_sim_order_->int_price()],
                                        p_sim_order_);  ///< remove from map
        p_sim_order_->Modify(_price_, _int_price_, _new_size_);
        // intpx_bid_order_map_[p_sim_order_->int_price()].push_back(p_sim_order_);
        BroadcastOrderModifyNotification(saci_, p_sim_order_, _price_, _int_price_,
                                         _new_size_);  ///< send cancel notification
        /// Processing this as new order, Queue position will be lost
        /// Re-enqueing it all the time
        to_postpone_ = ProcessBidSendRequest(p_sim_order_, true);
      }
    } else {
      if (!t_sim_genrequest_.postponed_once_) {  // if never postponed then see if it needs to be postponed

        if (p_sim_order_->int_price() > smv_.bestask_int_price()) {
        }                                                                  // not best market
        else if (p_sim_order_->int_price() == smv_.bestask_int_price()) {  // at best market
          if ((p_sim_order_->num_events_seen() < 1) &&
              (!p_sim_order_->IsConfirmed())) {  // order has seen less than 1 event
            // or is not even confirmed yet
            to_postpone_ = 1;  ///< postpone
          } else {
          }
        } else {  ///< above best market
          if (p_sim_order_->num_events_seen() < 5) {
            to_postpone_ = 2;  ///< postpone
          } else {
            if (smv_.spread_increments() > 2 * smv_.normal_spread_increments()) {
              to_postpone_ = 3;  ///< postpone
            } else {
            }
          }
        }
      }

      if (to_postpone_ == 0) {
        if (_new_size_ > (int)p_sim_order_->size_remaining()) {
          p_sim_order_->ResetQueue();
        }
        VectorUtils::UniqueVectorRemove(intpx_ask_order_map_[p_sim_order_->int_price()],
                                        p_sim_order_);  ///< remove from map
        p_sim_order_->Modify(_price_, _int_price_, _new_size_);

        BroadcastOrderModifyNotification(saci_, p_sim_order_, _price_, _int_price_,
                                         _new_size_);  ///< send cancel notification
        ProcessAskSendRequest(p_sim_order_, true);
      }
    }
  } else if (p_sim_order_ == nullptr) {
    BroadcastOrderModifyRejectNotification(saci_, 0, saos_, smv_.security_id(), _price_, _buysell_, _new_size_,
                                           client_position_map_[saci_], global_position_, _int_price_, saos_);
  }

  return to_postpone_;
}

int BaseSimMarketMaker::ProcessReplayRequest(SimGenRequest& sim_genrequest) {
  int to_postpone = 0;

  const int caos = sim_genrequest.sreq_.sror_.client_assigned_order_sequence_;
  const TradeType_t buysell = sim_genrequest.sreq_.sror_.buysell_;
  const int int_price = sim_genrequest.sreq_.sror_.int_price_;
  const int saos = sim_genrequest.sreq_.sror_.server_assigned_order_sequence_;
  int saci_ = sim_genrequest.server_assigned_client_id_;

  if (saci_ >= int(saci_to_secid_.size()) ||
      find(saci_to_secid_[saci_].begin(), saci_to_secid_[saci_].end(), current_secid_) == saci_to_secid_[saci_].end())
    return -1;
  bool orderfound = false;
  if (saos != 0) {  // replaying an order where the client has received a kORRType_Seqd

    //   search in confirmed orders
    //   else BroadcastOrderNone
    switch (buysell) {
      case kTradeTypeBuy: {
        if (intpx_bid_order_map_.find(int_price) != intpx_bid_order_map_.end()) {
          const std::vector<BaseSimOrder*>& thisvec = intpx_bid_order_map_[int_price];

          for (size_t i = 0; i < thisvec.size(); i++) {
            if (thisvec[i]->server_assigned_order_sequence() == saos) {  // found order
              BroadcastConfirm(sim_genrequest.server_assigned_client_id_, thisvec[i]);
              orderfound = true;
            }
          }
        }
        break;
      }
      case kTradeTypeSell: {
        if (intpx_ask_order_map_.find(int_price) != intpx_ask_order_map_.end()) {
          const std::vector<BaseSimOrder*>& thisvec = intpx_ask_order_map_[int_price];

          for (size_t i = 0; i < thisvec.size(); i++) {
            if (thisvec[i]->server_assigned_order_sequence() == saos) {  // found order
              BroadcastConfirm(sim_genrequest.server_assigned_client_id_, thisvec[i]);
              orderfound = true;
            }
          }
        }
        break;
      }
      default:
        break;
    }
  } else {
    //   search in confirmed orders
    //   else search in sequenced requests
    //   else BroadcastOrderNone
    switch (buysell) {
      case kTradeTypeBuy: {
        if (intpx_bid_order_map_.find(int_price) != intpx_bid_order_map_.end()) {
          const std::vector<BaseSimOrder*>& _thisvec_ = intpx_bid_order_map_[int_price];

          for (size_t i = 0; i < _thisvec_.size(); i++) {
            if (_thisvec_[i]->server_assigned_order_sequence() == saos)  // found order
            {
              BroadcastConfirm(sim_genrequest.server_assigned_client_id_, _thisvec_[i]);
              orderfound = true;
            }
          }
        }
      } break;
      case kTradeTypeSell: {
        if (intpx_ask_order_map_.find(int_price) != intpx_ask_order_map_.end()) {
          const std::vector<BaseSimOrder*>& _thisvec_ = intpx_ask_order_map_[int_price];

          for (size_t i = 0; i < _thisvec_.size(); i++) {
            if (_thisvec_[i]->server_assigned_order_sequence() == saos)  // found order
            {
              BroadcastConfirm(sim_genrequest.server_assigned_client_id_, _thisvec_[i]);
              orderfound = true;
            }
          }
        }
      } break;
      default:
        break;
    }

    // Search for sequenced but not confirmed orders
    for (size_t i = 0; i < all_requests_.size(); i++) {
      if (all_requests_[i].orq_request_type_ == ORQ_SEND)  // Searching for Sequenced but Not-Confirmed order
      {
        if ((all_requests_[i].server_assigned_client_id_ == sim_genrequest.server_assigned_client_id_) &&
            (all_requests_[i].sreq_.ssor_.p_new_order_->client_assigned_order_sequence() ==
             caos)) {  // found order since it matches both the SACI and CAOS
          BroadcastSequenced(sim_genrequest.server_assigned_client_id_, all_requests_[i].sreq_.ssor_.p_new_order_);
          orderfound = true;
        }
      }
    }

    if (!orderfound) {  // not found in sequenced or unsequenced orders
      BroadcastOrderNone(sim_genrequest.server_assigned_client_id_, caos, saos, smv_.secname(),
                         smv_.GetDoublePx(int_price), int_price, buysell, 0, 0);
    }
  }

  return to_postpone;
}

void BaseSimMarketMaker::ProcessRequestQueue() {
  if (all_requests_busy_) {
    return;
  }

  all_requests_busy_ = true;

  // Reset before starting the processing of requeusts
  VectorUtils::FillInValue(saci_to_executed_size_, 0);
  for (auto sim_request_iter_ = all_requests_.begin();
       sim_request_iter_ != all_requests_.end() && sim_request_iter_->wakeup_time_ <= watch_.tv();) {
    SimGenRequest& genrequest = *sim_request_iter_;
    int to_postpone_ = 0;
    bool to_ignore_now_ = false;

    switch (genrequest.orq_request_type_) {
      case ORQ_SEND: {
        to_postpone_ = ProcessSendRequest(genrequest);
        break;
      }

      case ORQ_CANCEL: {
        to_postpone_ = ProcessCancelRequest(genrequest);
        break;
      }

      case ORQ_CXLREPLACE: {
        to_postpone_ = ProcessCxlReplaceRequest(genrequest);
        break;
      }

      case ORQ_REPLAY: {
        to_postpone_ = ProcessReplayRequest(genrequest);
        break;
      }

      default:
        break;
    }

    if (to_postpone_ == -1) {
      sim_request_iter_++;
    } else if (to_postpone_ != 0) {
      int POSTPONE_MSECS = 100;
      if (genrequest.orq_request_type_ == ORQ_SEND) {
        POSTPONE_MSECS = 5;
      }
      genrequest.wakeup_time_ = genrequest.wakeup_time_ + ttime_t(0, POSTPONE_MSECS * 1000);
      genrequest.postponed_once_ = true;
      std::stable_sort(all_requests_.begin(), all_requests_.end());
      sim_request_iter_ = all_requests_.begin();  ///< request postponed and deque sorted hence iterators invalidated
    } else {
      if (!to_ignore_now_) {
        sim_request_iter_ = all_requests_.erase(sim_request_iter_);  ///< also increments iterator
      } else {
        sim_request_iter_++;  // just increment iterator since we are ignoring this request for now
      }
    }
  }

  if (!pending_requests_.empty()) {
    for (auto i = 0u; i < pending_requests_.size(); i++) {
      all_requests_.push_back(pending_requests_[i]);
    }
    std::stable_sort(all_requests_.begin(), all_requests_.end());
    pending_requests_.clear();
  }

  VectorUtils::FillInValue(saci_to_executed_size_, 0);
  all_requests_busy_ = false;
}

void BaseSimMarketMaker::BackupQueueSizes(BaseSimOrder* sim_order) {
  // store the queue sizes in map for each saos
  if (saos_queue_size_map_.find(sim_order->server_assigned_order_sequence()) == saos_queue_size_map_.end()) {
    saos_queue_size_map_[sim_order->server_assigned_order_sequence()].push_back(sim_order->queue_size_ahead());
    saos_queue_size_map_[sim_order->server_assigned_order_sequence()].push_back(sim_order->queue_size_behind());
  } else {
    saos_queue_size_map_[sim_order->server_assigned_order_sequence()][0] = sim_order->queue_size_ahead();
    saos_queue_size_map_[sim_order->server_assigned_order_sequence()][1] = sim_order->queue_size_behind();
  }
}

int BaseSimMarketMaker::RestoreQueueSizes(BaseSimOrder* sim_order, const int posttrade_asksize_at_trade_price,
                                          const int trd_size) {
  if (saos_queue_size_map_.find(sim_order->server_assigned_order_sequence()) != saos_queue_size_map_.end()) {
    if (saos_queue_size_map_[sim_order->server_assigned_order_sequence()][0] +
            saos_queue_size_map_[sim_order->server_assigned_order_sequence()][1] ==
        posttrade_asksize_at_trade_price + trd_size) {
      // if there were no cancellations, size decrease only due to trade
      sim_order->queue_size_ahead_ = saos_queue_size_map_[sim_order->server_assigned_order_sequence()][0];
      sim_order->queue_size_behind_ = saos_queue_size_map_[sim_order->server_assigned_order_sequence()][1];
    } else if (saos_queue_size_map_[sim_order->server_assigned_order_sequence()][0] +
                   saos_queue_size_map_[sim_order->server_assigned_order_sequence()][1] <
               posttrade_asksize_at_trade_price + trd_size) {
      // if the size has increased, assuming this addition (no cancellation + addition) is from behind
      sim_order->queue_size_ahead_ = saos_queue_size_map_[sim_order->server_assigned_order_sequence()][0];
      sim_order->queue_size_behind_ = posttrade_asksize_at_trade_price - sim_order->queue_size_ahead_;
    } else {
      // if the size has decreased more than trade size, estimate the queue size decrease from front  due to
      // cancellations

      if (sim_order->queue_size_ahead_ + sim_order->queue_size_behind_ > 0) {
        sim_order->queue_size_ahead_ = saos_queue_size_map_[sim_order->server_assigned_order_sequence()][0] -
                                       int(((saos_queue_size_map_[sim_order->server_assigned_order_sequence()][0] +
                                             saos_queue_size_map_[sim_order->server_assigned_order_sequence()][1] -
                                             posttrade_asksize_at_trade_price - trd_size) *
                                            sim_order->queue_size_ahead_) /
                                           (sim_order->queue_size_ahead_ + sim_order->queue_size_behind_));
      } else {
        sim_order->queue_size_ahead_ = posttrade_asksize_at_trade_price;
      }

      if (sim_order->queue_size_ahead_ < 0) {
        sim_order->queue_size_ahead_ = 0;
      }
    }
  }
  return 0;
}

// Get the log string for the first sim order at this int_price
std::string BaseSimMarketMaker::GetBidSimOrderStr(int int_price) {
  BidPriceSimOrderMapIter_t i2bov_iter = intpx_bid_order_map_.find(int_price);

  if (i2bov_iter == intpx_bid_order_map_.end() || i2bov_iter->second.empty()) {
    return "";
  }

  std::ostringstream oss;
  oss << "[ ";

  BaseSimOrderPtrVec::iterator order_vec_iter = i2bov_iter->second.begin();

  if (order_vec_iter != i2bov_iter->second.end()) {
    BaseSimOrder* sim_order = *order_vec_iter;

    oss << sim_order->queue_orders_ahead_ << " " << sim_order->queue_size_ahead_ << " * " << sim_order->size_remaining()
        << " * " << sim_order->queue_size_behind_ << " " << sim_order->queue_orders_behind_;
  }
  oss << " ] ";

  return oss.str();
}

// Get the log string for the first sim order at this int_price
std::string BaseSimMarketMaker::GetAskSimOrderStr(int int_price) {
  AskPriceSimOrderMapIter_t i2aov_iter = intpx_ask_order_map_.find(int_price);

  if (i2aov_iter == intpx_ask_order_map_.end() || i2aov_iter->second.empty()) {
    return "";
  }

  std::ostringstream oss;
  oss << "[ ";

  BaseSimOrderPtrVec::iterator order_vec_iter = i2aov_iter->second.begin();

  if (order_vec_iter != i2aov_iter->second.end()) {
    BaseSimOrder* sim_order = *order_vec_iter;

    oss << sim_order->queue_orders_ahead_ << " " << sim_order->queue_size_ahead_ << " * " << sim_order->size_remaining()
        << " * " << sim_order->queue_size_behind_ << " " << sim_order->queue_orders_behind_;
  }
  oss << " ] ";

  return oss.str();
}

// Log sim order exec event
void BaseSimMarketMaker::LogExec(BaseSimOrder* sim_order, int size_executed) {
  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    int caos = sim_order->client_assigned_order_sequence();
    int saos = sim_order->server_assigned_order_sequence();
    int saci = sim_order->server_assigned_client_id();

    DBGLOG_TIME_CLASS_FUNC << "ExecutedOrder SACI: " << saci << ' ' << sim_order->security_name() << ' '
                           << GetTradeTypeChar(sim_order->buysell()) << ' ' << sim_order->price() << ' '
                           << sim_order->size_remaining() << " sE:" << sim_order->size_executed() << ' '
                           << ToString(sim_order->order_status()) << " [" << sim_order->queue_size_ahead() << "-"
                           << sim_order->queue_size_behind() << '-' << sim_order->num_events_seen() << ']'
                           << " CAOS: " << caos << " SAOS: " << saos << " size_executed: " << size_executed
                           << DBGLOG_ENDL_FLUSH;
  }
}

void BaseSimMarketMaker::UpdateAggSizes() {
  // Update aggressive fill values

  // Iterate over all SACI
  for (size_t i = 0; i < intpx_agg_bid_size_map_.size(); i++) {
    // Iterate over all bid pairs
    for (auto pair_itr = intpx_agg_bid_size_map_[i].begin(); pair_itr != intpx_agg_bid_size_map_[i].end();) {
      int int_price = pair_itr->first;

      // If this price is no longer aggressive, delete this pair
      if (int_price < smv_.bestask_int_price()) {
        intpx_agg_bid_size_map_[i].erase(pair_itr++);
        continue;
      }

      int size = GetSizeAtIntPrice(kTradeTypeSell, int_price);

      // If the new size is less, decrease the exec size as well
      if (size < pair_itr->second.first) {
        pair_itr->second.second = pair_itr->second.second * (size / pair_itr->second.first);
      }

      // If the exec size is <= 0, delete this map entry
      if (pair_itr->second.second <= 0) {
        intpx_agg_bid_size_map_[i].erase(pair_itr++);
        continue;
      }

      // Update the market size
      pair_itr->second.first = size;
      pair_itr++;
    }
  }

  for (size_t i = 0; i < intpx_agg_ask_size_map_.size(); i++) {
    // Iterate over all ask pairs
    for (auto pair_itr = intpx_agg_ask_size_map_[i].begin(); pair_itr != intpx_agg_ask_size_map_[i].end();) {
      int int_price = pair_itr->first;

      // If this price is no longer aggressive, delete this pair
      if (int_price > smv_.bestbid_int_price()) {
        intpx_agg_ask_size_map_[i].erase(pair_itr++);
        continue;
      }

      int size = GetSizeAtIntPrice(kTradeTypeBuy, int_price);

      // If the new size is less, decrease the exec size as well
      if (size < pair_itr->second.first) {
        pair_itr->second.second = pair_itr->second.second * (size / pair_itr->second.first);
      }

      // If the exec size is <= 0, delete this map entry
      if (pair_itr->second.second <= 0) {
        intpx_agg_ask_size_map_[i].erase(pair_itr++);
        continue;
      }

      // Update the market size
      pair_itr->second.first = size;
      pair_itr++;
    }
  }
}

int BaseSimMarketMaker::Connect() {
  const int saci = (int)client_position_map_.size();
  client_position_map_.push_back(0);  // hence client_position_map_ [ this_server_assigned_client_id_ ] = 0 ;
  global_position_to_send_map_.push_back(global_position_);  // hence global_position_to_send_map_ [
                                                             // this_server_assigned_client_id_ ] = global_position_ ;
  saci_to_executed_size_.push_back(0);
  real_saci_map_.push_back(0);

  if ((int)intpx_agg_bid_size_map_.size() <= saci) {
    intpx_agg_bid_size_map_.resize(saci + 1);
  }

  if ((int)intpx_agg_ask_size_map_.size() <= saci) {
    intpx_agg_ask_size_map_.resize(saci + 1);
  }

  return saci;
}

int BaseSimMarketMaker::MatchBidORSExec(BaseSimOrder* sim_order, int size_executed, int size_remaining) {
  int ord_size_executed = 0;
  if (current_matching_algo_ == kSimpleProRata) {
    // Get the fraction of ors order size executed and execute the same size

    // The denominator should be original_size_executed, numerator should be calculated from masked_size
    int size_to_be_executed =
        double(size_executed * sim_order->size_remaining_) / double(size_remaining + size_executed);

    // Match the size to be executed
    ord_size_executed = sim_order->MatchPartial(size_to_be_executed);

  } else if (current_matching_algo_ == kFIFO) {
    // this order was older
    ord_size_executed = 0;
    if (size_executed < sim_order->size_remaining()) {
      // Partial exec followed by queue size adjustment to bring this order to the front
      ord_size_executed = sim_order->MatchPartial(size_executed);
      sim_order->queue_size_ahead_ = 0;
      sim_order->queue_size_behind_ = smv_.bestbid_size();
      sim_order->ors_exec_size_ += ord_size_executed;
    } else {
      ord_size_executed = sim_order->ExecuteRemaining();
    }

  } else if (current_matching_algo_ == kSplitFIFOProRata) {
    // Get the fifo executed size ( approximation)
    int fifo_executed_size = fifo_matching_fraction_ * size_executed;

    // Remaining executed size is pro-rata
    // The denominator should be original_size_executed, numerator should be calculated from masked_size
    int pro_rata_executed_size = ((size_executed - fifo_executed_size) * sim_order->size_remaining()) /
                                 (size_executed - fifo_executed_size + size_remaining);

    // Total executed size
    int total_executed_size = fifo_executed_size + pro_rata_executed_size;

    ord_size_executed = 0;
    if (total_executed_size > 0) {
      // If total is > 0

      if (total_executed_size < sim_order->size_remaining()) {
        // Total executed size is less than order size, order is still in market
        ord_size_executed = sim_order->MatchPartial(total_executed_size);

        // Set Book-keeping variables
        // Not setting the qa-qb as we assume have already adjusted the qa-qb based on  original size change
        sim_order->ors_exec_size_ += ord_size_executed;
      } else {
        // All of the order got executed
        ord_size_executed = sim_order->ExecuteRemaining();
      }
    }
  } else {
    std::cerr << " BidORS exec not handled for matching algo " << current_matching_algo_ << std::endl;
  }

  return ord_size_executed;
}

void BaseSimMarketMaker::BidORSExec(int real_saci, int real_caos, int real_saos, int security_id, double price,
                                    int size_remaining, int size_executed, int client_position, int global_position,
                                    int int_price) {
  // Get the sequenced time and original int price for this exec
  ttime_t seqd_time(0, 0);
  int bid_int_price = 0;

  auto bid_orders_iter = ors_bid_orders_.begin();
  while (bid_orders_iter != ors_bid_orders_.end()) {
    if (real_saos == bid_orders_iter->server_assigned_order_sequence_) {
      bid_int_price = bid_orders_iter->int_price_;
      seqd_time = bid_orders_iter->sequenced_time_;

      if (size_remaining == 0) {
        // Remove from the vector
        ors_bid_orders_.erase(bid_orders_iter);
      }
      break;
    }
    bid_orders_iter++;
  }

  // Return from here if we did not find the order
  if (seqd_time == ttime_t(0, 0)) {
    return;
  }

  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "ExecutedRealOrder SAOS: " << real_saos << " " << int_price << " sequenced at "
                           << seqd_time << DBGLOG_ENDL_FLUSH;
  }

  // Note that the real size executed is not equal to size_executed
  // since size_executed contains the total size executed so far
  int size_exec = size_executed;
  if (saos_to_size_exec_real_.find(real_saos) != saos_to_size_exec_real_.end()) {
    size_exec = size_executed - saos_to_size_exec_real_[real_saos];
  }
  saos_to_size_exec_real_[real_saos] = size_executed;

  // Iterate over the current sim orders and see if we can give fill to anyone
  for (auto& intpx_order_vec_pair : intpx_bid_order_map_) {
    // Break if the int price is below the int price of exec
    if (intpx_order_vec_pair.first < bid_int_price) {
      break;
    }

    // Improve/Aggress sim order
    if (intpx_order_vec_pair.first > bid_int_price) {
      if (intpx_order_vec_pair.second.empty()) {
        continue;
      }

      for (auto& sim_order : intpx_order_vec_pair.second) {
        if (sim_order == NULL) {
          continue;
        }

        int ord_size_executed = sim_order->ExecuteRemaining();
        int saci = sim_order->server_assigned_client_id();
        client_position_map_[saci] += ord_size_executed;
        global_position_to_send_map_[saci] += ord_size_executed;
        LogExec(sim_order, ord_size_executed);

        BroadcastExecNotification(saci, sim_order);

        basesimorder_mempool_.DeAlloc(sim_order);
        sim_order = NULL;
      }

      intpx_order_vec_pair.second.clear();
    } else if (intpx_order_vec_pair.first == bid_int_price) {
      for (auto sim_order_iter = intpx_order_vec_pair.second.begin();
           sim_order_iter != intpx_order_vec_pair.second.end();) {
        // check which orders are executed, send message and
        // deallocate order, nullify the pointer, erase from vector.
        // Note the iterator does not need to be incremented since we either break out of loop or erase the iterator
        // and
        // hence increment it.

        BaseSimOrder* const sim_order = *sim_order_iter;
        if (sim_order == nullptr) {
          sim_order_iter++;
          continue;
        }

        int saci = sim_order->server_assigned_client_id();
        int ord_size_executed = 0;

        // Computing this as we would not want to give more fills to sim orders than real
        // would work fine for both FIFO and pro-rata, in pro-rata it would come to this only for residual fills
        int new_size_exec = size_exec - saci_to_executed_size_[saci];
        if (new_size_exec <= 0) {
          sim_order_iter++;
          continue;
        }

        if (sim_order->order_sequenced_time_ <= seqd_time &&
            (real_saci_map_[saci] <= 0 || real_saci_map_[saci] == real_saci)) {
          ord_size_executed = MatchBidORSExec(sim_order, size_exec, size_remaining);

          if (ord_size_executed > 0) {
            // If there was an execution for this order, then log it and broadcast this
            client_position_map_[saci] += ord_size_executed;
            global_position_to_send_map_[saci] += ord_size_executed;
            saci_to_executed_size_[saci] += ord_size_executed;

            LogExec(sim_order, ord_size_executed);

            BroadcastExecNotification(saci, sim_order);

            // dealloc order and erase from vector
            if (sim_order->size_remaining() == 0) {
              basesimorder_mempool_.DeAlloc(sim_order);
              sim_order_iter = intpx_order_vec_pair.second.erase(sim_order_iter);
            } else {
              sim_order_iter++;
            }
          } else {
            sim_order_iter++;
          }

        } else {
          // // if this order was not executed, it is likely that others after this would not be executed as well.
          // // only way this breaks if size was modified of an order and hence it was put back in queue.
          // // we just need to make sure that if the vector has more than 1 order, when an order is modified up in
          // size
          // // we actually remove the order at put it back in the vector as well.
          // break;
          sim_order_iter++;
        }
      }
    }
  }

  if (config_.use_ors_exec_before_conf_ && !all_requests_busy_) {
    all_requests_busy_ = true;

    for (unsigned iter = 0; iter < all_requests_.size();) {
      SimGenRequest& genrequest = all_requests_[iter];
      switch (genrequest.orq_request_type_) {
        case ORQ_SEND: {
          BaseSimOrder* sim_order = genrequest.sreq_.ssor_.p_new_order_;

          int saci = sim_order->server_assigned_client_id();
          // shoudln't we check _buysell_ of p_sim_order_ here
          if (sim_order->int_price_ == bid_int_price &&
              (real_saci_map_[saci] <= 0 || real_saci_map_[saci] == real_saci)) {
            if (sim_order->order_sequenced_time_ <= seqd_time) {
              // this order was older
              int ord_size_executed = 0;

              sim_order->Confirm();
              BroadcastConfirm(genrequest.server_assigned_client_id_, sim_order);

              if (size_exec < sim_order->size_remaining()) {
                // Partial exec followed by queue size adjustment to bring this order to the front
                ord_size_executed = sim_order->MatchPartial(size_exec);
                sim_order->queue_size_ahead_ = 0;
                sim_order->queue_size_behind_ = smv_.bestbid_size();
                sim_order->ors_exec_size_ += ord_size_executed;
              } else {
                ord_size_executed = sim_order->ExecuteRemaining();
              }
              client_position_map_[saci] += ord_size_executed;
              global_position_to_send_map_[saci] += ord_size_executed;
              LogExec(sim_order, ord_size_executed);

              BroadcastExecNotification(sim_order->server_assigned_client_id(), sim_order);

              if (sim_order->size_remaining() <= 0) {
                basesimorder_mempool_.DeAlloc(sim_order);
                all_requests_.erase(all_requests_.begin() + iter);
              } else {
                iter++;
              }
            } else {
              iter++;
            }
          } else {
            iter++;
          }
          break;
        }
        default: {
          iter++;
          break;
        }
      }
    }
    all_requests_busy_ = false;
  }

  VectorUtils::FillInValue(saci_to_executed_size_, 0);
}

// This simply matches our top bid orders, and does not affect queue positions (just to avoid false simreal issues)
void BaseSimMarketMaker::BidORSIntExec(int real_saci, int real_caos, int real_saos, int security_id, double price,
                                       int size_remaining, int size_executed, int client_position, int global_position,
                                       int int_price) {
  // Get the sequenced time and original int price for this exec
  ttime_t seqd_time(0, 0);
  int bid_int_price = 0;

  auto bid_orders_iter = ors_bid_orders_.begin();
  while (bid_orders_iter != ors_bid_orders_.end()) {
    if (real_saos == bid_orders_iter->server_assigned_order_sequence_) {
      bid_int_price = bid_orders_iter->int_price_;
      seqd_time = bid_orders_iter->sequenced_time_;
      break;
    }
    bid_orders_iter++;
  }

  // Return from here if we did not find the order
  if (seqd_time == ttime_t(0, 0)) {
    return;
  }

  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "ExecutedIntMatchRealOrder SAOS: " << real_saos << " " << int_price << " sequenced at "
                           << seqd_time << DBGLOG_ENDL_FLUSH;
  }

  // Iterate over the current sim orders and see if we can give fill to anyone
  for (auto& intpx_order_vec_pair : intpx_bid_order_map_) {
    // Break if the int price is below the int price of exec
    if (intpx_order_vec_pair.first < bid_int_price || size_executed <= 0) {
      break;
    }

    for (auto sim_order_iter = intpx_order_vec_pair.second.begin();
         sim_order_iter != intpx_order_vec_pair.second.end() && size_executed > 0;) {
      BaseSimOrder* sim_order = *sim_order_iter;
      // If this order is valid and has to be filled
      if (sim_order == nullptr || real_saci_map_[sim_order->server_assigned_client_id()] != real_saci) {
        sim_order_iter++;
        continue;
      }

      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "ExecutedBidIntMatchRealOrder SAOS: " << real_saos << " " << int_price
                               << " sequenced at " << seqd_time << ", Real_SACI: " << real_saci
                               << ", Size_Executed: " << size_executed << DBGLOG_ENDL_FLUSH;
      }

      int ord_size_executed = 0;
      // Computing this as we would not want to give more fills to sim orders than real
      // would work fine for both FIFO and pro-rata, in pro-rata it would come to this only for residual fills
      int new_size_exec = size_executed - saci_to_executed_size_[sim_order->server_assigned_client_id()];
      if (new_size_exec <= 0) {
        sim_order_iter++;
        continue;
      }

      // Match the appropriate size based on on matching logic
      ord_size_executed = MatchBidORSExec(sim_order, size_executed, size_remaining);

      if (ord_size_executed > 0) {
        int saci = sim_order->server_assigned_client_id();
        double old_price = sim_order->price_;
        sim_order->price_ = price;  // Use internal match price for logging/pnl cumputation
        client_position_map_[saci] += ord_size_executed;
        global_position_to_send_map_[saci] += ord_size_executed;
        saci_to_executed_size_[saci] += ord_size_executed;

        LogExec(sim_order, ord_size_executed);

        BroadcastExecNotification(saci, sim_order);

        // dealloc order and erase from vector
        if (sim_order->size_remaining() == 0) {
          basesimorder_mempool_.DeAlloc(sim_order);
          sim_order_iter = intpx_order_vec_pair.second.erase(sim_order_iter);
        } else {
          sim_order->price_ = old_price;
          sim_order_iter++;
        }

        size_executed -= ord_size_executed;
      } else {
        sim_order_iter++;
      }
    }
  }
  VectorUtils::FillInValue(saci_to_executed_size_, 0);
}

/**
 *
 * @param sim_order
 * @param size_executed
 * @param size_remaining
 * @return
 */
int BaseSimMarketMaker::MatchAskORSExec(BaseSimOrder* sim_order, int size_executed, int size_remaining) {
  int ord_size_executed = 0;
  if (current_matching_algo_ == kSimpleProRata) {
    int size_to_be_executed =
        double(size_executed * sim_order->size_remaining_) / double(size_remaining + size_executed);
    ord_size_executed = sim_order->MatchPartial(size_to_be_executed);

  } else if (current_matching_algo_ == kFIFO) {
    ord_size_executed = 0;
    if (size_executed < sim_order->size_remaining()) {
      // Partial exec followed by queue size adjustment to bring this order to the front
      ord_size_executed = sim_order->MatchPartial(size_executed);
      sim_order->queue_size_ahead_ = 0;
      sim_order->queue_size_behind_ = smv_.bestask_size();
      sim_order->ors_exec_size_ += ord_size_executed;
    } else {
      ord_size_executed = sim_order->ExecuteRemaining();
    }
  } else if (current_matching_algo_ == kSplitFIFOProRata) {
    // Get the fifo executed size ( approximation)
    int fifo_executed_size = fifo_matching_fraction_ * size_executed;

    // Remaining executed size is pro-rata
    int pro_rata_executed_size = ((size_executed - fifo_executed_size) * sim_order->size_remaining()) /
                                 (size_executed - fifo_executed_size + size_remaining);

    // Total executed size
    int total_executed_size = fifo_executed_size + pro_rata_executed_size;

    ord_size_executed = 0;
    if (total_executed_size > 0) {
      // If total is > 0

      if (total_executed_size < sim_order->size_remaining()) {
        // Total executed size is less than order size, order is still in market
        ord_size_executed = sim_order->MatchPartial(total_executed_size);

        // Set Book-keeping variables
        // Not setting the qa-qb as we assume have already adjusted the qa-qb based on  original size change

        sim_order->ors_exec_size_ += ord_size_executed;
      } else {
        // All of the order got executed
        ord_size_executed = sim_order->ExecuteRemaining();
      }
    }
  } else {
    std::cerr << "askORS Exec was not implemented for matching algo : " << current_matching_algo_ << std::endl;
  }
  return ord_size_executed;
}

// This simply matches our top bid orders, and does not affect queue positions (just to avoid false simreal issues)
void BaseSimMarketMaker::AskORSIntExec(int real_saci, int real_caos, int real_saos, int security_id, double price,
                                       int size_remaining, int size_executed, int client_position, int global_position,
                                       int int_price) {
  // Get the sequenced time and original int price for this exec
  ttime_t seqd_time(0, 0);
  int ask_int_price = 0;

  auto ask_orders_iter = ors_ask_orders_.begin();
  while (ask_orders_iter != ors_ask_orders_.end()) {
    if (real_saos == ask_orders_iter->server_assigned_order_sequence_) {
      ask_int_price = ask_orders_iter->int_price_;
      seqd_time = ask_orders_iter->sequenced_time_;
      break;
    }
    ask_orders_iter++;
  }

  // Return from here if we did not find the order
  if (seqd_time == ttime_t(0, 0)) {
    return;
  }

  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "ExecutedIntMatchRealOrder SAOS: " << real_saos << " " << int_price << " sequenced at "
                           << seqd_time << DBGLOG_ENDL_FLUSH;
  }

  // Iterate over the current sim orders and see if we can give fill to anyone
  for (auto& intpx_order_vec_pair : intpx_ask_order_map_) {
    // Break if the int price is below the int price of exec
    if (intpx_order_vec_pair.first > ask_int_price || size_executed <= 0) {
      break;
    }

    for (auto sim_order_iter = intpx_order_vec_pair.second.begin();
         sim_order_iter != intpx_order_vec_pair.second.end() && size_executed > 0;) {
      BaseSimOrder* sim_order = *sim_order_iter;

      // If this order is valid and has to be filled
      if (sim_order == nullptr || real_saci_map_[sim_order->server_assigned_client_id()] != real_saci) {
        sim_order_iter++;
        continue;
      }

      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "ExecutedAskIntMatchRealOrder SAOS: " << real_saos << " " << int_price
                               << " sequenced at " << seqd_time << ", Real_SACI: " << real_saci
                               << ", Size_Executed: " << size_executed << DBGLOG_ENDL_FLUSH;
      }

      int ord_size_executed = 0;
      // Computing this as we would not want to give more fills to sim orders than real
      // would work fine for both FIFO and pro-rata, in pro-rata it would come to this only for residual fills
      int new_size_exec = size_executed - saci_to_executed_size_[sim_order->server_assigned_client_id()];
      if (new_size_exec <= 0) {
        sim_order_iter++;
        continue;
      }

      // match the appropriate size based on appropriate matching logic
      ord_size_executed = MatchAskORSExec(sim_order, size_executed, size_remaining);

      if (ord_size_executed > 0) {
        int saci = sim_order->server_assigned_client_id();
        double old_price = sim_order->price_;
        sim_order->price_ = price;  // Use internal match price for logging/pnl cumputation
        client_position_map_[saci] -= ord_size_executed;
        global_position_to_send_map_[saci] -= ord_size_executed;
        saci_to_executed_size_[saci] += new_size_exec;

        LogExec(sim_order, ord_size_executed);

        BroadcastExecNotification(saci, sim_order);

        // dealloc order and erase from vector
        if (sim_order->size_remaining() == 0) {
          basesimorder_mempool_.DeAlloc(sim_order);
          sim_order_iter = intpx_order_vec_pair.second.erase(sim_order_iter);
        } else {
          sim_order->price_ = old_price;
          sim_order_iter++;
        }

        size_executed -= ord_size_executed;
      } else {
        sim_order_iter++;
      }
    }
  }
  VectorUtils::FillInValue(saci_to_executed_size_, 0);
}

void BaseSimMarketMaker::AskORSExec(int real_saci, int real_caos, int real_saos, int security_id, double price,
                                    int size_remaining, int size_executed, int client_position, int global_position,
                                    int int_price) {
  ttime_t seqd_time(0, 0);
  int ask_int_price = 0;

  auto ask_orders_iter = ors_ask_orders_.begin();

  while (ask_orders_iter != ors_ask_orders_.end()) {
    if (real_saos == ask_orders_iter->server_assigned_order_sequence_) {
      ask_int_price = ask_orders_iter->int_price_;
      seqd_time = ask_orders_iter->sequenced_time_;

      if (size_remaining == 0) {
        // Remove from the vector
        ors_ask_orders_.erase(ask_orders_iter);
      }
      break;
    }
    ask_orders_iter++;
  }

  if (seqd_time == ttime_t(0, 0)) {
    return;
  }

  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "ExecutedRealOrder SAOS: " << real_saos << " " << int_price << " sequenced at "
                           << seqd_time << DBGLOG_ENDL_FLUSH;
  }

  int size_exec = size_executed;
  if (saos_to_size_exec_real_.find(real_saos) != saos_to_size_exec_real_.end()) {
    size_exec = size_executed - saos_to_size_exec_real_[real_saos];
  }
  saos_to_size_exec_real_[real_saos] = size_executed;

  for (auto& intpx_order_vec_pair : intpx_ask_order_map_) {
    if (intpx_order_vec_pair.first > ask_int_price) {
      break;
    }

    if (intpx_order_vec_pair.first < ask_int_price) {
      if (intpx_order_vec_pair.second.empty()) {
        continue;
      }

      for (auto& sim_order : intpx_order_vec_pair.second) {
        if (sim_order == NULL) {
          continue;
        }

        int saci = sim_order->server_assigned_client_id();
        int ord_size_executued = sim_order->ExecuteRemaining();

        client_position_map_[saci] -= ord_size_executued;
        global_position_to_send_map_[saci] -= ord_size_executued;

        LogExec(sim_order, ord_size_executued);

        BroadcastExecNotification(saci, sim_order);

        basesimorder_mempool_.DeAlloc(sim_order);
        sim_order = NULL;
      }

      intpx_order_vec_pair.second.clear();
    } else if (intpx_order_vec_pair.first == ask_int_price) {
      // if price is same as executed order check the sequenced_time_
      for (auto sim_order_iter = intpx_order_vec_pair.second.begin();
           sim_order_iter != intpx_order_vec_pair.second.end();) {
        // check which orders are executed, send message and
        // deallocate order, nullify the pointer, erase from vector.
        // Note the iterator does not need to be incremented since we either break out of loop or erase the iterator
        // and
        // hence increment it.

        BaseSimOrder* const sim_order = *sim_order_iter;
        if (sim_order == nullptr) {
          sim_order_iter++;
          continue;
        }

        int saci = sim_order->server_assigned_client_id();
        int ord_size_executed = 0;
        // Computing this as we would not want to give more fills to sim orders than real
        // would work fine for both FIFO and pro-rata, in pro-rata it would come to this only for residual fills
        int new_size_exec = size_executed - saci_to_executed_size_[saci];
        if (new_size_exec <= 0) {
          sim_order_iter++;
          continue;
        }

        if (sim_order->order_sequenced_time_ <= seqd_time &&
            (real_saci_map_[saci] <= 0 || real_saci_map_[saci] == real_saci)) {
          // this order was older
          ord_size_executed = MatchAskORSExec(sim_order, size_exec, size_remaining);

          if (ord_size_executed > 0) {
            // non zero size was executed, adjust the maps
            // log the exec if required and broadcast to listeners
            client_position_map_[saci] -= ord_size_executed;
            global_position_to_send_map_[saci] -= ord_size_executed;
            saci_to_executed_size_[saci] += ord_size_executed;

            LogExec(sim_order, ord_size_executed);

            BroadcastExecNotification(sim_order->server_assigned_client_id(), sim_order);

            // dealloc order and erase from vector
            if (sim_order->size_remaining() <= 0) {
              basesimorder_mempool_.DeAlloc(sim_order);
              sim_order_iter = intpx_order_vec_pair.second.erase(sim_order_iter);
            } else
              sim_order_iter++;
          } else {
            sim_order_iter++;
          }
        } else {
          // // if this order was not executed, it is likely that others after this would not be executed as well.
          // // only way this breaks if size was modified of an order and hence it was put back in queue.
          // // we just need to make sure that if the vector has more than 1 order, when an order is modified up in
          // size
          // // we actually remove the order at put it back in the vector as well.
          // break;
          sim_order_iter++;
        }
      }
    }
  }

  if (config_.use_ors_exec_before_conf_ && !all_requests_busy_) {
    all_requests_busy_ = true;
    for (unsigned iter_ = 0; iter_ < all_requests_.size();) {
      SimGenRequest& genrequest = all_requests_[iter_];

      switch (genrequest.orq_request_type_) {
        case ORQ_SEND: {
          BaseSimOrder* sim_order = genrequest.sreq_.ssor_.p_new_order_;

          int saci = sim_order->server_assigned_client_id();
          if (sim_order->int_price_ == ask_int_price &&
              (real_saci_map_[saci] <= 0 || real_saci_map_[saci] == real_saci)) {
            if (sim_order->order_sequenced_time_ <= seqd_time) {
              // this order was older
              int this_size_executed_ = 0;

              sim_order->Confirm();
              BroadcastConfirm(genrequest.server_assigned_client_id_, sim_order);

              if (size_exec < sim_order->size_remaining()) {
                // Partial exec followed by queue size adjustment to bring this order to the front
                this_size_executed_ = sim_order->MatchPartial(size_exec);
                sim_order->queue_size_ahead_ = 0;
                sim_order->queue_size_behind_ = smv_.bestbid_size();
                sim_order->ors_exec_size_ += this_size_executed_;
              } else {
                this_size_executed_ = sim_order->ExecuteRemaining();
              }
              client_position_map_[sim_order->server_assigned_client_id()] -= this_size_executed_;
              global_position_to_send_map_[sim_order->server_assigned_client_id()] -= this_size_executed_;
              LogExec(sim_order, this_size_executed_);

              // BroadcastExecNotification (p_sim_order_->server_assigned_client_id(), sim_order);

              if (sim_order->size_remaining() <= 0) {
                basesimorder_mempool_.DeAlloc(sim_order);
                all_requests_.erase(all_requests_.begin() + iter_);
              } else {
                iter_++;
              }
            } else {
              iter_++;
            }
          } else {
            iter_++;
          }
          break;
        }
        default: {
          iter_++;
          break;
        }
      }
    }
    all_requests_busy_ = false;
  }

  VectorUtils::FillInValue(saci_to_executed_size_, 0);
}

// Called in case of internal executions
// Please note that this is not an actual market event, this is needed only for adding handling in simreal
void BaseSimMarketMaker::OrderInternallyMatched(const int saci, const int caos, const int saos,
                                                const unsigned int security_id, const double price,
                                                const TradeType_t buysell, const int size_remaining,
                                                const int size_executed, const int client_position,
                                                const int global_position, const int int_price,
                                                const int32_t server_assigned_message_sequence,
                                                const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  switch (buysell) {
    case kTradeTypeBuy: {
      BidORSIntExec(saci, caos, saos, security_id, price, size_remaining, size_executed, client_position,
                    global_position, int_price);
      break;
    }

    case kTradeTypeSell: {
      AskORSIntExec(saci, caos, saos, security_id, price, size_remaining, size_executed, client_position,
                    global_position, int_price);
      break;
    }

    default: { break; }
  }
}

void BaseSimMarketMaker::OrderExecuted(const int saci, const int caos, const int saos, const unsigned int security_id,
                                       const double price, const TradeType_t buysell, const int size_remaining,
                                       const int size_executed, const int client_position, const int global_position,
                                       const int int_price, const int32_t server_assigned_message_sequence,
                                       const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  if (!config_.use_ors_exec_ || (current_matching_algo_ != kFIFO && current_matching_algo_ != kSimpleProRata &&
                                 current_matching_algo_ != kSplitFIFOProRata)) {
    // Not sure why splitFIFO was not there earlier
    return;
  }

  if (!all_requests_.empty()) {
    ProcessRequestQueue();
  }

  switch (buysell) {
    case kTradeTypeBuy: {
      BidORSExec(saci, caos, saos, security_id, price, size_remaining, size_executed, client_position, global_position,
                 int_price);
      break;
    }

    case kTradeTypeSell: {
      AskORSExec(saci, caos, saos, security_id, price, size_remaining, size_executed, client_position, global_position,
                 int_price);
      break;
    }

    default: { break; }
  }
}

void BaseSimMarketMaker::OrderSequencedAtTime(const int r_server_assigned_client_id_,
                                              const int _client_assigned_order_sequence_,
                                              const int _server_assigned_order_sequence_,
                                              const unsigned int _security_id_, const double _price_,
                                              const TradeType_t _buysell_, const int _size_remaining_,
                                              const int _size_executed_, const int _client_position_,
                                              const int _global_position_, const int _int_price_,
                                              const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  OrderInfoStruct this_order_summary_;
  this_order_summary_.buysell_ = _buysell_;
  this_order_summary_.int_price_ = _int_price_;
  this_order_summary_.server_assigned_order_sequence_ = _server_assigned_order_sequence_;
  this_order_summary_.sequenced_time_ = time_set_by_server;
  this_order_summary_.size_ = _size_remaining_;

  if (_buysell_ == HFSAT::kTradeTypeSell) {
    ors_ask_orders_.push_back(this_order_summary_);
  } else {
    ors_bid_orders_.push_back(this_order_summary_);
  }
}

void BaseSimMarketMaker::OrderConfirmedAtTime(
    const int r_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t _buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int _int_price_, const ttime_t _time_set_by_server_,
    const uint64_t t_exch_assigned_sequence_) {}

/**
 *  \brief called by ORSMessageLiveSource when the messagetype is kORRType_CxRe
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
void BaseSimMarketMaker::OrderConfCxlReplaced(
    const int r_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t _buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int _int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "CxlReplace: " << _server_assigned_order_sequence_ << ", " << _int_price_
                           << DBGLOG_ENDL_FLUSH;
  }

  // Update the new price and sequence times for orders
  if (_buysell_ == HFSAT::kTradeTypeBuy) {
    std::vector<OrderInfoStruct>::iterator ors_bid_orders_iter_ = ors_bid_orders_.begin();
    while (ors_bid_orders_iter_ != ors_bid_orders_.end()) {
      if (_server_assigned_order_sequence_ == ors_bid_orders_iter_->server_assigned_order_sequence_) {
        ors_bid_orders_iter_->int_price_ = _int_price_;
        // If the size has increased or price has changed, order position would have changed thus required to reset
        // everything
        if (ors_bid_orders_iter_->size_ > _size_remaining_ || ors_bid_orders_iter_->int_price_ != _int_price_) {
          ors_bid_orders_iter_->size_ = _size_remaining_;
          ors_bid_orders_iter_->int_price_ = _int_price_;
          ors_bid_orders_iter_->sequenced_time_ = time_set_by_server;
        }
        break;
      }
      ors_bid_orders_iter_++;
    }
  } else {
    std::vector<OrderInfoStruct>::iterator ors_ask_orders_iter_ = ors_ask_orders_.begin();
    while (ors_ask_orders_iter_ != ors_ask_orders_.end()) {
      if (_server_assigned_order_sequence_ == ors_ask_orders_iter_->server_assigned_order_sequence_) {
        ors_ask_orders_iter_->int_price_ = _int_price_;

        // If the size has increased or price has changed, order position would have changed thus required to reset
        // everything

        if (ors_ask_orders_iter_->size_ > _size_remaining_ || ors_ask_orders_iter_->int_price_ != _int_price_) {
          ors_ask_orders_iter_->size_ = _size_remaining_;
          ors_ask_orders_iter_->int_price_ = _int_price_;
          ors_ask_orders_iter_->sequenced_time_ = time_set_by_server;
        }
        break;
      }
      ors_ask_orders_iter_++;
    }
  }
}

void BaseSimMarketMaker::OrderCancelRejected(const int t_server_assigned_client_id_,
                                             const int _client_assigned_order_sequence_,
                                             const int _server_assigned_order_sequence_,
                                             const unsigned int _security_id_, const double _price_,
                                             const TradeType_t t_buysell_, const int _size_remaining_,
                                             const int _rejection_reason_, const int t_client_position_,
                                             const int t_global_position_, const int r_int_price_,
                                             const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  // Execute the orders which have cancel reject messages
  ////here assumption is that
  if (_rejection_reason_ == 1)  // unknow order, means the order has been executed
  {
    OrderExecuted(t_server_assigned_client_id_, _client_assigned_order_sequence_, _server_assigned_order_sequence_,
                  _security_id_, _price_, t_buysell_, _size_remaining_, 0, t_client_position_, t_global_position_,
                  r_int_price_, 0, exchange_order_id, time_set_by_server);
  }
}

void BaseSimMarketMaker::PrintDelayStats(ttime_t start_time, ttime_t end_time) {
  std::vector<ttime_t> t_ts_vec_;
  security_delay_stats_->GetTimeStampsInRange(start_time, end_time, t_ts_vec_);

  if (t_ts_vec_.size() == 0) {
    t_ts_vec_.push_back(start_time);
    t_ts_vec_.push_back(end_time);
  }

  std::vector<int> t_sod_vec_;
  std::vector<int> t_cod_vec_;
  std::vector<int> t_crod_vec_;

  ttime_t t_delay_;
  unsigned int t_size_ = t_ts_vec_.size();
  for (unsigned int i = 0; i < t_size_; i++) {
    t_delay_ = GetSendOrderDelay(t_ts_vec_[i]);
    t_sod_vec_.push_back(t_delay_.tv_usec);

    t_delay_ = GetCancelOrderDelay(t_ts_vec_[i]);
    t_cod_vec_.push_back(t_delay_.tv_usec);

    t_delay_ = GetCancelReplaceOrderDelay(t_ts_vec_[i]);
    t_crod_vec_.push_back(t_delay_.tv_usec);
  }

  std::sort(t_sod_vec_.begin(), t_sod_vec_.end());
  std::sort(t_cod_vec_.begin(), t_cod_vec_.end());
  std::sort(t_crod_vec_.begin(), t_crod_vec_.end());

  std::cerr << "[" << lower_bound_usecs_ << " " << upper_bound_usecs_ << "]\n";

  unsigned int idx = 0;
  std::cerr << "MIN: " << idx << " " << t_sod_vec_[idx] << " " << t_cod_vec_[idx] << " " << t_crod_vec_[idx] << "\n";

  idx = (double)(t_size_ - 1) * ((double)50 / 100);
  std::cerr << "MEDIAN: " << idx << " " << t_sod_vec_[idx] << " " << t_cod_vec_[idx] << " " << t_crod_vec_[idx] << "\n";

  idx = (double)(t_size_ - 1) * ((double)75 / 100);
  std::cerr << "P75: " << idx << " " << t_sod_vec_[idx] << " " << t_cod_vec_[idx] << " " << t_crod_vec_[idx] << "\n";

  idx = (double)(t_size_ - 1) * ((double)90 / 100);
  std::cerr << "P90: " << idx << " " << t_sod_vec_[idx] << " " << t_cod_vec_[idx] << " " << t_crod_vec_[idx] << "\n";

  idx = (double)(t_size_ - 1) * ((double)99 / 100);
  std::cerr << "P99: " << idx << " " << t_sod_vec_[idx] << " " << t_cod_vec_[idx] << " " << t_crod_vec_[idx] << "\n";

  idx = t_size_ - 1;
  std::cerr << "MAX: " << idx << " " << t_sod_vec_[idx] << " " << t_cod_vec_[idx] << " " << t_crod_vec_[idx] << "\n";
}

}  // HFSAT
