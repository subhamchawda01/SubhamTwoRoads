/**
    \file IndicatorsCode/future_to_spot_pricing.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/future_to_spot_pricing.hpp"

namespace HFSAT {

void FutureToSpotPricing::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                            std::vector<std::string>& _ors_source_needed_vec_,
                                            const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

FutureToSpotPricing* FutureToSpotPricing::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                            const std::vector<const char*>& r_tokens_,
                                                            PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _indep_market_view_ _fractional_seconds_ _price_type_
  PriceType_t t_price_type_ = kPriceTypeMax;
  if (r_tokens_.size() < 7) {
    if (r_tokens_.size() < 6) {
      ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                  "   FutureToSoptPriceing Incorrect Syntax. Correct syntax would b  INDICATOR _this_weight_ "
                  "_indicator_string_ _dep_market_view_ _indep_market_view_ _fractional_seconds_ _price_type_ ");
    } else {
      t_dbglogger_ << " FutureToSoptPriceing Incorrect Syntax. Correct syntax would b  INDICATOR _this_weight_ "
                      "_indicator_string_ _dep_market_view_ _indep_market_view_ _fractional_seconds_ _price_type_ "
                   << "\n";
      t_dbglogger_.CheckToFlushBuffer();
      t_price_type_ = _basepx_pxtype_;
    }
  } else {
    if (std::string(r_tokens_[6]).compare("#") == 0) {
      t_price_type_ = _basepx_pxtype_;
    } else {
      t_price_type_ = StringToPriceType_t(r_tokens_[6]);
    }
  }

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           StringToPriceType_t(r_tokens_[5]), t_price_type_);
}

FutureToSpotPricing* FutureToSpotPricing::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                            SecurityMarketView& _dep_market_view_,
                                                            SecurityMarketView& _indep_market_view_,
                                                            PriceType_t _price_type_, PriceType_t _dep_price_type_) {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << PriceType_t_To_String(_price_type_) << ' ' << PriceType_t_To_String(_dep_price_type_);

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, FutureToSpotPricing*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new FutureToSpotPricing(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                _indep_market_view_, _price_type_, _dep_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

FutureToSpotPricing::FutureToSpotPricing(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                         const std::string& concise_indicator_description_,
                                         SecurityMarketView& _dep_market_view_, SecurityMarketView& _indep_market_view_,
                                         PriceType_t _price_type_, PriceType_t _dep_price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_),
      dep_price_type_(_dep_price_type_),
      tradingdate_(r_watch_.YYYYMMDD()),
      current_dep_price_(0),
      current_indep_price_(0),
      dep_interrupted_(false),
      indep_interrupted_(false) {
  if (dep_market_view_.security_id() == indep_market_view_.security_id()) {  // added this since for convenience one
                                                                             // could add a combo or portfolio as source
                                                                             // with a security
    // that is also the dependant
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    if (!dep_market_view_.subscribe_price_type(this, _dep_price_type_)) {
      PriceType_t t_error_price_type_ = price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << " to DEP " << std::endl;
    }
    if (!indep_market_view_.subscribe_price_type(this, price_type_)) {
      PriceType_t t_error_price_type_ = price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }

#if EQUITY_INDICATORS_ALWAYS_READY
    if (IndicatorUtil::IsEquityShortcode(dep_market_view_.shortcode()) &&
        IndicatorUtil::IsEquityShortcode(indep_market_view_.shortcode())) {
      is_ready_ = true;
      InitializeValues();
    }
#endif
    std::string offline_info_filename_ =
        "/spare/local/tradeinfo/SpotFutureIndicatorInfo/spot_future_indicator_offline_info.txt";
    std::ifstream offline_info_file_;
    offline_info_file_.open(offline_info_filename_.c_str(), std::ifstream::in);
    days_to_expiry_ = SpotFutureIndicatorUtils::GetDaysToExpiry(tradingdate_, indep_market_view_.shortcode());
    if (offline_info_file_.is_open()) {
      const int kOfflineInfoFileLineBufferLen = 1024;
      char readline_buffer_[kOfflineInfoFileLineBufferLen];
      bzero(readline_buffer_, kOfflineInfoFileLineBufferLen);

      while (offline_info_file_.good()) {
        bzero(readline_buffer_, kOfflineInfoFileLineBufferLen);
        offline_info_file_.getline(readline_buffer_, kOfflineInfoFileLineBufferLen);
        PerishableStringTokenizer st_(readline_buffer_, kOfflineInfoFileLineBufferLen);
        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (tokens_.size() >= 4) {
          char text_date_[9];
          memcpy(text_date_, tokens_[0], 9);
          text_date_[8] = '\0';
          if (atoi(text_date_) == tradingdate_) {
            if ((dep_market_view_.shortcode().compare(std::string(tokens_[1])) == 0) &&
                (indep_market_view_.shortcode().compare(std::string(tokens_[2])) == 0)) {
              diff_daily_interest_rates_ = atof(tokens_[3]);
            }
          } else if ((std::string(text_date_)).compare("DEFAULT") == 0) {
            if ((dep_market_view_.shortcode().compare(std::string(tokens_[1])) == 0) &&
                (indep_market_view_.shortcode().compare(std::string(tokens_[2])) == 0)) {
              diff_daily_interest_rates_ = atof(tokens_[3]);
            }
          }
        }
      }
    }
  }
}

void FutureToSpotPricing::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void FutureToSpotPricing::OnMarketUpdate(const unsigned int _security_id_,
                                         const MarketUpdateInfo& cr_market_update_info_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    current_dep_price_ = SecurityMarketView::GetPriceFromType(dep_price_type_, cr_market_update_info_);
  } else {
    current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
  }

// the fllowing has been added since above we could be potentially be accessing the price when the
// SMV is not ready
#if EQUITY_INDICATORS_ALWAYS_READY
  if ((!IndicatorUtil::IsEquityShortcode(dep_market_view_.shortcode()) &&
       (dep_market_view_.IsBidBookEmpty() || dep_market_view_.IsAskBookEmpty())) ||
      (!IndicatorUtil::IsEquityShortcode(indep_market_view_.shortcode()) &&
       (indep_market_view_.IsBidBookEmpty() || indep_market_view_.IsAskBookEmpty())))
#else
  if (dep_market_view_.IsBidBookEmpty() || dep_market_view_.IsAskBookEmpty() || indep_market_view_.IsBidBookEmpty() ||
      indep_market_view_.IsAskBookEmpty())
#endif
  {
    return;
  }

  if (!is_ready_) {
#if EQUITY_INDICATORS_ALWAYS_READY
    if ((IndicatorUtil::IsEquityShortcode(dep_market_view_.shortcode()) || dep_market_view_.is_ready_complex(2)) &&
        (IndicatorUtil::IsEquityShortcode(indep_market_view_.shortcode()) || indep_market_view_.is_ready_complex(2)))
#else
    if (dep_market_view_.is_ready_complex(2) && indep_market_view_.is_ready_complex(2))
#endif
    {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    indicator_value_ =
        current_indep_price_ * (1 + diff_daily_interest_rates_ * days_to_expiry_) - current_dep_price_ * 1000;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void FutureToSpotPricing::InitializeValues() { indicator_value_ = 0; }

void FutureToSpotPricing::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                  const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    indep_interrupted_ = true;
  } else if (dep_market_view_.security_id() == _security_id_) {
    dep_interrupted_ = true;
  }
  if (indep_interrupted_ || dep_interrupted_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void FutureToSpotPricing::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    if (indep_market_view_.security_id() == _security_id_) {
      indep_interrupted_ = false;
    } else if (dep_market_view_.security_id() == _security_id_) {
      dep_interrupted_ = false;
    }
    if (!(dep_interrupted_ || indep_interrupted_)) {
      InitializeValues();
      data_interrupted_ = false;
    }
  }
}
}
