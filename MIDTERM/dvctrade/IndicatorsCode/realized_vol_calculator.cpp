/**
    \file IndicatorsCode/realized_vol_calculator.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/realized_vol_calculator.hpp"

namespace HFSAT {
void RealizedVolCalculator::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                              std::vector<std::string>& _ors_source_needed_vec_,
                                              const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

RealizedVolCalculator* RealizedVolCalculator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const std::vector<const char*>& r_tokens_,
                                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_  _local_histvol_in_minutes_ _look_back_days_
  // _price_type_

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atoi(r_tokens_[4]), atoi(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

RealizedVolCalculator* RealizedVolCalculator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const SecurityMarketView& _indep_market_view_,
                                                                const unsigned int t_local_histvol_in_minutes_,
                                                                const unsigned int t_look_back_days_,
                                                                PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << t_local_histvol_in_minutes_ << ' '
              << t_look_back_days_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  // static std::map<std::string, RealizedVolCalculator*> concise_indicator_description_map_;
  if (global_concise_indicator_description_map_.find(concise_indicator_description_) ==
      global_concise_indicator_description_map_.end()) {
    global_concise_indicator_description_map_[concise_indicator_description_] =
        new RealizedVolCalculator(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                                  t_local_histvol_in_minutes_, t_look_back_days_, _price_type_);
  }
  return dynamic_cast<RealizedVolCalculator*>(
      global_concise_indicator_description_map_[concise_indicator_description_]);
}

RealizedVolCalculator::RealizedVolCalculator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                             const std::string& concise_indicator_description_,
                                             const SecurityMarketView& _indep_market_view_,
                                             const unsigned int t_local_histvol_period_in_minutes_,
                                             const unsigned int t_look_back_days_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      local_vol_history_(std::min(300u, t_local_histvol_period_in_minutes_)),
      look_back_days_(t_look_back_days_),
      price_type_(_price_type_),
      current_indep_price_(0),
      last_price_recorded_(0),
      current_histvol_rseries_(0),
      current_histvol_n_(0),
      current_histvol_(0),
      vol_history_decay_(0),
      inverse_vol_history_decay_(0),
      histvol_(0),
      previous_histvol_(0),
      realized_vol_calculator_listener_ptr_vec_() {
  // Historical Realized Volatility
  // no stock prices, going with futures instead
  unsigned int i =0;
  std::vector <double> prices_;
  int yyyymmdd_ = watch_.YYYYMMDD();
  std::map<std::string, std::string> t_shortcode_2_token_map_;

  while(i < look_back_days_) {
    std::string price_ = HFSAT::NSESecurityDefinitions::GetBhavCopyToken(yyyymmdd_, indep_market_view_.shortcode().c_str(), 10, "STKFUT", t_shortcode_2_token_map_);
    prices_.push_back(atof(price_.c_str()));    
    yyyymmdd_ = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", yyyymmdd_);
    i++;
  }
    // not sure about decaying returns instead should we decay different lookback histvols
    // well 
    // i = 0 corresponds to yesterday closing price:
  double ralpha_  = MathUtils::CalcDecayFactor(prices_.size());
  std::vector<double> rdecay_vector_;
  for (unsigned int k = 0; k < prices_.size(); k++) {
    rdecay_vector_.push_back(pow(ralpha_, (int)k));
  }

  std::vector<double> returns_;
  i = 0;
  while (i < (prices_.size() - 1)) {
    returns_.push_back(log(prices_[i] / prices_[i + 1]));
    i++;
  }

  double realized_vol_ = 0;
  for (unsigned int j = 0; j < returns_.size(); j++) {
    realized_vol_ += (rdecay_vector_[j + 1]) * (returns_[j] * returns_[j]);
  }
  histvol_ = 100 * sqrt((252 / returns_.size()) * realized_vol_);
  // Todays Intraday Volatility
  // compute returns every minute
  // last_price_ current_price
  vol_history_decay_ = MathUtils::CalcDecayFactor((6 * 60) / local_vol_history_);
  inverse_vol_history_decay_ = 1 - vol_history_decay_;
  indep_market_view_.subscribe_price_type(NULL, price_type_);
  watch_.subscribe_OneMinutePeriod(this);
  // subscribe watch
}

void RealizedVolCalculator::OnTimePeriodUpdate(const int num_pages_to_add_) {
  // ignoring little discrepancies since time call back are driven by mkt_events
  // this_return_ = log ( current_price_ / last_price_  )
  // we need to decay giving importance to recent volatility
  // we have h0 h1 h2 h3 h4 h5 ... hn
  // if look back is 15 minutes we make one hi, we multiply hj, for all j < i with a decay factor
  // the length of such would be end_time - start_time in minutes.
  // if we assume half life of such series is 24
  current_indep_price_ = indep_market_view_.price_from_type(price_type_);
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex2(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    double this_return_;
    this_return_ = log(current_indep_price_ / last_price_recorded_);

    current_histvol_rseries_ += this_return_ * this_return_;
    current_histvol_n_++;
    current_histvol_ = 100 * sqrt((6 * 60 * 252 / current_histvol_n_) * current_histvol_rseries_);
    last_price_recorded_ = current_indep_price_;

    if (current_histvol_n_ == local_vol_history_) {
      // first decay existing
      // histvol_ *= vol_history_decay_;
      // add new
      // histvol_ += inverse_vol_history_decay_*current_histvol_;

      histvol_ = histvol_ - previous_histvol_ * inverse_vol_history_decay_;
      histvol_ = histvol_ * vol_history_decay_;
      histvol_ = histvol_ + current_histvol_ * inverse_vol_history_decay_;

      current_histvol_rseries_ = 0;
      current_histvol_n_ = 0;
    } else {
      // adjust the recent hist_vol_values_
      histvol_ += (current_histvol_ - previous_histvol_) * (inverse_vol_history_decay_);
      previous_histvol_ = current_histvol_;
    }
  }

  indicator_value_ = histvol_;
  // mean nonzero value, but just of ease of access
  NotifyIndicatorListeners(indicator_value_);
}

void RealizedVolCalculator::InitializeValues() {
  last_price_recorded_ = current_indep_price_;
  current_histvol_rseries_ = 0;
  current_histvol_n_ = 0;
  current_histvol_ = 0;
  indicator_value_ = 0;
}

void RealizedVolCalculator::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                    const int msecs_since_last_receive_) {
  data_interrupted_ = true;
}

void RealizedVolCalculator::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
