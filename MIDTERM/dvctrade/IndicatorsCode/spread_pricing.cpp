/**
    \file IndicatorsCode/spread_pricing.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/spread_pricing.hpp"

namespace HFSAT {

void SpreadPricing::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                      std::vector<std::string>& _ors_source_needed_vec_,
                                      const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

SpreadPricing* SpreadPricing::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                const std::vector<const char*>& r_tokens_,
                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _indep_market_view_ _fractional_seconds_ _price_type_
  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])), StringToPriceType_t(r_tokens_[5]));
}

SpreadPricing* SpreadPricing::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                SecurityMarketView& _dep_market_view_,
                                                SecurityMarketView& _indep_market_view_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SpreadPricing*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new SpreadPricing(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _indep_market_view_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SpreadPricing::SpreadPricing(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                             const std::string& concise_indicator_description_, SecurityMarketView& _dep_market_view_,
                             SecurityMarketView& _indep_market_view_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      first_leg_market_view_(_dep_market_view_),
      second_leg_market_view_(_indep_market_view_),
      current_leg1_price_(0.0),
      current_leg1_bestbid_price_(0.0),
      current_leg1_bestbid_size_(0),
      current_leg1_bestask_price_(0.0),
      current_leg1_bestask_size_(0),
      current_leg2_price_(0.0),
      current_leg2_bestbid_price_(0.0),
      current_leg2_bestbid_size_(0),
      current_leg2_bestask_price_(0.0),
      current_leg2_bestask_size_(0),
      spread_bestbid_price_(0.0),
      spread_max_bid_size_(0),
      spread_bestask_price_(0.0),
      spread_max_ask_size_(0),
      size_factor_(0.0),
      size_threshold_(0.0),
      price_threshold_(0.0),
      spread_factor_(0.0),
      max_size_(0),
      price_type_(_price_type_),
      retail_offer_listeners_(),
      last_retail_offer_(),
      last_retail_update_msecs_(),
      leg1_position_(0),
      leg1_max_position_(0),
      leg2_position_(0),
      leg2_max_position_(0),
      spread_secname_(first_leg_market_view_.shortcode() + "_" + second_leg_market_view_.shortcode()) {
  if (first_leg_market_view_.security_id() ==
      second_leg_market_view_.security_id()) {  // added this since for convenience one could add a combo or portfolio
                                                // as source with a security
    // that is also the dependant
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    SetTimeDecayWeights();

    if (!first_leg_market_view_.subscribe_price_type(this, price_type_)) {
      PriceType_t t_error_price_type_ = price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << " to DEP " << std::endl;
    }
    if (!second_leg_market_view_.subscribe_price_type(this, price_type_)) {
      PriceType_t t_error_price_type_ = price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }

#if EQUITY_INDICATORS_ALWAYS_READY
    if (IndicatorUtil::IsEquityShortcode(first_leg_market_view_.shortcode()) &&
        IndicatorUtil::IsEquityShortcode(second_leg_market_view_.shortcode())) {
      is_ready_ = true;
      InitializeValues();
    }
#endif
  }
}

void SpreadPricing::WhyNotReady() {
  if (!is_ready_) {
    if (!(first_leg_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << first_leg_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(second_leg_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << second_leg_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void SpreadPricing::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& cr_market_update_info_) {
  HFSAT::CDef::RetailOffer this_retail_offer_;
  this_retail_offer_.retail_update_type_ = HFSAT::CDef::knormal;
  this_retail_offer_.offered_bid_price_ = MIN_INVALID_PRICE;
  this_retail_offer_.offered_ask_price_ = MAX_INVALID_PRICE;
  this_retail_offer_.offered_bid_size_ = 0;
  this_retail_offer_.offered_ask_size_ = 0;

  if (first_leg_market_view_.IsBidBookEmpty() || first_leg_market_view_.IsAskBookEmpty() ||
      second_leg_market_view_.IsBidBookEmpty() || second_leg_market_view_.IsAskBookEmpty()) {
    return;
  }

  if (!is_ready_) {
    if (first_leg_market_view_.is_ready_complex(2) && second_leg_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
    }
  } else {
    if (_security_id_ == first_leg_market_view_.security_id()) {
      current_leg1_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
      current_leg1_bestbid_price_ = cr_market_update_info_.bestbid_price_;
      current_leg1_bestbid_size_ = cr_market_update_info_.bestbid_size_;
      current_leg1_bestask_price_ = cr_market_update_info_.bestask_price_;
      current_leg1_bestask_size_ = cr_market_update_info_.bestask_size_;
    }
    if (_security_id_ == second_leg_market_view_.security_id()) {
      current_leg2_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
      current_leg2_bestbid_price_ = cr_market_update_info_.bestbid_price_;
      current_leg2_bestbid_size_ = cr_market_update_info_.bestbid_size_;
      current_leg2_bestask_price_ = cr_market_update_info_.bestask_price_;
      current_leg2_bestask_size_ = cr_market_update_info_.bestask_size_;
    }

    RefreshSizeFactor();
    current_leg2_bestbid_size_ =
        second_leg_market_view_.min_order_size() *
        ((int)(current_leg2_bestbid_size_ * size_factor_ / second_leg_market_view_.min_order_size()));
    current_leg2_bestask_size_ =
        second_leg_market_view_.min_order_size() *
        ((int)(current_leg2_bestask_size_ * size_factor_ / second_leg_market_view_.min_order_size()));

    spread_bestbid_price_ = current_leg1_bestbid_price_ - current_leg2_bestask_price_;
    spread_bestask_price_ = current_leg1_bestask_price_ - current_leg2_bestbid_price_;
    spread_max_bid_size_ = std::min(current_leg1_bestbid_size_, current_leg2_bestask_size_);
    spread_max_ask_size_ = std::min(current_leg1_bestask_size_, current_leg2_bestbid_size_);

    double target_price_ = current_leg1_price_ - current_leg2_price_;

    if (target_price_ - spread_bestbid_price_ > size_threshold_) {
      spread_max_bid_size_ = std::min(max_size_, spread_max_bid_size_);
    }
    if (spread_bestask_price_ - target_price_ > size_threshold_) {
      spread_max_ask_size_ = std::min(max_size_, spread_max_ask_size_);
    }

    int target_bid_ticks_diff_ = (int)((target_price_ - spread_bestbid_price_ - price_threshold_) /
                                       first_leg_market_view_.min_price_increment());
    int target_ask_ticks_diff_ = (int)((spread_bestask_price_ - target_price_ - price_threshold_) /
                                       first_leg_market_view_.min_price_increment());

    spread_bestbid_price_ =
        spread_bestbid_price_ + target_bid_ticks_diff_ * first_leg_market_view_.min_price_increment();
    spread_bestask_price_ =
        spread_bestask_price_ + target_ask_ticks_diff_ * first_leg_market_view_.min_price_increment();

    int leg1_bid_allowance_ = std::max(0, leg1_max_position_ - leg1_position_);
    int leg1_ask_allowance_ = std::max(0, leg1_position_ + leg1_max_position_);
    int leg2_bid_allowance_ =
        second_leg_market_view_.min_order_size() * ((int)(std::max(0, leg2_max_position_ - leg2_position_) *
                                                          size_factor_ / second_leg_market_view_.min_order_size()));
    int leg2_ask_allowance_ =
        second_leg_market_view_.min_order_size() * ((int)(std::max(0, leg2_max_position_ + leg2_position_) *
                                                          size_factor_ / second_leg_market_view_.min_order_size()));

    int bid_allowance_ = std::min(leg1_ask_allowance_, leg2_bid_allowance_);
    int ask_allowance_ = std::min(leg1_bid_allowance_, leg2_ask_allowance_);

    spread_max_bid_size_ = std::min(spread_max_bid_size_, bid_allowance_);
    spread_max_ask_size_ = std::min(spread_max_ask_size_, ask_allowance_);

    this_retail_offer_.offered_bid_price_ = spread_bestbid_price_;
    this_retail_offer_.offered_ask_price_ = spread_bestask_price_;
    this_retail_offer_.offered_bid_size_ = spread_max_bid_size_;
    this_retail_offer_.offered_ask_size_ = spread_max_ask_size_;

    if (this_retail_offer_.offered_bid_size_ <= 0) {
      this_retail_offer_.offered_bid_price_ = MIN_INVALID_PRICE;
    }

    if (this_retail_offer_.offered_ask_size_ <= 0) {
      this_retail_offer_.offered_ask_price_ = MAX_INVALID_PRICE;
    }
  }
  if (data_interrupted_) indicator_value_ = 0;

  NotifyIndicatorListeners(indicator_value_);
}

void SpreadPricing::RefreshSizeFactor() {
  double leg1_dv01_ =
      HFSAT::CurveUtils::stirs_fut_dv01(first_leg_market_view_.shortcode(), watch_.YYYYMMDD(), current_leg1_price_);
  double leg2_dv01_ =
      HFSAT::CurveUtils::stirs_fut_dv01(second_leg_market_view_.shortcode(), watch_.YYYYMMDD(), current_leg2_price_);
  size_factor_ = leg2_dv01_ / leg1_dv01_;
}

void SpreadPricing::InitializeValues() { indicator_value_ = 0; }

void SpreadPricing::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void SpreadPricing::OnMarketDataResumed(const unsigned int _security_id_) { data_interrupted_ = false; }
}
