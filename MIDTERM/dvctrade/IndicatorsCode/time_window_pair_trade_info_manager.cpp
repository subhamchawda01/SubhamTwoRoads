/**
    \file IndicatorsCode/time_window_pair_trade_info_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include <sstream>

#include "dvctrade/Indicators/time_window_pair_trade_info_manager.hpp"

namespace HFSAT {

TimeWindowPairTradeInfoManager* TimeWindowPairTradeInfoManager::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& _indep_market_view1_,
    SecurityMarketView& _indep_market_view2_, double _seconds_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view1_.secname() << ' ' << _indep_market_view2_.secname() << ' '
              << _seconds_;

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TimeWindowPairTradeInfoManager*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new TimeWindowPairTradeInfoManager(
        t_dbglogger_, r_watch_, _indep_market_view1_, _indep_market_view2_, _seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TimeWindowPairTradeInfoManager::TimeWindowPairTradeInfoManager(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                               SecurityMarketView& _indep_market_view1_,
                                                               SecurityMarketView& _indep_market_view2_,
                                                               double _seconds_)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      indep1_time_window_trade_info_manager_(
          *(TimeWindowTradeInfoManager::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view1_, _seconds_))),
      indep2_time_window_trade_info_manager_(
          *(TimeWindowTradeInfoManager::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view2_, _seconds_))),
      indep_market_view1_(_indep_market_view1_),
      indep_market_view2_(_indep_market_view2_),
      is_ready1_(false),
      is_ready2_(false),
      last_market_update_msecs_(-1),
      indep1_last_mkt_price_(0),
      indep2_last_mkt_price_(0),
      indep1_min_price_increment_(indep_market_view1_.min_price_increment()),
      indep2_min_price_increment_(indep_market_view2_.min_price_increment()),
      computing_beta_(false),
      computing_correlation_(false),
      window_size_(_seconds_),
      num_bw_(std::max(0, (int)_seconds_)),  // = window_size_/basic_window_
      bw_count_(0),
      bw_size_(100),
      basic_window_(10),
      indep1_sum_bw_sz_(0),
      indep1_sum_bw_px_(0),
      indep1_sum_bw_px2_(0),
      indep2_sum_bw_sz_(0),
      indep2_sum_bw_px_(0),
      indep2_sum_bw_px2_(0),
      sum_bw_px_prod_(0),
      sumpxprod_(0.0),
      indep1_sumpx_(0.0),
      indep1_sumpx2_(0.0),
      indep1_avgpx_(0.0),
      indep1_avgpx2_(0.0),
      indep1_sumsz_(0.0),
      indep1_sdvpx_(0.0),
      indep2_sumpx_(0.0),
      indep2_sumpx2_(0.0),
      indep2_avgpx_(0.0),
      indep2_avgpx2_(0.0),
      indep2_sumsz_(0.0),
      indep2_sdvpx_(0.0),
      beta_(0.0),
      correlation_(0.0) {
  watch_.subscribe_TimePeriod(this);

  InitializeValues();

  indep_market_view1_.subscribe_tradeprints(this);
  indep_market_view2_.subscribe_tradeprints(this);

  num_bw_ = (int)(window_size_ / basic_window_);

  /// this is done to get a constant source of pings to be able to decay values ...
  /// If this does not improve values dramatically
  /// then we can think about removing this
  indep_market_view1_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  indep_market_view2_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
}

void TimeWindowPairTradeInfoManager::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (is_ready1_ && is_ready2_ && (last_market_update_msecs_ < 0 ||
                                   watch_.msecs_from_midnight() - last_market_update_msecs_ < basic_window_ * 1000)) {
    if (bw_count_ < bw_size_) {
      bw_count_++;
      indep1_sum_bw_px_ += (indep1_last_mkt_price_) / 100.0;
      indep1_sum_bw_px2_ += ((indep1_last_mkt_price_) / 100.0) * ((indep1_last_mkt_price_) / 100.0);
      indep2_sum_bw_px_ += (indep2_last_mkt_price_) / 100.0;
      indep2_sum_bw_px2_ += ((indep2_last_mkt_price_) / 100.0) * ((indep2_last_mkt_price_) / 100.0);
      sum_bw_px_prod_ += ((indep1_last_mkt_price_) / 100.0) * ((indep2_last_mkt_price_) / 100.0);
    } else {
      px_prod_bwq_.push(sum_bw_px_prod_);
      indep1_px_bwq_.push(indep1_sum_bw_px_);
      indep1_px2_bwq_.push(indep1_sum_bw_px2_);
      indep2_px_bwq_.push(indep2_sum_bw_px_);
      indep2_px2_bwq_.push(indep2_sum_bw_px2_);
      AdjustResults();
      if (px_prod_bwq_.size() > num_bw_) {
        px_prod_bwq_.pop();
        indep1_px2_bwq_.pop();
        indep1_px_bwq_.pop();
        indep2_px2_bwq_.pop();
        indep2_px_bwq_.pop();
      }

      sum_bw_px_prod_ = ((indep1_last_mkt_price_) / 100.0) * ((indep2_last_mkt_price_) / 100.0);
      indep1_sum_bw_px_ = (indep1_last_mkt_price_) / 100.0;
      indep1_sum_bw_px2_ = ((indep1_last_mkt_price_) / 100.0) * ((indep1_last_mkt_price_) / 100.0);
      indep2_sum_bw_px_ = (indep2_last_mkt_price_) / 100.0;
      indep2_sum_bw_px2_ = ((indep2_last_mkt_price_) / 100.0) * ((indep2_last_mkt_price_) / 100.0);
      bw_count_ = 1;
    }
  }
}

void TimeWindowPairTradeInfoManager::OnMarketUpdate(const unsigned int _security_id_,
                                                    const MarketUpdateInfo& _market_update_info_) {
  last_market_update_msecs_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % int(basic_window_ * 1000));
  if (_security_id_ == indep_market_view1_.security_id() && _market_update_info_.bestbid_int_price_ > 0 &&
      _market_update_info_.bestask_int_price_ > 0) {
    is_ready1_ = true;
  }

  if (_security_id_ == indep_market_view2_.security_id() && _market_update_info_.bestbid_int_price_ > 0 &&
      _market_update_info_.bestask_int_price_ > 0) {
    is_ready2_ = true;
  }

  if (_security_id_ == indep_market_view1_.security_id() && is_ready1_) {
    indep1_last_mkt_price_ = _market_update_info_.mkt_size_weighted_price_;
  }

  if (_security_id_ == indep_market_view2_.security_id() && is_ready2_) {
    indep2_last_mkt_price_ = _market_update_info_.mkt_size_weighted_price_;
  }
}

void TimeWindowPairTradeInfoManager::OnTradePrint(const unsigned int _security_id_,
                                                  const TradePrintInfo& _trade_print_info_,
                                                  const MarketUpdateInfo& _market_update_info_) {}

void TimeWindowPairTradeInfoManager::compute_beta() { computing_beta_ = true; }

void TimeWindowPairTradeInfoManager::compute_correlation() { computing_correlation_ = true; }

void TimeWindowPairTradeInfoManager::AdjustResults() {
  sumpxprod_ += px_prod_bwq_.back();
  indep1_sumpx_ += indep1_px_bwq_.back();
  indep1_sumpx2_ += indep1_px2_bwq_.back();
  indep2_sumpx_ += indep2_px_bwq_.back();
  indep2_sumpx2_ += indep2_px2_bwq_.back();

  if (px_prod_bwq_.size() > num_bw_) {
    sumpxprod_ -= px_prod_bwq_.front();
    indep1_sumpx_ -= indep1_px_bwq_.front();
    indep1_sumpx2_ -= indep1_px2_bwq_.front();
    indep2_sumpx_ -= indep2_px_bwq_.front();
    indep2_sumpx2_ -= indep2_px2_bwq_.front();

    indep1_avgpx_ = (100.0 * indep1_sumpx_ / (num_bw_ * bw_size_));
    indep1_avgpx2_ = 10000.0 * (indep1_sumpx2_ / (num_bw_ * bw_size_));

    indep2_avgpx_ = (100.0 * indep2_sumpx_ / (num_bw_ * bw_size_));
    indep2_avgpx2_ = 10000.0 * (indep2_sumpx2_ / (num_bw_ * bw_size_));
    if (indep1_avgpx2_ - indep1_avgpx_ * indep1_avgpx_ < 0)
      indep1_sdvpx_ = 0.0;
    else
      indep1_sdvpx_ = sqrt(indep1_avgpx2_ - indep1_avgpx_ * indep1_avgpx_);

    if (indep2_avgpx2_ - indep2_avgpx_ * indep2_avgpx_ < 0)
      indep2_sdvpx_ = 0.0;
    else
      indep2_sdvpx_ = sqrt(indep2_avgpx2_ - indep2_avgpx_ * indep2_avgpx_);

    if (indep1_sdvpx_ < 0.0001 || indep2_sdvpx_ < 0.0001) {
      correlation_ = 0.0;
      beta_ = 0.0;
    } else {
      correlation_ = (10000.0 * (sumpxprod_ / (num_bw_ * bw_size_)) - (indep1_avgpx_ * indep2_avgpx_)) /
                     (indep1_sdvpx_ * indep2_sdvpx_);
      beta_ = (10000.0 * (sumpxprod_ / (num_bw_ * bw_size_)) - (indep1_avgpx_ * indep2_avgpx_)) /
              (indep1_sdvpx_ * indep1_sdvpx_);
    }
    std::cout << correlation_ << " " << beta_ << "\n";
  }
}

void TimeWindowPairTradeInfoManager::InitializeValues() {
  sumpxprod_ = 0;
  beta_ = 0;
  correlation_ = 0;
}
}
