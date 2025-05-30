/**
    \file IndicatorsCode/offline_computed_returns_pairs_mktevents.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include <fstream>
#include <strings.h>
#include <stdlib.h>
#include <sstream>

#include "dvccode/CDef/math_utils.hpp"

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

#include "dvctrade/Indicators/simple_returns_mktevents.hpp"
#include "dvctrade/Indicators/offline_computed_returns_pairs_mktevents.hpp"

namespace HFSAT {

void OfflineComputedReturnsPairsMktEvents::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

OfflineComputedReturnsPairsMktEvents* OfflineComputedReturnsPairsMktEvents::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_shortcode_ _indep_market_view_ _num_events_halflife_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);

  if (r_tokens_.size() < 7) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight OfflineComputedReturnsPairsMktEvents _dep_market_view_ _indep_market_view_ "
                "_num_events_/_fractional_seconds_ <T> _price_type_ ");
  } else if (r_tokens_.size() == 7) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             (unsigned int)std::max(1, atoi(r_tokens_[5])), StringToPriceType_t(r_tokens_[6]));
  }

  if (std::string(r_tokens_[7]).compare("#") == 0) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             (unsigned int)std::max(1, atoi(r_tokens_[5])), StringToPriceType_t(r_tokens_[6]));
  }

  if (std::string(r_tokens_[6]).compare("T") == 0) {
    double avg_l1_events_per_sec_ = HFSAT::SampleDataUtil::GetAvgForPeriod(
        std::string(r_tokens_[3]), r_watch_.YYYYMMDD(), 60, std::string("L1EVPerSec"));
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             (unsigned int)std::max(1.00, avg_l1_events_per_sec_ * atof(r_tokens_[5])),
                             StringToPriceType_t(r_tokens_[7]), true);
  } else {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             (unsigned int)std::max(1, atoi(r_tokens_[5])), StringToPriceType_t(r_tokens_[6]));
  }
}

OfflineComputedReturnsPairsMktEvents* OfflineComputedReturnsPairsMktEvents::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& _dep_market_view_,
    SecurityMarketView& _indep_market_view_, unsigned int _num_events_halflife_, PriceType_t _price_type_,
    bool _use_time_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << _num_events_halflife_ << ' ' << PriceType_t_To_String(_price_type_) << ' ' << _use_time_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OfflineComputedReturnsPairsMktEvents*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new OfflineComputedReturnsPairsMktEvents(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _indep_market_view_,
        _num_events_halflife_, _price_type_, _use_time_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OfflineComputedReturnsPairsMktEvents::OfflineComputedReturnsPairsMktEvents(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    SecurityMarketView& t_dep_market_view_, SecurityMarketView& t_indep_market_view_,
    unsigned int _num_events_halflife_, PriceType_t _price_type_, bool _use_time_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      indep_market_view_(t_indep_market_view_),
      price_type_(_price_type_),
      dep_price_returns_(0.0),
      indep_price_returns_(0.0),
      rlrdb_(OfflineReturnsRetLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode())),
      last_lrinfo_updated_msecs_(0),
      current_lrinfo_(0.0, 0.0),
      current_projection_multiplier_(0.0),
      current_projected_returns_(0),
      p_dep_indicator_(NULL),
      p_indep_indicator_(NULL) {
  watch_.subscribe_BigTimePeriod(this);  // for UpdateLRInfo and updating volume adjustment

  bool lrdb_absent_ = false;

  if (t_dep_market_view_.security_id() == t_indep_market_view_.security_id()) {  // added this since for convenience one
                                                                                 // could add a combo or portfolio as
                                                                                 // source with a security
    // that is also the dependant
    lrdb_absent_ = true;
  }

  if (!(rlrdb_.LRCoeffPresent(t_dep_market_view_.shortcode(), t_indep_market_view_.shortcode()))) {
    lrdb_absent_ = true;
  }

  if (lrdb_absent_) {
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    p_dep_indicator_ = SimpleReturnsMktEvents::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_,
                                                                 _num_events_halflife_, _price_type_);
    p_dep_indicator_->add_unweighted_indicator_listener(1u, this);

    if (_use_time_) {
      double dep_avg_l1_events_per_sec_ = HFSAT::SampleDataUtil::GetAvgForPeriod(
          t_dep_market_view_.shortcode(), watch_.YYYYMMDD(), 60, std::string("L1EVPerSec"));
      double indep_avg_l1_events_per_sec_ = HFSAT::SampleDataUtil::GetAvgForPeriod(
          t_indep_market_view_.shortcode(), watch_.YYYYMMDD(), 60, std::string("L1EVPerSec"));

      unsigned int indep_num_events_halflife_ = 1;

      if (dep_avg_l1_events_per_sec_ > 0.001) {
        indep_num_events_halflife_ = (unsigned int)std::max(
            1.00, indep_avg_l1_events_per_sec_ * _num_events_halflife_ / dep_avg_l1_events_per_sec_);
      }

      p_indep_indicator_ = SimpleReturnsMktEvents::GetUniqueInstance(t_dbglogger_, r_watch_, t_indep_market_view_,
                                                                     indep_num_events_halflife_, _price_type_);
      p_indep_indicator_->add_unweighted_indicator_listener(2u, this);
    } else {
      p_indep_indicator_ = SimpleReturnsMktEvents::GetUniqueInstance(t_dbglogger_, r_watch_, t_indep_market_view_,
                                                                     _num_events_halflife_, _price_type_);
      p_indep_indicator_->add_unweighted_indicator_listener(2u, this);
    }
  }
}

void OfflineComputedReturnsPairsMktEvents::OnIndicatorUpdate(const unsigned int& t_indicator_index_,
                                                             const double& t_new_indicator_value_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    switch (t_indicator_index_) {
      case 1u: {
        dep_price_returns_ = t_new_indicator_value_;
      } break;
      case 2u: {
        indep_price_returns_ = t_new_indicator_value_;
        current_projected_returns_ = indep_price_returns_ * current_projection_multiplier_;
      } break;
    }

    indicator_value_ =
        (current_projected_returns_ - dep_price_returns_) * dep_market_view_.price_from_type(price_type_);

    //	    if( p_indep_indicator_->IsDataInterrupted())
    // indicator_value_ = 0;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void OfflineComputedReturnsPairsMktEvents::WhyNotReady() {
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

// market_interrupt_listener interface
void OfflineComputedReturnsPairsMktEvents::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                                   const int msecs_since_last_receive_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void OfflineComputedReturnsPairsMktEvents::OnMarketDataResumed(const unsigned int _security_id_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

void OfflineComputedReturnsPairsMktEvents::InitializeValues() {
  indicator_value_ = 0;
  UpdateLRInfo();
}

void OfflineComputedReturnsPairsMktEvents::UpdateLRInfo() {
  if ((last_lrinfo_updated_msecs_ == 0) ||
      (watch_.msecs_from_midnight() - last_lrinfo_updated_msecs_ > TENMINUTESMSECS)) {
    current_lrinfo_ = rlrdb_.GetLRCoeff(dep_market_view_.shortcode(), indep_market_view_.shortcode());
    ComputeMultiplier();

    if (dbglogger_.CheckLoggingLevel(LRDB_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "lrinfo ( " << dep_market_view_.shortcode() << ", " << indep_market_view_.shortcode()
                             << " ) " << current_lrinfo_.lr_coeff_ << ' ' << current_lrinfo_.lr_correlation_ << " -> "
                             << current_projection_multiplier_ << DBGLOG_ENDL_FLUSH;
    }

    last_lrinfo_updated_msecs_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % TENMINUTESMSECS);
  }
}
}
