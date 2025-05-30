/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#include "dvctrade/ExecLogic/signal_based_trading.hpp"

namespace HFSAT {
/*  void SignalBasedTrading::CollectTradingInfo(DebugLogger& _dbglogger_, std::string _model_filename_,
                                            std::vector<std::string>& _source_shortcode_vec_,
                                            std::vector<std::string>& _ors_shortcode_vec_) {
  std::ifstream modelfile_;
  modelfile_.open(_model_filename_);
  if (modelfile_.is_open()) {
    const int kParamfileListBufflerLen = 1024;
    char readlinebuffer_[kParamfileListBufflerLen];
    bzero(readlinebuffer_, kParamfileListBufflerLen);
    while (modelfile_.good()) {
      bzero(readlinebuffer_, kParamfileListBufflerLen);
      modelfile_.getline(readlinebuffer_, kParamfileListBufflerLen);
      PerishableStringTokenizer st_(readlinebuffer_, kParamfileListBufflerLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() == 0) {
        continue;
      }
      if (strcmp(tokens_[0], "PORT: ") == 0) {
        unsigned int t_num_preds_ = atoi(tokens_[1]);
        if (tokens_.size() != 2 + 2 * t_num_preds_) {
          std::cerr << "Malformed PORT: line in model file .. exiting \n";
          exit(-1);
        }
        for (unsigned int t_ctr_ = 0; t_ctr_ < t_num_preds_; t_ctr_++) {
          double t_weight_ = atof(tokens_[3 + 2 * t_ctr_]);
          std::string t_shc_ = tokens_[2 + 2 * t_ctr_];
          _source_shortcode_vec_.push_back(t_shc_);
          _ors_shortcode_vec_.push_back(t_shc_);
        }
      }
    }
    modelfile_.close();
  }
  }*/

SignalBasedTrading::SignalBasedTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
                                       const std::vector<SecurityMarketView*> _dep_market_view_vec_,
                                       const std::vector<SmartOrderManager*> _order_manager_vec_,
                                       const std::vector<SBEParamSet*> _sbe_paramset_vec_, const bool _livetrading_,
                                       MultBasePNL* _mult_base_pnl_, int _trading_start_utc_mfm_,
                                       int _trading_end_utc_mfm_)
    : SBEInterface(_dbglogger_, _watch_, _dep_market_view_vec_, _order_manager_vec_, _sbe_paramset_vec_, _livetrading_,
                   _trading_start_utc_mfm_, _trading_end_utc_mfm_),
      mult_base_pnl_(_mult_base_pnl_),
      sec_name_indexer_(SecurityNameIndexer::GetUniqueInstance()) {
  number_of_contracts_ = dep_market_view_vec_.size();

  risk_from_exchange_.resize(number_of_contracts_, 0);
  risk_from_signal_.resize(number_of_contracts_, 0);

  mult_base_pnl_->AddListener(this);

  // execution support + processing orders ( in sim atleast )
  watch_.subscribe_BigTimePeriod(this);
  // l1 decisions ( ontrade we call l1_price_listeners )
  for (unsigned int i = 0; i < dep_market_view_vec_.size(); i++) {
    dep_market_view_vec_[i]->subscribe_price_type(this, kPriceTypeMktSizeWPrice);
    paramset_vec_[i]->SetSizeFactors(dep_market_view_vec_[i]->min_order_size());
    DBGLOG_TIME_CLASS_FUNC << paramset_vec_[i]->ToString() << DBGLOG_ENDL_FLUSH;
  }

  // hist information including volume and l1 size map

  // version 1: stay passive all along
  // version 2: begin passive and move to agg
  //            a) move to agg, if mkt_price if far
  //            b) move to agg, if pc1 > k and mkt_price is far
  //            c) move to agg, if participation is lower compared to mkt_volume
}

void SignalBasedTrading::OnNewOrderFromStrategy(std::string _instrument_, std::string _order_id_, int _order_lots_,
                                                double _ref_px_) {
  // _instrument_2_security_id_
  // _order_lots_ -> _buysell_ / _exec_quantity_
  // _new_position_ {we assume that there are no drops}
  // _ref_px_ -> _price_
  // _ref_px_ * x -> r_int_price_
  if (_order_lots_ != 0) {
    DBGLOG_TIME_CLASS_FUNC << "Instrument: " << _instrument_ << " OrderId: " << _order_id_
                           << " OrderLots: " << _order_lots_ << " RefPx: " << _ref_px_ << DBGLOG_ENDL_FLUSH;
  }
  // if existing position from strategy and new order has same sign there is nothing we have to dp
  // apparently getting id from vector is better than using map !
  int t_security_id_ = sec_name_indexer_.GetIdFromString(_instrument_);
  risk_from_signal_[t_security_id_] += (_order_lots_ * dep_market_view_vec_[t_security_id_]->min_order_size());
  L1TradingLogic(t_security_id_);
}

