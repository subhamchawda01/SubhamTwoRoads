/**
    \file IndicatorsCode/time_window_trade_info_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include <sstream>

#include "dvctrade/Indicators/time_window_trade_info_manager.hpp"

namespace HFSAT {

TimeWindowTradeInfoManager* TimeWindowTradeInfoManager::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                          const Watch& r_watch_,
                                                                          SecurityMarketView& _indep_market_view_,
                                                                          double _seconds_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _seconds_;

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TimeWindowTradeInfoManager*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new TimeWindowTradeInfoManager(t_dbglogger_, r_watch_, _indep_market_view_, _seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TimeWindowTradeInfoManager::TimeWindowTradeInfoManager(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                       SecurityMarketView& _indep_market_view_, double _seconds_)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      indep_market_view_(_indep_market_view_),
      is_ready_(false),
      last_mkt_price_(0),
      min_price_increment_(indep_market_view_.min_price_increment()),
      computing_avgpx_(false),
      computing_sumsz_(false),
      computing_sdvpx_(false),
      window_size_(_seconds_),
      num_bw_(std::max(0, (int)_seconds_)),  // = window_size_/basic_window_
      bw_count_(0),
      bw_size_(100),
      basic_window_(10),
      sum_bw_sz_(0),
      sum_bw_px_(0),
      sum_bw_px2_(0),
      last_recorded_num_trades_(0),
      num_trades_(0),
      sumpx_(0.0),
      sumpx2_(0.0),
      avgpx_(0.0),
      avgpx2_(0.0),
      sumsz_(0.0),
      sdvpx_(0.0) {
  watch_.subscribe_TimePeriod(this);

  InitializeValues();

  indep_market_view_.subscribe_tradeprints(this);

  /// this is done to get a constant source of pings to be able to decay values ...
  /// If this does not improve values dramatically
  /// then we can think about removing this
  indep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
}

void TimeWindowTradeInfoManager::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (is_ready_) {
    if (bw_count_ < bw_size_) {
      bw_count_++;
      sum_bw_sz_ += num_trades_ - last_recorded_num_trades_;
      sum_bw_px_ += (last_mkt_price_) / 100.0;
      sum_bw_px2_ += ((last_mkt_price_) / 100.0) * ((last_mkt_price_) / 100.0);
      last_recorded_num_trades_ = num_trades_;
    } else {
      //        std::cout<<sum_bw_px_<<" "<<sum_bw_px2_<<"\n";
      px_bwq_.push(sum_bw_px_);
      px2_bwq_.push(sum_bw_px2_);
      sz_bwq_.push(sum_bw_sz_);
      AdjustResults();
      if (px_bwq_.size() > num_bw_) {
        px_bwq_.pop();
        px2_bwq_.pop();
        sz_bwq_.pop();
      }

      sum_bw_sz_ = num_trades_ - last_recorded_num_trades_;
      sum_bw_px_ = (last_mkt_price_) / 100.0;
      sum_bw_px2_ = ((last_mkt_price_) / 100.0) * ((last_mkt_price_) / 100.0);
      last_recorded_num_trades_ = num_trades_;
      bw_count_ = 1;
    }
  }
}

void TimeWindowTradeInfoManager::OnMarketUpdate(const unsigned int _security_id_,
                                                const MarketUpdateInfo& _market_update_info_) {
  if (_market_update_info_.bestbid_int_price_ > 0 && _market_update_info_.bestask_int_price_ > 0) {
    is_ready_ = true;
    last_mkt_price_ = _market_update_info_.mkt_size_weighted_price_;
  }
}

void TimeWindowTradeInfoManager::OnTradePrint(const unsigned int _security_id_,
                                              const TradePrintInfo& _trade_print_info_,
                                              const MarketUpdateInfo& _market_update_info_) {
  num_trades_ += _trade_print_info_.size_traded_;
}

void TimeWindowTradeInfoManager::compute_sumsz() { computing_sumsz_ = true; }

void TimeWindowTradeInfoManager::compute_avgpx() { computing_avgpx_ = true; }

void TimeWindowTradeInfoManager::compute_sdvpx() { computing_sdvpx_ = true; }

void TimeWindowTradeInfoManager::AdjustResults() {
  sumpx_ += px_bwq_.back();
  sumpx2_ += px2_bwq_.back();
  if (px_bwq_.size() > num_bw_) {
    sumpx_ -= px_bwq_.front();
    sumpx2_ -= px2_bwq_.front();
    avgpx_ = 100.0 * (sumpx_ / (num_bw_ * bw_size_));
    avgpx2_ = 10000.0 * (sumpx2_ / (num_bw_ * bw_size_));
    // std::cout<<avgpx2_<<" "<<avgpx_*avgpx_<<"\n";
    sdvpx_ = sqrt(avgpx2_ - avgpx_ * avgpx_);
    // std::cout<<sdvpx_<<"\n";
  }
}

void TimeWindowTradeInfoManager::InitializeValues() {
  sumsz_ = 0;
  avgpx_ = 0;
  sdvpx_ = 0;
}
}
