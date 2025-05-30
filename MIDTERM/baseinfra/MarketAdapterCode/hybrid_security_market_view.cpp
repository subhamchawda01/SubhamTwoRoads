/**
   \file MarketAdapterCode/hybrid_security_market_view.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include <iostream>
#include <fstream>
#include <strings.h>
#include <stdlib.h>
#include <ctime>

#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CDef/security_definitions.hpp"

#include "baseinfra/MarketAdapter/normal_spread_manager.hpp"
#include "baseinfra/MarketAdapter/hybrid_security_market_view.hpp"

namespace HFSAT {

HybridSecurityMarketView::HybridSecurityMarketView(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                                   SecurityNameIndexer& t_sec_name_indexer_,
                                                   const std::string& t_shortcode_, const char* t_exchange_symbol_,
                                                   const unsigned int t_security_id_, const ExchSource_t t_exch_source_,
                                                   SecurityMarketView* p_smv1_, SecurityMarketView* p_smv2_,
                                                   const std::string& t_offline_mix_mms_wts_filename_,
                                                   const std::string& t_online_mix_price_const_filename_,
                                                   const std::string& t_online_beta_kalman_const_filename_)
    : SecurityMarketView(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_shortcode_, t_exchange_symbol_, t_security_id_,
                         t_exch_source_, false, t_offline_mix_mms_wts_filename_, t_online_mix_price_const_filename_,
                         t_online_beta_kalman_const_filename_),
      flag_switch(false),
      s_alpha(0.0),
      s_beta(0.0),
      comp1_smv_(p_smv1_),
      comp2_smv_(p_smv2_) {
  watch_.subscribe_TimePeriod(this);
  comp1_smv_->subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  comp1_smv_->subscribe_price_type(this, kPriceTypeMidprice);
  comp2_smv_->subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  comp2_smv_->subscribe_price_type(this, kPriceTypeMidprice);
  market_update_info_.asklevels_.push_back(*(new MarketUpdateInfoLevelStruct()));
  market_update_info_.bidlevels_.push_back(*(new MarketUpdateInfoLevelStruct()));
}

void HybridSecurityMarketView::subscribe_OnReady(SecurityMarketViewOnReadyListener* t_new_listener_) {
  VectorUtils::UniqueVectorAdd(onready_listeners_, t_new_listener_);
  comp1_smv_->subscribe_OnReady(this);
  comp2_smv_->subscribe_OnReady(this);
}

// listener interface
void HybridSecurityMarketView::OnMarketUpdate(const unsigned int _security_id_,
                                              const MarketUpdateInfo& cr_market_update_info_) {
  if (flag_switch && _security_id_ == comp2_smv_->market_update_info_.security_id_) {
    // pass NASDAQ data
    UpdateMarketUpdateInfo(cr_market_update_info_);
    for (auto i = 0u; i < l1_price_listeners_.size(); i++) {
      l1_price_listeners_[i]->OnMarketUpdate(market_update_info_.security_id_,
                                             market_update_info_);  // TODO pass hybrid security_id_
      l1_size_listeners_[i]->OnMarketUpdate(market_update_info_.security_id_,
                                            market_update_info_);  // TODO pass hybrid security_id_
    }
  } else if (!flag_switch && _security_id_ == comp1_smv_->market_update_info_.security_id_) {
    // pass CME data
    UpdateMarketUpdateInfo(cr_market_update_info_);
    for (auto i = 0u; i < l1_price_listeners_.size(); i++) {
      l1_price_listeners_[i]->OnMarketUpdate(market_update_info_.security_id_,
                                             market_update_info_);  // TODO pass hybrid security_id_
      l1_size_listeners_[i]->OnMarketUpdate(market_update_info_.security_id_,
                                            market_update_info_);  // TODO pass hybrid security_id_
    }
  }
}

void HybridSecurityMarketView::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                            const MarketUpdateInfo& _market_update_info_) {
  if (flag_switch && _security_id_ == comp2_smv_->market_update_info_.security_id_) {
    // pass NASDAQ data
    UpdateMarketUpdateInfo(_market_update_info_);
    UpdateTradePrintInfo(_trade_print_info_);
    for (auto i = 0u; i < l1_price_listeners_.size(); i++) {
      l1_price_listeners_[i]->OnTradePrint(market_update_info_.security_id_, trade_print_info_, market_update_info_);
    }
  } else if (!flag_switch && _security_id_ == comp1_smv_->market_update_info_.security_id_) {
    // pass CME data
    UpdateMarketUpdateInfo(_market_update_info_);
    UpdateTradePrintInfo(_trade_print_info_);
    for (auto i = 0u; i < l1_price_listeners_.size(); i++) {
      l1_price_listeners_[i]->OnTradePrint(market_update_info_.security_id_, trade_print_info_, market_update_info_);
    }
  }
}

void HybridSecurityMarketView::UpdateMarketUpdateInfo(const MarketUpdateInfo& _market_update_info_) {
  if (flag_switch) {
    market_update_info_.mkt_size_weighted_price_ = _market_update_info_.mkt_size_weighted_price_ * s_alpha + s_beta;
  } else {
    market_update_info_.mkt_size_weighted_price_ = _market_update_info_.mkt_size_weighted_price_;
  }

  market_update_info_.mid_price_ = market_update_info_.mkt_size_weighted_price_;
  market_update_info_.mkt_sinusoidal_price_ = market_update_info_.mkt_size_weighted_price_;
  market_update_info_.order_weighted_price_ = market_update_info_.mkt_size_weighted_price_;
  market_update_info_.offline_mix_mms_price_ = market_update_info_.mkt_size_weighted_price_;
  market_update_info_.online_mix_price_ = market_update_info_.mkt_size_weighted_price_;
}

void HybridSecurityMarketView::UpdateTradePrintInfo(const TradePrintInfo& _trade_print_info_) {
  trade_print_info_ = _trade_print_info_;
  if (flag_switch) {
    trade_print_info_.trade_price_ = trade_print_info_.trade_price_ * s_alpha + s_beta;
    trade_print_info_.int_trade_price_ = int(trade_print_info_.int_trade_price_ * s_alpha + s_beta);
    trade_print_info_.tradepx_mktpx_diff_ = trade_print_info_.tradepx_mktpx_diff_ * s_alpha + s_beta;
  }
}

void HybridSecurityMarketView::GetRegressionCoeffs() {
  if (market_update_info_.shortcode_.compare("HYB_ESPY") == 0) {
    s_alpha = 976.5;
    s_beta = 2840.0;
  } else if (market_update_info_.shortcode_.compare("HYB_NQQQ") == 0) {
    s_alpha = 3752.5;
    s_beta = 22000.0;
  } else if (market_update_info_.shortcode_.compare("HYB_YDIA") == 0) {
    s_alpha = 99.48;
    s_beta = 26.8;
  }

  // comp1 is ES_0, comp2 is SPY
  // we map SPY prices to ES_0, i.e, ES_0:y,SPY:x
  /*
    double mean1_ = VectorUtils::GetMean(comp1_prices_);
    double mean2_ = VectorUtils::GetMean(comp2_prices_);
    s_alpha = (VectorUtils::CalcDotProduct(comp1_prices_,comp2_prices_) -
    mean1_*mean1_)/(VectorUtils::CalcL2Norm(comp2_prices_) + mean1_*mean2_) ;
    s_beta = mean1_ - s_alpha*mean2_;
    //std::cout<<"Regression Coefficients : "<<s_alpha<<" "<<s_beta<<"\n";
    */
}
}