void SignalBasedTrading::OnNewPositionFromStrategy(std::string _instrument_, std::string _strat_id_,
                                                   int _position_lots_, double _new_ref_px_) {
  if (_position_lots_ != 0) {
    DBGLOG_TIME_CLASS_FUNC << "Instrument: " << _instrument_ << " StratId: " << _strat_id_
                           << " PositionLots: " << _position_lots_ << " NewRefPx: " << _new_ref_px_
                           << DBGLOG_ENDL_FLUSH;
  }

  int t_security_id_ = sec_name_indexer_.GetIdFromString(_instrument_);
  risk_from_signal_[t_security_id_] = (_position_lots_ * dep_market_view_vec_[t_security_id_]->min_order_size());
  L1TradingLogic(t_security_id_);
}

void SignalBasedTrading::OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
                                const double _price_, const int r_int_price_, const int _security_id_) {
  // first basic sanity checks
  // it is totally possible we might not have got enough time to cancel but we need to record this
  if (risk_from_signal_[_security_id_] != 0) {
    HFSAT::TradeType_t t_expected_side_ =
        (risk_from_signal_[_security_id_] > 0) ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;
    if (t_expected_side_ != _buysell_) {
      DBGLOG_TIME_CLASS_FUNC << "Fatal Error: OnExec: Received opposite side in exec than expected: "
                             << risk_from_signal_[_security_id_] << " " << risk_from_exchange_[_security_id_]
                             << " Some incorrect order netting for product or couldnt cancel ontime: " << _new_position_
                             << " " << dep_market_view_vec_[_security_id_]->shortcode() << DBGLOG_ENDL_FLUSH;
    }
  }
  // again it is totally possible we could exceed this, but still less the instances better it is
  if (std::abs(risk_from_signal_[_security_id_] - risk_from_exchange_[_security_id_]) < _exec_quantity_) {
    DBGLOG_TIME_CLASS_FUNC << "Fatal Error: OnExec Received greater order exec size than required for product: "
                           << dep_market_view_vec_[_security_id_]->shortcode() << DBGLOG_ENDL_FLUSH;
  }
  risk_from_exchange_[_security_id_] = _new_position_;
  // we will eventually receive onmarketupdate/ontradeprint call so not worrying about calling tradinglogic
}

void SignalBasedTrading::OnMarketUpdate(const unsigned int _security_id_,
                                        const HFSAT::MarketUpdateInfo& _market_update_info_) {
  // aggress cooloff could be function of l1_change
  // pause_state_with_no_mkt_orders
  // trading_state_with_l1_only
  // trading_start_with_l2
  // zerorisk_state
  L1TradingLogic(_security_id_);
}

