/**
    \file IndicatorsCode/offline_computed_returns_pairs_port_port.cpp

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
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/offline_computed_returns_pairs_port.hpp"

namespace HFSAT {

void OfflineComputedReturnsPairsPort::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                        std::vector<std::string>& _ors_source_needed_vec_,
                                                        const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[4], _shortcodes_affecting_this_indicator_);
}

OfflineComputedReturnsPairsPort* OfflineComputedReturnsPairsPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ t_dep_market_view_ t_indep_market_view_ t_fractional_seconds_
  // t_price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           (std::string)(r_tokens_[4]), atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

OfflineComputedReturnsPairsPort* OfflineComputedReturnsPairsPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& t_dep_market_view_,
    std::string t_portfolio_descriptor_shortcode_, double t_fractional_seconds_, PriceType_t t_price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_dep_market_view_.secname() << ' ' << t_portfolio_descriptor_shortcode_ << ' '
              << t_fractional_seconds_ << ' ' << t_price_type_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OfflineComputedReturnsPairsPort*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OfflineComputedReturnsPairsPort(t_dbglogger_, r_watch_, concise_indicator_description_, t_dep_market_view_,
                                            t_portfolio_descriptor_shortcode_, t_fractional_seconds_, t_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OfflineComputedReturnsPairsPort::OfflineComputedReturnsPairsPort(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                 const std::string& concise_indicator_description_,
                                                                 SecurityMarketView& t_dep_market_view_,
                                                                 std::string t_portfolio_descriptor_shortcode_,
                                                                 double t_fractional_seconds_,
                                                                 PriceType_t t_price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      indep_portfolio_price_(
          *(PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, t_portfolio_descriptor_shortcode_, t_price_type_))),
      dep_price_trend_(0),
      indep_price_trend_(0),
      rlrdb_(OfflineReturnsRetLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode())),
      last_lrinfo_updated_msecs_(0),
      current_lrinfo_(0.0, 0.0),
      current_projection_multiplier_(0.0),
      current_projected_trend_(0),
      p_dep_indicator_(NULL),
      p_indep_indicator_(NULL),
      price_type_(t_price_type_) {
  watch_.subscribe_BigTimePeriod(this);  // for UpdateLRInfo and updating volume adjustment

  bool rlrdb_absent_ = false;

  if (!(rlrdb_.LRCoeffPresent(t_dep_market_view_.shortcode(), indep_portfolio_price_.shortcode()))) {
    rlrdb_absent_ = true;
  }

  if (rlrdb_absent_) {
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    p_dep_indicator_ = SimpleReturns::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_,
                                                        t_fractional_seconds_, t_price_type_);
    if (p_dep_indicator_ == NULL) {
      indicator_value_ = 0;
      is_ready_ = true;
      return;
    }

    p_indep_indicator_ = SimpleReturnsPort::GetUniqueInstance(t_dbglogger_, r_watch_, t_portfolio_descriptor_shortcode_,
                                                              t_fractional_seconds_, t_price_type_);
    if (p_indep_indicator_ == NULL) {
      indicator_value_ = 0;
      is_ready_ = true;
      return;
    }

    p_dep_indicator_->add_unweighted_indicator_listener(1u, this);
    p_indep_indicator_->add_unweighted_indicator_listener(2u, this);
  }
}

void OfflineComputedReturnsPairsPort::OnIndicatorUpdate(const unsigned int& t_indicator_index_,
                                                        const double& t_new_indicator_value_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_portfolio_price_.is_ready()) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (!data_interrupted_) {
      switch (t_indicator_index_) {
        case 1u: {
          dep_price_trend_ = t_new_indicator_value_;
        } break;
        case 2u: {
          indep_price_trend_ = t_new_indicator_value_;
          current_projected_trend_ = indep_price_trend_ * current_projection_multiplier_;
        } break;
      }

      indicator_value_ = (current_projected_trend_ - dep_price_trend_) * dep_market_view_.price_from_type(price_type_);
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void OfflineComputedReturnsPairsPort::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(indep_portfolio_price_.is_ready())) {
      DBGLOG_TIME_CLASS << indep_portfolio_price_.shortcode() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

// market_interrupt_listener interface
void OfflineComputedReturnsPairsPort::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                              const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void OfflineComputedReturnsPairsPort::OnMarketDataResumed(const unsigned int _security_id_) {
  data_interrupted_ = false;
  InitializeValues();
}

void OfflineComputedReturnsPairsPort::InitializeValues() {
  indicator_value_ = 0;
  UpdateLRInfo();
}

void OfflineComputedReturnsPairsPort::UpdateLRInfo() {
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
}
