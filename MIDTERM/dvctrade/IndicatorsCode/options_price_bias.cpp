/**
    \file IndicatorsCode/options_price_bias.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include "dvctrade/Indicators/options_price_bias.hpp"

namespace HFSAT {

void OptionsPriceBias::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                     std::vector<std::string>& _ors_source_needed_vec_,
                                     const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_,
                               NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(r_tokens_[3]));
}

OptionsPriceBias* OptionsPriceBias::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                              const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _options_shc_ _moving_average_window_ _price_type_
  if(r_tokens_.size() < 7 ) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
		"INDICATOR weight OptionsPriceBias _dep_market_view_ _fractional_seconds_ _model_type_ _price_type_ ");
  }

  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(
      NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(r_tokens_[3]));
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), atoi(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

OptionsPriceBias* OptionsPriceBias::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                              SecurityMarketView& _option_market_view_,
                                              double _moving_average_window_, int t_model_type_,
					      PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _option_market_view_.secname() << ' ' << _moving_average_window_
	      << ' ' << t_model_type_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OptionsPriceBias*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OptionsPriceBias(t_dbglogger_, r_watch_, concise_indicator_description_, _option_market_view_,
                         _moving_average_window_, t_model_type_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OptionsPriceBias::OptionsPriceBias(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                           const std::string& concise_indicator_description_,
                           SecurityMarketView& _option_market_view_, double _moving_average_window_,
			   int t_model_type_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      option_market_view_(_option_market_view_),  // subscribe mid_price
      future_market_view_(*(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
          NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(option_market_view_.shortcode())))),
      opt_price_(0.0),
      future_price_(0.0),
      price_type_(_price_type_) {  // subscribe mid_price

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

  if (t_model_type_ == 1 ) {
    implied_vol_ = MovingAverageImpliedVol::GetUniqueInstance(t_dbglogger_, r_watch_, option_market_view_,
							      _moving_average_window_, _price_type_);
  } else {
    implied_vol_ = VWAPImpliedVol::GetUniqueInstance(t_dbglogger_, r_watch_, option_market_view_,
						     _moving_average_window_, _price_type_);
  }

  implied_vol_->add_unweighted_indicator_listener(1u, this);
  implied_vol_set_ = false;
  model_iv_ = 0.0;
  option_interrupted_ = false;
  future_interrupted_ = false;
}

void OptionsPriceBias::WhyNotReady() {
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

void OptionsPriceBias::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (_security_id_ == option_market_view_.security_id()) {
    opt_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  } else {
    future_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  }
  if (!is_ready_) {
    if ((future_market_view_.is_ready_complex(2)) && (option_market_view_.is_ready_complex(2)) && implied_vol_set_) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    model_price_ = option_->ModelImpliedPrice(future_price_, model_iv_);
    indicator_value_ = model_price_ - opt_price_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void OptionsPriceBias::OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_) {
  if (!implied_vol_set_) {
    implied_vol_set_ = true;
  }
  model_iv_ = new_value_;
}

void OptionsPriceBias::InitializeValues() { indicator_value_ = 0; }

// market_interrupt_listener interface

void OptionsPriceBias::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
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

void OptionsPriceBias::OnMarketDataResumed(const unsigned int _security_id_) {
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
