/*
 * spread_market_view.cpp
 *
 *  Created on: 05-Jun-2014
 *      Author: archit
 */

#include "baseinfra/MarketAdapter/synthetic_market_view.hpp"

namespace HFSAT {

SyntheticMarketView::SyntheticMarketView(DebugLogger &_dbglogger_, const Watch &_watch_,
                                         const std::string &_spread_shc_)
    : dbglogger_(_dbglogger_),
      watch_(_watch_),
      spread_shc_(_spread_shc_),
      shortcode_vec_(),
      weights_vec_(),
      smv_vec_(),
      secid_to_idx_map_(),
      ready_vec_(),
      is_ready_(false),
      is_dv01_not_updated_(true),
      is_di_spread_(false),
      bestbid_price_(0.0),
      bestask_price_(0.0),
      bestbid_size_(0),
      bestask_size_(0),
      last_bestbid_price_(),
      last_bestask_price_(),
      last_bestbid_size_(),
      last_bestask_size_(),
      dv01_vec_(),
      tradingdate_(_watch_.YYYYMMDD()) {
  watch_.subscribe_BigTimePeriod(this);
  HFSAT::SyntheticSecurityManager &synthetic_security_manager_ = HFSAT::SyntheticSecurityManager::GetUniqueInstance();

  synth_smv_ = HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance().GetSecurityMarketView(spread_shc_);
  shortcode_vec_ = synthetic_security_manager_.GetConstituentSHC(spread_shc_);
  weights_vec_ = synthetic_security_manager_.GetConstituentWeights(spread_shc_);

  for (auto i = 0u; i < shortcode_vec_.size(); i++) {
    secid_to_idx_map_[HFSAT::SecurityNameIndexer::GetUniqueInstance().GetIdFromString(shortcode_vec_[i])] = i;
    smv_vec_.push_back(
        HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance().GetSecurityMarketView(shortcode_vec_[i]));
    smv_vec_[i]->subscribe_price_type(this, kPriceTypeMktSizeWPrice);
    ready_vec_.push_back(false);
    last_bestbid_price_.push_back(0.0);
    last_bestask_price_.push_back(0.0);
    last_bestask_size_.push_back(0);
    last_bestbid_size_.push_back(0);
    dv01_vec_.push_back(-1);
  }

  if (spread_shc_.find("SPDI1") != std::string::npos) {
    is_di_spread_ = true;
  }
}

void SyntheticMarketView::OnMarketUpdate(const unsigned int _security_id_,
                                         const MarketUpdateInfo &_market_update_info_) {
  // TODO:: Need to take care of cases where best level don't have enough sizes
  // we can also switch to demand driven variables, instead of updating on every update
  int t_update_idx_ = secid_to_idx_map_[_security_id_];
  last_bestbid_price_[t_update_idx_] = _market_update_info_.bestbid_price_;
  last_bestask_price_[t_update_idx_] = _market_update_info_.bestask_price_;
  last_bestbid_size_[t_update_idx_] = _market_update_info_.bestask_size_;
  last_bestask_size_[t_update_idx_] = _market_update_info_.bestbid_size_;

  if (!is_ready_) {
    ready_vec_[t_update_idx_] = true;
    is_ready_ = HFSAT::VectorUtils::CheckAllForValue(ready_vec_, true);

    if (is_di_spread_) {
      dv01_vec_[t_update_idx_] = HFSAT::CurveUtils::stirs_fut_dv01(shortcode_vec_[t_update_idx_], tradingdate_,
                                                                   last_bestbid_price_[t_update_idx_]);
    } else {
      dv01_vec_[t_update_idx_] = 1.0;
    }

    is_dv01_not_updated_ = HFSAT::VectorUtils::LinearSearchValue(dv01_vec_, -1.0);
    if (is_ready_ && !is_dv01_not_updated_) {
      InitializeBestVars();
      //      NotifyListeners();
    }
    return;
  }

  bestbid_price_ = weights_vec_[0] * last_bestbid_price_[0] + weights_vec_[1] * last_bestask_price_[1];
  bestask_price_ = weights_vec_[0] * last_bestask_price_[0] + weights_vec_[1] * last_bestbid_price_[1];

  bestbid_size_ = std::min(last_bestbid_size_[0],
                           MathUtils::GetCeilMultipleOf((int)(dv01_vec_[1] / dv01_vec_[0] * last_bestask_size_[1]),
                                                        smv_vec_[1]->min_order_size()));
  bestask_size_ = std::min(last_bestask_size_[0],
                           MathUtils::GetCeilMultipleOf((int)(dv01_vec_[1] / dv01_vec_[0] * last_bestbid_size_[1]),
                                                        smv_vec_[1]->min_order_size()));

  synth_smv_->base_bid_index_ = 100;
  synth_smv_->base_ask_index_ = 100;

  synth_smv_->market_update_info_.bidlevels_[synth_smv_->base_bid_index_].limit_price_ = bestbid_price_;
  synth_smv_->market_update_info_.bidlevels_[synth_smv_->base_bid_index_].limit_int_price_ =
      synth_smv_->GetIntPx(bestbid_price_);
  synth_smv_->market_update_info_.asklevels_[synth_smv_->base_ask_index_].limit_price_ = bestask_price_;
  synth_smv_->market_update_info_.asklevels_[synth_smv_->base_ask_index_].limit_int_price_ =
      synth_smv_->GetIntPx(bestask_price_);
  synth_smv_->market_update_info_.bidlevels_[synth_smv_->base_bid_index_].limit_size_ = bestbid_size_;
  synth_smv_->market_update_info_.asklevels_[synth_smv_->base_ask_index_].limit_size_ = bestask_size_;
  synth_smv_->market_update_info_.bidlevels_[synth_smv_->base_bid_index_].limit_ordercount_ = 1;
  synth_smv_->market_update_info_.asklevels_[synth_smv_->base_ask_index_].limit_ordercount_ = 1;

  if (!synth_smv_->is_ready_) {
    synth_smv_->is_ready_ = true;
  }

  synth_smv_->UpdateBestBid(synth_smv_->base_bid_index_);
  synth_smv_->UpdateBestAsk(synth_smv_->base_ask_index_);
  synth_smv_->UpdateL1Prices();
  synth_smv_->NotifyL1PriceListeners();
}

void SyntheticMarketView::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (!is_dv01_not_updated_) {
    if (is_di_spread_) {
      for (auto i = 0u; i < shortcode_vec_.size(); i++) {
        dv01_vec_[i] = HFSAT::CurveUtils::stirs_fut_dv01(shortcode_vec_[i], tradingdate_, last_bestbid_price_[i]);
      }
    }
  }
}

void SyntheticMarketView::InitializeBestVars() {
  bestbid_price_ = 0.0;
  bestask_price_ = 0.0;
  bestbid_size_ = 0;
  bestask_size_ = 0;
  // ShowBook();
}

void SyntheticMarketView::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                                       const MarketUpdateInfo &_market_update_info_) {
  OnMarketUpdate(_security_id_, _market_update_info_);
}

void SyntheticMarketView::ShowBook() const {
  DBGLOG_TIME_CLASS_FUNC_LINE << "SpreadMArketView " << shortcode() << " "
                              << "BS: " << bestbid_size_ << " "
                              << "BP: " << bestbid_price_ << " "
                              << "AP: " << bestask_price_ << " "
                              << "AS: " << bestask_size_ << " " << DBGLOG_ENDL_FLUSH;
}

void SyntheticMarketView::PrintBook() const {
  std::cout << watch_.tv().ToString() << " SpreadMArketView " << shortcode() << " "
            << "BS: " << bestbid_size_ << " "
            << "BP: " << bestbid_price_ << " "
            << "AP: " << bestask_price_ << " "
            << "AS: " << bestask_size_ << " " << std::endl;
}
} /* namespace HFSAT */