// risk_from_exchange need to follow risk_from_signals in this mode
void SignalBasedTrading::L1TradingLogic(int _security_id_) {
  // block for agg order only
  if (paramset_vec_[_security_id_]->exec_algo_ == kAggOnly || paramset_vec_[_security_id_]->exec_algo_ == kPassAndAgg) {
  }

  // block for passive orders
  if ((paramset_vec_[_security_id_]->exec_algo_ == kPassAndAgg ||
       paramset_vec_[_security_id_]->exec_algo_ == kPassOnly)) {
    // side
    HFSAT::TradeType_t t_risk_side_ = (risk_from_signal_[_security_id_] - risk_from_exchange_[_security_id_] > 0)
                                          ? HFSAT::kTradeTypeBuy
                                          : HFSAT::kTradeTypeSell;

    // price
    int t_best_nonself_bid_int_price_ = dep_market_view_vec_[_security_id_]->bestbid_int_price();
    int t_best_nonself_ask_int_price_ = dep_market_view_vec_[_security_id_]->bestask_int_price();

    // size (our_risk_mandate/mkt_restrictions/required_risk/max_min_order_size)
    int t_required_size_ = std::abs(risk_from_signal_[_security_id_] - risk_from_exchange_[_security_id_]);
    int t_mkt_limit_ = ((int)(paramset_vec_[_security_id_]->l1size_limit_per_trade_ *
                              (dep_market_view_vec_[_security_id_]->bestbid_size() +
                               dep_market_view_vec_[_security_id_]->bestask_size()) /
                              dep_market_view_vec_[_security_id_]->min_order_size())) *
                       dep_market_view_vec_[_security_id_]->min_order_size();
    int t_size_to_execute_ = std::min(t_required_size_, t_mkt_limit_);

    // cancel any l1 orders
    // non_best_price let l2tradinglogic handle
    // placing orders, if doesnt exist already
    // treating unconfirmed and confirmed orders in similar fashion, given we dont intend be change states with in
    // seq2conf
    if (t_size_to_execute_ == 0) {
      order_manager_vec_[_security_id_]->CancelBidsEqAboveIntPrice(t_best_nonself_bid_int_price_);
      order_manager_vec_[_security_id_]->CancelAsksEqAboveIntPrice(t_best_nonself_ask_int_price_);
    } else if (t_risk_side_ == HFSAT::kTradeTypeBuy) {
      order_manager_vec_[_security_id_]->CancelAsksEqAboveIntPrice(t_best_nonself_ask_int_price_);
      // if (order_manager_vec_[_security_id_]->GetTotalBidSizeOrderedAtIntPx(t_best_nonself_bid_int_price_) == 0) {
      if (order_manager_vec_[_security_id_]->SumBidSizes() == 0) {
        DBGLOG_TIME_CLASS_FUNC << "Instrument: " << dep_market_view_vec_[_security_id_]->shortcode() << " mkt:["
                               << t_best_nonself_bid_int_price_ << "X" << t_best_nonself_ask_int_price_
                               << "] t_required_size_: " << t_required_size_ << " t_size_to_execute_"
                               << t_size_to_execute_ << " t_int_price_: " << t_best_nonself_bid_int_price_
                               << DBGLOG_ENDL_FLUSH;
        order_manager_vec_[_security_id_]->SendTradeIntPx(t_best_nonself_bid_int_price_, t_size_to_execute_,
                                                          t_risk_side_, 'B');
      }
    } else {
      order_manager_vec_[_security_id_]->CancelBidsEqAboveIntPrice(t_best_nonself_bid_int_price_);
      // if (order_manager_vec_[_security_id_]->GetTotalAskSizeOrderedAtIntPx(t_best_nonself_ask_int_price_) == 0) {
      if (order_manager_vec_[_security_id_]->SumAskSizes() == 0) {
        DBGLOG_TIME_CLASS_FUNC << "Instrument: " << dep_market_view_vec_[_security_id_]->shortcode() << " mkt:["
                               << t_best_nonself_bid_int_price_ << "X" << t_best_nonself_ask_int_price_
                               << "] t_required_size_: " << t_required_size_ << " t_size_to_execute_"
                               << t_size_to_execute_ << " t_int_price_: " << t_best_nonself_ask_int_price_
                               << DBGLOG_ENDL_FLUSH;
        order_manager_vec_[_security_id_]->SendTradeIntPx(t_best_nonself_ask_int_price_, t_size_to_execute_,
                                                          t_risk_side_, 'B');
      }
    }

    // immediate l2 attn needed if flag is not set otherwise ontimeperiod
    if (!paramset_vec_[_security_id_]->use_nonbest_support_) {
      order_manager_vec_[_security_id_]->CancelBidsBelowIntPrice(t_best_nonself_bid_int_price_);
      order_manager_vec_[_security_id_]->CancelAsksBelowIntPrice(t_best_nonself_ask_int_price_);
    }
  }
}

// risk_from_exchange need to zero irrespective of risk_from_signals
void SignalBasedTrading::GetFlat(int _security_id_) {}

void SignalBasedTrading::ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_) {
  int total_pnl_ = 0;
  int total_volume_ = 0;
  for (auto i = 0u; i < dep_market_view_vec_.size(); i++) {
    if (order_manager_vec_[i]->trade_volume() > 0) {
      std::cout << dep_market_view_vec_[i]->shortcode() << " "
                << order_manager_vec_[i]->base_pnl().ReportConservativeTotalPNL(_conservative_close_) << " "
                << " " << order_manager_vec_[i]->trade_volume() / (dep_market_view_vec_[i]->min_order_size()) << " "
                << " OrderManager Count "
                << (order_manager_vec_[i]->SendOrderCount() + order_manager_vec_[i]->CxlOrderCount() +
                    order_manager_vec_[i]->ModifyOrderCount()) << "\n";
    }
    total_pnl_ += order_manager_vec_[i]->base_pnl().ReportConservativeTotalPNL(_conservative_close_);
    total_volume_ += order_manager_vec_[i]->trade_volume() / (dep_market_view_vec_[i]->min_order_size());
  }
  printf("SIMRESULT %d %d %d %d %d %d\n", total_pnl_, total_volume_, 0, 0, 0, 0);
}
}
