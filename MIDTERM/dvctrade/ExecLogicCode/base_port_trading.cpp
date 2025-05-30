/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#include "dvctrade/ExecLogic/base_port_trading.hpp"

namespace HFSAT {
  BasePortTrading::BasePortTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
				   const std::vector<SecurityMarketView*> _dep_market_view_vec_,
				   const std::vector<SmartOrderManager*> _order_manager_vec_,
				   const bool _livetrading_, MultBasePNL* _mult_base_pnl_,
				   int _trading_start_utc_mfm_, int _trading_end_utc_mfm_)
    : PortfolioTradingInterface(_dbglogger_, _watch_, _dep_market_view_vec_, _order_manager_vec_, _livetrading_,
				_trading_start_utc_mfm_, _trading_end_utc_mfm_),
    mult_base_pnl_(_mult_base_pnl_),
    sec_name_indexer_(SecurityNameIndexer::GetUniqueInstance()) {

    risk_from_exchange_.resize(dep_market_view_vec_.size());
    risk_from_signal_.resize(dep_market_view_vec_.size());

    signal_tick_pnl_.resize(dep_market_view_vec_.size());
    real_tick_pnl_.resize(dep_market_view_vec_.size());
    // economic manager should be easier than what we have
    // just getflat for shortcode exchanges 3 degree event
    // but for now skipping ...
    
    // at this point there is no point subscribing to the pnl if we are not doing anything with it
    // only case, i see is trades_writer need the ability to compute risk 
    // pnl is straight_fwd ( sum of pnl )
    // risk is not ( but what if send vector of weights , 1 / (inst_uts), then it can compute both risk and pnl)
    // so not subsribing to mult_base_pnl (not really smart enough) for now !
    
    // subscription + expand param per shortcode
    for (unsigned int i = 0u; i < dep_market_view_vec_.size(); i++) {
      dep_market_view_vec_[i]->subscribe_price_type(this, kPriceTypeMktSizeWPrice);
      paramset_vec_[i]->scale_lot_values(dep_market_view_vec_[i]->min_order_size());
    }   
  }


  void BasePortTrading::OnNewPortRiskFromStrategy(int _strategy_id_, std::vector<int>& _signal_risk_vec_,
						  std::vector<double>& _ref_price_vec_) {
    for (unsigned int i = 0u; i < _signal_risk_vec_.size(); i++) {
      signal_tick_pnl_[i] -= ((_signal_risk_vec_[i] * dep_market_view_vec_[i]->min_order_size() - risk_from_signal_[i]) * _ref_price_vec_[i]);
      risk_from_signal_[i] = _signal_risk_vec_[i] * dep_market_view_vec_[i]->min_order_size();
      L1TradingLogic(i);
    }
  }

  void BasePortTrading::OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
			       const double _price_, const int r_int_price_, const int _security_id_) {

    if (risk_from_signal_[_security_id_] != 0) {
      HFSAT::TradeType_t t_expected_side_ = 
	(risk_from_signal_[_security_id_] > 0) ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;
      if (t_expected_side_ != _buysell_) {
	DBGLOG_TIME_CLASS_FUNC << "Fatal Error: OnExec: Received opposite side in exec than expected: "
			       << risk_from_signal_[_security_id_] << " " << risk_from_exchange_[_security_id_]
			       << " Some incorrect order netting for product or couldnt cancel ontime: "
			       << _new_position_ << " " << dep_market_view_vec_[_security_id_]->shortcode() << DBGLOG_ENDL_FLUSH;
      }
    }
    // again it is totally possible we could exceed this, but still less the instances better it is
    if (std::abs(risk_from_signal_[_security_id_] - risk_from_exchange_[_security_id_]) < _exec_quantity_) {
      DBGLOG_TIME_CLASS_FUNC << "Fatal Error: OnExec Received greater order exec size than required for product: "
		 << dep_market_view_vec_[_security_id_]->shortcode() << DBGLOG_ENDL_FLUSH;
    }
    risk_from_exchange_[_security_id_] = _new_position_;
    real_tick_pnl_[_security_id_] -= ((_buysell_ == HFSAT::kTradeTypeBuy ? (_exec_quantity_ * _price_) : (-_exec_quantity_ * _price_)));
    // we will eventually receive onmarketupdate/ontradeprint call so not worrying about calling tradinglogic
  }

  void BasePortTrading::OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
    L1TradingLogic(_security_id_);
  }

  // risk_from_exchange need to follow risk_from_signals in this mode
  void BasePortTrading::L1TradingLogic(int _security_id_) {

    // block for passive orders
    if ((paramset_vec_[_security_id_]->exec_algo_ == kPassAndAgg || paramset_vec_[_security_id_]->exec_algo_ == kPassOnly)) {
      // side
      HFSAT::TradeType_t t_risk_side_ = 
	(risk_from_signal_[_security_id_] - risk_from_exchange_[_security_id_] > 0) ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;

      // price
      int t_best_nonself_bid_int_price_ = dep_market_view_vec_[_security_id_]->bestbid_int_price();
      int t_best_nonself_ask_int_price_ = dep_market_view_vec_[_security_id_]->bestask_int_price();

    // size (our_risk_mandate/mkt_restrictions/required_risk/max_min_order_size)
      int t_required_size_ = std::abs(risk_from_signal_[_security_id_] - risk_from_exchange_[_security_id_]);
      int t_mkt_limit_ = ((int)(paramset_vec_[_security_id_]->l1perc_limit_ * 
				(dep_market_view_vec_[_security_id_]->bestbid_size() + dep_market_view_vec_[_security_id_]->bestask_size()) / dep_market_view_vec_[_security_id_]->min_order_size())) * 
	dep_market_view_vec_[_security_id_]->min_order_size();
      int t_size_to_execute_ = std::min(t_required_size_, t_mkt_limit_);

      // cancel any l1 orders
      // non_best_price let l2tradinglogic handle
      // placing orders, if doesnt exist already
      // treating unconfirmed and confirmed orders in similar fashion, given we dont intend be change states with in seq2conf
      if (t_size_to_execute_ == 0) {
	order_manager_vec_[_security_id_]->CancelBidsEqAboveIntPrice(t_best_nonself_bid_int_price_);
	order_manager_vec_[_security_id_]->CancelAsksEqAboveIntPrice(t_best_nonself_ask_int_price_);
      } else if (t_risk_side_ == HFSAT::kTradeTypeBuy) {
	order_manager_vec_[_security_id_]->CancelAsksEqAboveIntPrice(t_best_nonself_ask_int_price_);
	//if (order_manager_vec_[_security_id_]->GetTotalBidSizeOrderedAtIntPx(t_best_nonself_bid_int_price_) == 0) {
	if (order_manager_vec_[_security_id_]->SumBidSizes() == 0 && order_manager_vec_[_security_id_]->GetUnSequencedBids().size() == 0) {
	  DBGLOG_TIME_CLASS_FUNC << "Instrument: " << dep_market_view_vec_[_security_id_]->shortcode() << " mkt:[" << t_best_nonself_bid_int_price_
				 << "X" << t_best_nonself_ask_int_price_ << "] t_required_size_: " << t_required_size_ << " t_size_to_execute_" << t_size_to_execute_
				 << " t_int_price_: " << t_best_nonself_bid_int_price_
				 << DBGLOG_ENDL_FLUSH;
	  order_manager_vec_[_security_id_]->SendTradeIntPx(t_best_nonself_bid_int_price_, t_size_to_execute_, t_risk_side_, 'B');
	}
      } else {
	order_manager_vec_[_security_id_]->CancelBidsEqAboveIntPrice(t_best_nonself_bid_int_price_);
	//if (order_manager_vec_[_security_id_]->GetTotalAskSizeOrderedAtIntPx(t_best_nonself_ask_int_price_) == 0) {
	if (order_manager_vec_[_security_id_]->SumAskSizes() == 0 && order_manager_vec_[_security_id_]->GetUnSequencedAsks().size() == 0) {
	  DBGLOG_TIME_CLASS_FUNC << "Instrument: " << dep_market_view_vec_[_security_id_]->shortcode() << " mkt:[" << t_best_nonself_bid_int_price_
				 << "X" << t_best_nonself_ask_int_price_ << "] t_required_size_: " << t_required_size_ << " t_size_to_execute_" << t_size_to_execute_
				 << " t_int_price_: " << t_best_nonself_ask_int_price_
				 << DBGLOG_ENDL_FLUSH;
	  order_manager_vec_[_security_id_]->SendTradeIntPx(t_best_nonself_ask_int_price_, t_size_to_execute_, t_risk_side_, 'B');
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
  void BasePortTrading::GetFlat(int _security_id_) {}

  void BasePortTrading::ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_) {
    int total_pnl_ = 0;
    int total_volume_ = 0;
    for (auto i = 0u; i < dep_market_view_vec_.size(); i++) {
      if (order_manager_vec_[i]->trade_volume() > 0) {
	std::cout << dep_market_view_vec_[i]->shortcode() 
		  << " Pnl: " << order_manager_vec_[i]->base_pnl().ReportConservativeTotalPNL(_conservative_close_)
		  << " Vol: " << order_manager_vec_[i]->trade_volume() / (dep_market_view_vec_[i]->min_order_size())
		  << " OMCount: " << (order_manager_vec_[i]->SendOrderCount() + order_manager_vec_[i]->CxlOrderCount() + 
				      order_manager_vec_[i]->ModifyOrderCount())
		  << " STP: " << (signal_tick_pnl_[i] * SecurityDefinitions::contract_specification_map_[dep_market_view_vec_[i]->shortcode()].numbers_to_dollars_)
		  << " RTP: " << (real_tick_pnl_[i] * SecurityDefinitions::contract_specification_map_[dep_market_view_vec_[i]->shortcode()].numbers_to_dollars_) << "\n";
      }
      total_pnl_ += order_manager_vec_[i]->base_pnl().ReportConservativeTotalPNL(_conservative_close_);
      total_volume_ += order_manager_vec_[i]->trade_volume() / (dep_market_view_vec_[i]->min_order_size());
    }
    printf("SIMRESULT %d %d %d %d %d %d\n", total_pnl_, total_volume_,0 ,0 ,0 ,0);
  }
}
