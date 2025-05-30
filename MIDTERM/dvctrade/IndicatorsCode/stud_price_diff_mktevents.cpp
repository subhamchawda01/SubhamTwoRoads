/**
    \file IndicatorsCode/stud_price_diff_mktevents.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/stud_price_diff_mktevents.hpp"

namespace HFSAT {

void StudPriceDiffMktEvents::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                               std::vector<std::string>& _ors_source_needed_vec_,
                                               const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

StudPriceDiffMktEvents* StudPriceDiffMktEvents::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                  const std::vector<const char*>& r_tokens_,
                                                                  PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _indep_market_view_ _fractional_seconds_
  // _include_lrdb_sign_ _price_type_
  if (r_tokens_.size() < 7) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight StudPriceDiffMktEvents _dep_market_view_ _indep_market_view_ _fractional_seconds_ "
                "_price_type_ ");
  } else if (r_tokens_.size() == 7) {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             atof(r_tokens_[5]), -1, StringToPriceType_t(r_tokens_[6]));
  }
  if (std::string(r_tokens_[7]).compare("#") == 0) {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             atof(r_tokens_[5]), -1, StringToPriceType_t(r_tokens_[6]));
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atof(r_tokens_[5]), atof(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

StudPriceDiffMktEvents* StudPriceDiffMktEvents::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                  const SecurityMarketView& _dep_market_view_,
                                                                  const SecurityMarketView& _indep_market_view_,
                                                                  double _fractional_seconds_, double _lrdb_sign_,
                                                                  PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << _fractional_seconds_ << ' ' << _lrdb_sign_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, StudPriceDiffMktEvents*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new StudPriceDiffMktEvents(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                   _indep_market_view_, _fractional_seconds_, _lrdb_sign_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

StudPriceDiffMktEvents::StudPriceDiffMktEvents(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                               const std::string& concise_indicator_description_,
                                               const SecurityMarketView& _dep_market_view_,
                                               const SecurityMarketView& _indep_market_view_,
                                               double _fractional_seconds_, double _lrdb_sign_,
                                               PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_),
      dep_decay_page_factor_(0.95),
      dep_inv_decay_sum_(0.05),
      indep_decay_page_factor_(0.95),
      indep_inv_decay_sum_(0.05),
      moving_avg_dep_(0),
      current_dep_price_(0),
      moving_avg_indep_(0),
      current_indep_price_(0),
      stdev_dep_(-1),
      stdev_indep_(-1),
      dep_interrupted_(false),
      indep_interrupted_(false),
      lrdb_sign_(1) {
  double dep_avg_l1_events_per_sec_ = HFSAT::SampleDataUtil::GetAvgForPeriod(
      dep_market_view_.shortcode(), r_watch_.YYYYMMDD(), 60, std::string("L1EVPerSec"));
  double indep_avg_l1_events_per_sec_ = HFSAT::SampleDataUtil::GetAvgForPeriod(
      indep_market_view_.shortcode(), r_watch_.YYYYMMDD(), 60, std::string("L1EVPerSec"));

  unsigned int dep_num_events_halflife_ =
      (unsigned int)std::max(1.00, dep_avg_l1_events_per_sec_ * _fractional_seconds_);
  unsigned int indep_num_events_halflife_ =
      (unsigned int)std::max(1.00, indep_avg_l1_events_per_sec_ * _fractional_seconds_);

  dep_decay_page_factor_ = MathUtils::CalcDecayFactor(dep_num_events_halflife_);
  dep_inv_decay_sum_ = (1 - dep_decay_page_factor_);
  indep_decay_page_factor_ = MathUtils::CalcDecayFactor(indep_num_events_halflife_);
  indep_inv_decay_sum_ = (1 - indep_decay_page_factor_);

  if (!dep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
  if (!indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  SlowStdevCalculator* std_1 =
      SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_.shortcode());
  if (std_1 != NULL) {
    std_1->AddSlowStdevCalculatorListener(this);
  }

  SlowStdevCalculator* std_2 =
      SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_.shortcode());
  if (std_2 != NULL) {
    std_2->AddSlowStdevCalculatorListener(this);
  }
  if (_lrdb_sign_ > 0) {
    if (OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, _dep_market_view_.shortcode())
            .GetLRCoeff(dep_market_view_.shortcode(), indep_market_view_.shortcode())
            .lr_correlation_ < 0) {
      lrdb_sign_ = -1;
    }
  } else {
    lrdb_sign_ = 1;
  }
}

void StudPriceDiffMktEvents::OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    stdev_dep_ = _new_stdev_value_;
  } else {
    stdev_indep_ = _new_stdev_value_;
  }
}

void StudPriceDiffMktEvents::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void StudPriceDiffMktEvents::OnMarketUpdate(const unsigned int _security_id_,
                                            const MarketUpdateInfo& _market_update_info_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  } else {
    current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  }

  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2) && dep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (_security_id_ == dep_market_view_.security_id()) {
      moving_avg_dep_ = (current_dep_price_ * dep_inv_decay_sum_) + (moving_avg_dep_ * dep_decay_page_factor_);
    } else if (_security_id_ == indep_market_view_.security_id()) {
      moving_avg_indep_ =
          (current_indep_price_ * indep_inv_decay_sum_) + (moving_avg_indep_ * indep_decay_page_factor_);
    }

    indicator_value_ = lrdb_sign_ * (current_indep_price_ - moving_avg_indep_) / stdev_indep_ -
                       (current_dep_price_ - moving_avg_dep_) / stdev_dep_;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void StudPriceDiffMktEvents::InitializeValues() {
  moving_avg_dep_ = current_dep_price_;
  moving_avg_indep_ = current_indep_price_;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void StudPriceDiffMktEvents::OnMarketDataInterrupted(const unsigned int _security_id_,
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

void StudPriceDiffMktEvents::OnMarketDataResumed(const unsigned int _security_id_) {
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
