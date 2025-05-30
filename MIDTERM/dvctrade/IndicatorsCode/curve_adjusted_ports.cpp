/**
    \file IndicatorsCode/curve_adjusted_simple_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
//#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/curve_adjusted_ports.hpp"

namespace HFSAT {

void CurveAdjustedPorts::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                           std::vector<std::string>& _ors_source_needed_vec_,
                                           const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() > 7u)  // doesnt mean indicator string syntax is valid .
  {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);  // dep
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);  // indep1
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[5]);  // indep2
  }
}

CurveAdjustedPorts* CurveAdjustedPorts::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          const std::vector<const char*>& r_tokens_,
                                                          PriceType_t _base_price_type_) {
  if (r_tokens_.size() < 8u) {
    ExitVerbose(kExitErrorCodeGeneral, "CurveAdjustedPorts needs 8 atleast tokens");
    return nullptr;
  }

  // INDICATOR 1.00 CurveAdjustedPorts DEP NUM_PORTS INDEP1 INDEP2 .. INDEP(i) HALFLIFE PRICE_TYPE
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  auto num_maturities = atoi(r_tokens_[4]);

  if ((int)r_tokens_.size() < 6 + num_maturities) {
    ExitVerbose(kExitErrorCodeGeneral, (std::string("CuverAdjustedPorts doesn't have enough indeps ") +
                                        std::to_string(num_maturities)).c_str());
    return nullptr;
  }

  for (auto i = 0; i < num_maturities; i++) {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[5 + i]);
  }

  std::vector<std::string> indep_shortcode_vec;
  for (auto i = 0; i < num_maturities; i++) {
    indep_shortcode_vec.push_back(r_tokens_[5 + i]);
  }

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           indep_shortcode_vec, atoi(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

CurveAdjustedPorts* CurveAdjustedPorts::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          const SecurityMarketView* _dep_market_view_,
                                                          const std::vector<std::string> indep_shortcodes_vec,
                                                          double _fractional_secs_, PriceType_t t_price_type_) {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << VarName() << ' ' << _dep_market_view_->secname() << ' ';
  for (auto shortcode : indep_shortcodes_vec) t_temp_oss_ << shortcode << ' ';
  t_temp_oss_ << _fractional_secs_ << ' ' << t_price_type_;

  std::string concise_indicator_description_(t_temp_oss_.str());
  static std::map<std::string, CurveAdjustedPorts*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new CurveAdjustedPorts(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                               indep_shortcodes_vec, _fractional_secs_, t_price_type_);
  }

  return concise_indicator_description_map_[concise_indicator_description_];
}

CurveAdjustedPorts::CurveAdjustedPorts(DebugLogger& t_dbglogger, const Watch& r_watch,
                                       const std::string& t_concise_indicator_description,
                                       const SecurityMarketView* r_dep_market_view,
                                       const std::vector<std::string> indep_shortcode_vec, double t_fractional_secs,
                                       PriceType_t t_price_type)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger, r_watch, t_concise_indicator_description),
      dep_market_view_(r_dep_market_view),
      indep_smv_vec_(),
      dep_trend_(nullptr),
      indep_trend_vec_(),
      dep_term_(10),
      term_vec_(),
      is_ready_vec_(indep_shortcode_vec.size(), false),
      indep_interrupted_vec_(indep_shortcode_vec.size(), false),
      prev_value_vec_(indep_shortcode_vec.size() + 1, 0.0) {
  //

  dep_term_ = CurveUtils::_get_term_(r_watch.YYYYMMDD(), r_dep_market_view->secname());
  std::string t_dep_secname_ = std::string(r_dep_market_view->secname());

  auto listener_count = 0;
  dep_trend_ = SimpleTrend::GetUniqueInstance(t_dbglogger, r_watch, dep_market_view_->shortcode(), t_fractional_secs,
                                              t_price_type);

  dep_trend_->add_indicator_listener(listener_count++, this, -1);

  // Variables for each indep
  for (auto indep_shortcode : indep_shortcode_vec) {
    // Get SMV
    auto* smv = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(indep_shortcode);
    indep_smv_vec_.push_back(smv);

    // simple trend
    auto indep_trend =
        SimpleTrend::GetUniqueInstance(t_dbglogger, r_watch, indep_shortcode, t_fractional_secs, t_price_type);
    indep_trend_vec_.push_back(indep_trend);

    // Compute term for this expiry
    auto indep_term = CurveUtils::_get_term_(r_watch.YYYYMMDD(), smv->secname());

    // listen to it with weight
    indep_trend->add_indicator_listener(listener_count++, this, indep_term);
    dep_interrupted_ = false;
  }
}

void CurveAdjustedPorts::OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_) {
  if (!is_ready_) {
    is_ready_vec_[indicator_index_] = true;
    is_ready_ = AreAllReady();
  } else if (!data_interrupted_) {
    indicator_value_ += (new_value_ - prev_value_vec_[indicator_index_]);
    prev_value_vec_[indicator_index_] = new_value_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

// market_interrupt_listener interface
void CurveAdjustedPorts::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                 const int msecs_since_last_receive_) {
  if (dep_market_view_->security_id() == _security_id_) {
    dep_interrupted_ = true;
    data_interrupted_ = true;
  }

  else {
    // only if its not dep interrupt, look for indep
    for (auto smv : indep_smv_vec_) {
      if (smv->security_id() == _security_id_) {
        data_interrupted_ = true;
      }
    }
  }

  if (data_interrupted_) {
    InitializeValues();
    NotifyIndicatorListeners(indicator_value_);
  }
}

/**
 *
 * @param _security_id_
 */
void CurveAdjustedPorts::OnMarketDataResumed(const unsigned int _security_id_) {
  bool indep_interrupted = false;
  if (dep_market_view_->security_id() == _security_id_) {
    dep_interrupted_ = false;
  } else {
    // only if it's not dep interrupt, look for indep

    for (auto id = 0u; id < indep_smv_vec_.size(); id++) {
      auto smv = indep_smv_vec_[id];
      if (smv->security_id() == _security_id_) {
        indep_interrupted_vec_[id] = false;
      }

      // look at all indeps if they are interrupted
      indep_interrupted = indep_interrupted || indep_interrupted_vec_[id];
    }
  }

  if ((!indep_interrupted) && (!dep_interrupted_)) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

bool CurveAdjustedPorts::AreAllReady() { return VectorUtils::CheckAllForValue(is_ready_vec_, true); }

void CurveAdjustedPorts::InitializeValues() { indicator_value_ = 0; }
}
