/**
    \file IndicatorsCode/offline_computed_returns_cutoff_pairs_mktevents_port.cpp

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

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/offline_computed_returns_cutoff_pairs_mktevents_port.hpp"

namespace HFSAT {

void OfflineComputedReturnsCutoffPairsMktEventsPort::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[4], _shortcodes_affecting_this_indicator_);
}

OfflineComputedReturnsCutoffPairsMktEventsPort* OfflineComputedReturnsCutoffPairsMktEventsPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _portfolio_descriptor_shortcode_
  // _num_events_halflife_/_fractional_seconds_ <T>
  // _price_type_

  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  if (r_tokens_.size() < 7) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight OfflineComputedReturnsCutoffPairsMktEventsPort _dep_market_view_ _indep_market_view_ "
                "_num_events_/_fractional_seconds_ <T> _price_type_ ");
  } else if (r_tokens_.size() == 7) {
    return GetUniqueInstance(
        t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
        (std::string)(r_tokens_[4]), (unsigned int)std::max(1, atoi(r_tokens_[5])), StringToPriceType_t(r_tokens_[6]));
  }
  if (std::string(r_tokens_[7]).compare("#") == 0) {
    return GetUniqueInstance(
        t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
        (std::string)(r_tokens_[4]), (unsigned int)std::max(1, atoi(r_tokens_[5])), StringToPriceType_t(r_tokens_[6]));
  }

  if (std::string(r_tokens_[6]).compare("T") == 0) {
    double avg_l1_events_per_sec_ = HFSAT::SampleDataUtil::GetAvgForPeriod(
        std::string(r_tokens_[3]), r_watch_.YYYYMMDD(), 60, std::string("L1EVPerSec"));

    return GetUniqueInstance(
        t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
        (std::string)(r_tokens_[4]), (unsigned int)std::max(1.00, avg_l1_events_per_sec_ * atof(r_tokens_[5])),
        StringToPriceType_t(r_tokens_[7]), true);
  } else {
    return GetUniqueInstance(
        t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
        (std::string)(r_tokens_[4]), (unsigned int)std::max(1, atoi(r_tokens_[5])), StringToPriceType_t(r_tokens_[6]));
  }
}

OfflineComputedReturnsCutoffPairsMktEventsPort* OfflineComputedReturnsCutoffPairsMktEventsPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& _dep_market_view_,
    std::string _portfolio_descriptor_shortcode_, unsigned int _num_events_halflife_, PriceType_t _price_type_,
    bool _use_time_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _portfolio_descriptor_shortcode_ << ' '
              << _num_events_halflife_ << ' ' << PriceType_t_To_String(_price_type_) << ' ' << _use_time_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OfflineComputedReturnsCutoffPairsMktEventsPort*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OfflineComputedReturnsCutoffPairsMktEventsPort(t_dbglogger_, r_watch_, concise_indicator_description_,
                                                           _dep_market_view_, _portfolio_descriptor_shortcode_,
                                                           _num_events_halflife_, _price_type_, _use_time_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OfflineComputedReturnsCutoffPairsMktEventsPort::OfflineComputedReturnsCutoffPairsMktEventsPort(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    SecurityMarketView& t_dep_market_view_, std::string _portfolio_descriptor_shortcode_,
    unsigned int _num_events_halflife_, PriceType_t _price_type_, bool _use_time_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      indep_portfolio_price_(
          *(PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _price_type_))),
      price_type_(_price_type_),
      dep_price_trend_(0.0),
      indep_price_trend_(0.0),
      p_dep_indicator_(NULL),
      p_indep_indicator_(NULL),
      rlrdb_(OfflineReturnsRetLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode())),
      last_lrinfo_updated_msecs_(0),
      current_lrinfo_(0.0, 0.0),
      current_projection_multiplier_(0.0),
      current_projected_trend_(0) {
  watch_.subscribe_BigTimePeriod(this);  // for UpdateLRInfo and updating volume adjustment

  bool rlrdb_absent_ = false;

  if (!(rlrdb_.LRCoeffPresent(t_dep_market_view_.shortcode(), indep_portfolio_price_.shortcode()))) {
    rlrdb_absent_ = true;
  }

  if (rlrdb_absent_) {
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    p_dep_indicator_ = SimpleReturnsMktEvents::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_,
                                                                 _num_events_halflife_, _price_type_);
    p_dep_indicator_->add_unweighted_indicator_listener(1, this);

    if (_use_time_) {
      unsigned int indep_num_events_halflife_ = 1;
      HFSAT::IndicatorUtil::GetNormalizedL1EventsForPort(_portfolio_descriptor_shortcode_,
                                                         t_dep_market_view_.shortcode(), _num_events_halflife_,
                                                         indep_num_events_halflife_, r_watch_.YYYYMMDD());

      p_indep_indicator_ = SimpleReturnsMktEventsPort::GetUniqueInstance(
          t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, indep_num_events_halflife_, _price_type_);
      p_indep_indicator_->add_unweighted_indicator_listener(2, this);
    } else {
      p_indep_indicator_ = SimpleReturnsMktEventsPort::GetUniqueInstance(
          t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _num_events_halflife_, _price_type_);
      p_indep_indicator_->add_unweighted_indicator_listener(2, this);
    }
  }
}

void OfflineComputedReturnsCutoffPairsMktEventsPort::OnIndicatorUpdate(const unsigned int& _indicator_index_,
                                                                       const double& _new_value_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_portfolio_price_.is_ready()) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (_indicator_index_ == 1) {
      dep_price_trend_ = _new_value_;
    } else if (_indicator_index_ == 2) {
      indep_price_trend_ = _new_value_;
      current_projected_trend_ = indep_price_trend_ * current_projection_multiplier_;
    }

    if (fabs(current_projected_trend_) > fabs(dep_price_trend_)) {
      indicator_value_ = (current_projected_trend_ - dep_price_trend_) * dep_market_view_.price_from_type(price_type_);
    } else {
      indicator_value_ = 0;
    }

    if (data_interrupted_) {
      indicator_value_ = 0;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void OfflineComputedReturnsCutoffPairsMktEventsPort::InitializeValues() {
  indicator_value_ = 0;
  UpdateLRInfo();
}

void OfflineComputedReturnsCutoffPairsMktEventsPort::UpdateLRInfo() {
  if ((last_lrinfo_updated_msecs_ == 0) ||
      (watch_.msecs_from_midnight() - last_lrinfo_updated_msecs_ > TENMINUTESMSECS)) {
    current_lrinfo_ = rlrdb_.GetLRCoeff(dep_market_view_.shortcode(), indep_portfolio_price_.shortcode());
    ComputeMultiplier();

    if (dbglogger_.CheckLoggingLevel(LRDB_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "lrinfo ( " << dep_market_view_.shortcode() << ", "
                             << indep_portfolio_price_.shortcode() << " ) " << current_lrinfo_.lr_coeff_ << ' '
                             << current_lrinfo_.lr_correlation_ << " -> " << current_projection_multiplier_
                             << DBGLOG_ENDL_FLUSH;
    }

    last_lrinfo_updated_msecs_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % TENMINUTESMSECS);
  }
}

void OfflineComputedReturnsCutoffPairsMktEventsPort::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                                             const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void OfflineComputedReturnsCutoffPairsMktEventsPort::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
