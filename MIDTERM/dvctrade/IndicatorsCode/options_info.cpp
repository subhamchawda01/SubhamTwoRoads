/**
    \file IndicatorsCode/options_info.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/options_info.hpp"


namespace HFSAT {

void OptionsInfo::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                    std::vector<std::string>& _ors_source_needed_vec_,
                                    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_,
                               NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(r_tokens_[3]));
}

OptionsInfo* OptionsInfo::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _options_shc_ _options_info_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(
      NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(r_tokens_[3]));
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atoi(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

OptionsInfo* OptionsInfo::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const SecurityMarketView& _option_market_view_, int _options_info_,
                                            PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _option_market_view_.secname() << ' ' << _options_info_ << ' ' << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OptionsInfo*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new OptionsInfo(
        t_dbglogger_, r_watch_, concise_indicator_description_, _option_market_view_, _options_info_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OptionsInfo::OptionsInfo(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                         const std::string& concise_indicator_description_,
                         const SecurityMarketView& _option_market_view_, int _options_info_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      option_market_view_(_option_market_view_),
      future_market_view_(*(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
          NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(option_market_view_.shortcode())))),
      interest_rate_(NSESecurityDefinitions::GetInterestRate(watch_.YYYYMMDD())),
      strike_price_(NSESecurityDefinitions::GetStrikePriceFromShortCode(option_market_view_.shortcode())),
      option_type_(((OptionType_t)NSESecurityDefinitions::GetOptionType(option_market_view_.shortcode()))),
      days_to_expiry_(0.0),
      opt_price_(0.0),
      future_price_(0.0),
      bs_implied_vol_(0.0),
      option_interrupted_(false),
      future_interrupted_(false),
      fifteen_min_counter_(0),
      fifteen_min_in_days_(15.0 / 1440.0),
      option_info_(_options_info_),
      price_type_(_price_type_) {
  if (option_info_ == 1) {
    indicator_value_ = interest_rate_;
  } else if (option_info_ == 2) {
    indicator_value_ = strike_price_;
  } else if (option_info_ == 3) {
    indicator_value_ = option_type_;
  } else if (option_info_ == 4) {
    watch_.subscribe_FifteenSecondPeriod(this);
    days_to_expiry_ =
        difftime(
            DateTime::GetTimeUTC(NSESecurityDefinitions::GetExpiryFromShortCode(option_market_view_.shortcode()), 1000),
            DateTime::GetTimeMidnightUTC(watch_.YYYYMMDD())) /
        (3600 * 24);
    days_to_expiry_ -= (double)trading_start_mfm_ / (double)(3600 * 24 * 1000);
    indicator_value_ = days_to_expiry_ / (365.0);
  } else if (option_info_ == 5) {
    if (!option_market_view_.subscribe_price_type(this, _price_type_)) {
      PriceType_t t_error_price_type_ = _price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }
  } else if (option_info_ == 6) {
    if (!future_market_view_.subscribe_price_type(this, _price_type_)) {
      PriceType_t t_error_price_type_ = _price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }
  } else if (option_info_ == 7) {
    days_to_expiry_ =
        difftime(
            DateTime::GetTimeUTC(NSESecurityDefinitions::GetExpiryFromShortCode(option_market_view_.shortcode()), 1000),
            DateTime::GetTimeMidnightUTC(watch_.YYYYMMDD())) /
        (3600 * 24);
    if (!option_market_view_.subscribe_price_type(this, _price_type_)) {
      PriceType_t t_error_price_type_ = _price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }
    if (!future_market_view_.subscribe_price_type(this, _price_type_)) {
      PriceType_t t_error_price_type_ = _price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }
    option_ = OptionObject::GetUniqueInstance(t_dbglogger_, watch_, option_market_view_.shortcode());
  } else {
  }
  watch_.subscribe_FifteenSecondPeriod(this);
}

void OptionsInfo::WhyNotReady() {
  if (!is_ready_) {
    if (!(future_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << future_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(option_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << option_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void OptionsInfo::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if ((future_market_view_.is_ready_complex(2)) && (option_market_view_.is_ready_complex(2))) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (_security_id_ == option_market_view_.security_id()) {
      opt_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
    } else {
      future_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
    }
    if (option_info_ == 5) {
      indicator_value_ = opt_price_;
      NotifyIndicatorListeners(indicator_value_);
    } else if (option_info_ == 6) {
      indicator_value_ = future_price_;
      NotifyIndicatorListeners(indicator_value_);
    } else if (option_info_ == 7) {
      bs_implied_vol_ = option_->MktImpliedVol(future_price_, opt_price_);
      indicator_value_ = bs_implied_vol_;
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}
void OptionsInfo::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (!is_ready_) {
    if ((future_market_view_.is_ready_complex(2)) && (option_market_view_.is_ready_complex(2))) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    fifteen_min_counter_++;
    if (fifteen_min_counter_ % 60 == 0) {
      days_to_expiry_ = days_to_expiry_ - fifteen_min_in_days_;
    }

    if (option_info_ < 4) {
      //      NotifyIndicatorListeners(indicator_value_);
    } else if (option_info_ == 4) {
      indicator_value_ = days_to_expiry_ / (365.0);
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void OptionsInfo::InitializeValues() {
  days_to_expiry_ = difftime(DateTime::GetTimeUTC(
                                 NSESecurityDefinitions::GetExpiryFromShortCode(option_market_view_.shortcode()), 1000),
                             DateTime::GetTimeMidnightUTC(watch_.YYYYMMDD())) /
                    (3600 * 24);
  days_to_expiry_ -= (double)watch_.msecs_from_midnight() / (double)(3600 * 24 * 1000);

  if (option_info_ == 1) {
    indicator_value_ = interest_rate_;
    NotifyIndicatorListeners(indicator_value_);
  } else if (option_info_ == 2) {
    indicator_value_ = strike_price_;
    NotifyIndicatorListeners(indicator_value_);
  } else if (option_info_ == 3) {
    indicator_value_ = option_type_;
    NotifyIndicatorListeners(indicator_value_);
  } else if (option_info_ == 4) {
    indicator_value_ = days_to_expiry_ / (365.0);
    NotifyIndicatorListeners(indicator_value_);
  } else {
    indicator_value_ = 0;
  }
}

// market_interrupt_listener interface

void OptionsInfo::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (option_market_view_.security_id() == _security_id_) {
    option_interrupted_ = true;
  } else if (future_market_view_.security_id() == _security_id_) {
    future_interrupted_ = true;
  }
  if (option_interrupted_ || future_interrupted_) {
    data_interrupted_ = true;
    InitializeValues();
    NotifyIndicatorListeners(indicator_value_);
  }
}

void OptionsInfo::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    if (option_market_view_.security_id() == _security_id_) {
      option_interrupted_ = false;
    } else if (future_market_view_.security_id() == _security_id_) {
      future_interrupted_ = false;
    }
    if (!(future_interrupted_ || option_interrupted_)) {
      InitializeValues();
      data_interrupted_ = false;
    }
  }
}
}
