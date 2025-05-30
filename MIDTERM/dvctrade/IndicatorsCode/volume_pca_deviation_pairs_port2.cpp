/**
    \file IndicatorsCode/volume_pca_deviation_pairs_port.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/recent_simple_volume_measure.hpp"
#include "dvctrade/Indicators/volume_pca_deviation_pairs_port2.hpp"
#include "dvctrade/Indicators/pca_weights_manager.hpp"
namespace HFSAT {

void VolumePCADeviationPairsPort2::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                     std::vector<std::string>& _ors_source_needed_vec_,
                                                     const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() >= 5u) {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
    IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[4], _shortcodes_affecting_this_indicator_);
  }
}

VolumePCADeviationPairsPort2* VolumePCADeviationPairsPort2::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                              const Watch& r_watch_,
                                                                              const std::vector<const char*>& r_tokens_,
                                                                              PriceType_t _basepx_pxtype_) {
  if (r_tokens_.size() < 8) {
    std::cerr << "VolumePCADeviationPairsPort2 syntax incorrect! given text:";
    for (auto i = 0u; i < r_tokens_.size(); i++) {
      std::cerr << " " << r_tokens_[i];
    }
    std::cerr << std::endl;
    std::cerr << "Expected syntax: INDICATOR  _this_weight_  VolumePCADeviationPairsPort2  _dep_market_view_  "
                 "_portfolio_descriptor_shortcode_  _fractional_seconds_ _brk_trend_fac_ _price_type_"
              << std::endl;

    exit(1);
  }

  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _portfolio_descriptor_shortcode_
  // _fractional_seconds_  _brk_trend_fac_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      (std::string)(r_tokens_[4]), atof(r_tokens_[5]), atof(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

VolumePCADeviationPairsPort2* VolumePCADeviationPairsPort2::GetUniqueInstance(
    DebugLogger& _dbglogger_, const Watch& _watch_, SecurityMarketView& _dep_market_view_,
    const std::string& _portfolio_descriptor_shortcode_, const double _fractional_seconds_,
    const double _brk_trend_fac_, const PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _portfolio_descriptor_shortcode_ << ' '
              << _fractional_seconds_ << ' ' << _brk_trend_fac_ << ' ' << PriceType_t_To_String(_price_type_);

  std::string concise_indicator_description_(t_temp_oss_.str());
  static std::map<std::string, VolumePCADeviationPairsPort2*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    // Get default values from the pcaweightsmanager
    PcaWeightsManager pca_weights_manager = PcaWeightsManager::GetUniqueInstance();
    EigenConstituentsVec principal_eigen_vec =
        pca_weights_manager.GetEigenConstituentVec(_portfolio_descriptor_shortcode_);

    if (principal_eigen_vec.size() < 1) {
      std::cerr << "VolumePCADeviationPairsPort2: No eigen vectors loaded for " << (_portfolio_descriptor_shortcode_)
                << " in PcaWeightsManager " << std::endl;
      exit(1);
    }

    concise_indicator_description_map_[concise_indicator_description_] = new VolumePCADeviationPairsPort2(
        _dbglogger_, _watch_, concise_indicator_description_, _dep_market_view_, _portfolio_descriptor_shortcode_,
        _fractional_seconds_, principal_eigen_vec[0u].eigenvec_components_, _brk_trend_fac_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

VolumePCADeviationPairsPort2::VolumePCADeviationPairsPort2(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                           const std::string& concise_indicator_description_,
                                                           SecurityMarketView& r_dep_market_view_,
                                                           const std::string& _portfolio_descriptor_shortcode_,
                                                           const double _fractional_seconds_,
                                                           const std::vector<double>& t_eigen_components_,
                                                           const double _brk_trend_fac_, const PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(r_dep_market_view_),
      dep_index_in_portfolio_(
          (PcaWeightsManager::GetUniqueInstance())
              .GetPortfolioShortCodeIndexer(_portfolio_descriptor_shortcode_, r_dep_market_view_.shortcode())),
      dep_trend_indicator_index_(t_eigen_components_.size()),
      indep_market_view_vec_(t_eigen_components_.size(), NULL),
      eigen_components_(t_eigen_components_),
      stdev_each_constituent_(
          (PcaWeightsManager::GetUniqueInstance()).GetPortfolioStdevs(_portfolio_descriptor_shortcode_)),
      risk_volume_(t_eigen_components_.size(), 0.0),
      risk_multiplier_(t_eigen_components_.size(), 0.0),
      st_weight_vec_(t_eigen_components_.size(), 0.0),
      trend_vec_(t_eigen_components_.size(), 0.0),
      weighted_eigen_ready_(false),
      is_ready_vec_(t_eigen_components_.size() + 1, false),
      data_interrupted_vec_(t_eigen_components_.size(), false),
      recompute_indicator_value_(false),
      p_indep_indicator_vec_(t_eigen_components_.size(), NULL),
      dep_mean_l1_norm_(LoadMeanL1Norm(r_watch_.YYYYMMDD(), dep_market_view_.shortcode(), NUM_DAYS_HISTORY)),
      brk_trend_val_(_brk_trend_fac_ * dep_mean_l1_norm_),
      dep_trend_val_(0.0) {
  IndicatorUtil::GetPortfolioSMVVec(_portfolio_descriptor_shortcode_, indep_market_view_vec_);

  if (dep_index_in_portfolio_ < 0) {
    DBGLOG_TIME_CLASS_FUNC << " since " << dep_index_in_portfolio_
                           << " not in PortFolio. Dep:" << r_dep_market_view_.shortcode()
                           << " Port: " << _portfolio_descriptor_shortcode_ << " exiting" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    std::cerr << "VolumePCADeviationPairsPort2: since " << dep_index_in_portfolio_
              << " not in PortFolio. Dep:" << r_dep_market_view_.shortcode()
              << " Port: " << _portfolio_descriptor_shortcode_ << " exiting\n";
    exit(1);
  } else {
    for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
      if (indep_market_view_vec_[i]->is_ready()) {
        is_ready_vec_[i] = true;
        InitializeValues(i);
      }

      RecentSimpleVolumeMeasure::GetUniqueInstance(t_dbglogger_, r_watch_, *(indep_market_view_vec_[i]), 900)
          ->AddRecentSimpleVolumeListener(i, this);

      SimpleTrend* _dep_indicator_ = SimpleTrend::GetUniqueInstance(
          t_dbglogger_, r_watch_, indep_market_view_vec_[i]->shortcode(), _fractional_seconds_, _price_type_);
      _dep_indicator_->add_unweighted_indicator_listener(i, this);
      p_indep_indicator_vec_[i] = _dep_indicator_;
    }

    if (_fractional_seconds_ != 900) {
      SimpleTrend* _dep_indicator_ =
          SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_.shortcode(), 900, _price_type_);
      _dep_indicator_->add_unweighted_indicator_listener(dep_trend_indicator_index_, this);
      is_ready_vec_[dep_trend_indicator_index_] = true;
    } else {
      dep_trend_indicator_index_ = dep_index_in_portfolio_;
      is_ready_vec_.pop_back();
    }

    is_ready_ = AreAllReady();
  }
}

void VolumePCADeviationPairsPort2::WhyNotReady() {
  if (!is_ready_) {
    if (!dep_market_view_.is_ready_complex(2)) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    // TODO
  }
}

void VolumePCADeviationPairsPort2::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (_indicator_index_ < trend_vec_.size()) {
    // doing this at every update because we don't want to have stale trends
    indicator_value_ += st_weight_vec_[_indicator_index_] * (_new_value_ - trend_vec_[_indicator_index_]);
    trend_vec_[_indicator_index_] = _new_value_;
  }
  if (int(_indicator_index_) == dep_trend_indicator_index_) {
    dep_trend_val_ = fabs(_new_value_);
  }

  if (weighted_eigen_ready_) {
    if (!is_ready_) {
      if (!is_ready_vec_[_indicator_index_] && indep_market_view_vec_[_indicator_index_]->is_ready()) {
        is_ready_vec_[_indicator_index_] = true;
        InitializeValues(_indicator_index_);
      }
      is_ready_ = AreAllReady();
    }

    if (is_ready_ && !data_interrupted_) {
      if (recompute_indicator_value_) {
        indicator_value_ = 0.0;
        for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
          indicator_value_ += st_weight_vec_[i] * trend_vec_[i];
        }
        recompute_indicator_value_ = false;
      }
      if (indicator_value_ * trend_vec_[dep_index_in_portfolio_] < 0 && dep_trend_val_ > brk_trend_val_) {
        NotifyIndicatorListeners(0);
      } else {
        NotifyIndicatorListeners(indicator_value_);
      }
    }
  }
}

void VolumePCADeviationPairsPort2::OnVolumeUpdate(unsigned int t_index_, double r_new_volume_value_) {
  risk_volume_[t_index_] = risk_multiplier_[t_index_] * r_new_volume_value_;

  if (!is_ready_) {
    if (!is_ready_vec_[t_index_] && indep_market_view_vec_[t_index_]->is_ready()) {
      is_ready_vec_[t_index_] = true;
      InitializeValues(t_index_);
    }
    is_ready_ = AreAllReady();
  }

  if (is_ready_ && !data_interrupted_) {
    double t_sum_weighted_eigens_ = 0;  // Sum ( eigen1_i^2 * RV_i^2 )
    for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
      t_sum_weighted_eigens_ += GetSquareOf(eigen_components_[i]) * GetSquareOf(risk_volume_[i]);
    }

    if (t_sum_weighted_eigens_ > 0) {
      weighted_eigen_ready_ = true;
      double t_indicator_value_ = 0.0;

      for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
        st_weight_vec_[i] = ((1 / stdev_each_constituent_[i]) * eigen_components_[i] * GetSquareOf(risk_volume_[i])) /
                            t_sum_weighted_eigens_;
        // { ( (1 / stdev_i) * eigen1_i * RiskVolume_i^2 ) / Sum ( eigen1_i^2 * RV_i^2 ) }

        // st_weight_vec_[i] *= eigen1_dep*stdev_dep
        st_weight_vec_[i] *=
            eigen_components_[dep_index_in_portfolio_] * stdev_each_constituent_[dep_index_in_portfolio_];

        if (int(i) == dep_index_in_portfolio_) {
          st_weight_vec_[i] += -1;
        }
        t_indicator_value_ += st_weight_vec_[i] * trend_vec_[i];
      }

      indicator_value_ = t_indicator_value_;
      NotifyIndicatorListeners(indicator_value_);
    } else {
      weighted_eigen_ready_ =
          false;  // for a long time trading hasn't happened for any of the security in the portfolio
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void VolumePCADeviationPairsPort2::InitializeValues(int _index_) {
  risk_multiplier_[_index_] =
      HFSAT::CurveUtils::dv01(indep_market_view_vec_[_index_]->shortcode(), watch_.YYYYMMDD(),
                              indep_market_view_vec_[_index_]->price_from_type(kPriceTypeMidprice));

  if (risk_multiplier_[_index_] <= 0.0) {
    std::cerr << "dv01 not found for shc: " << indep_market_view_vec_[_index_]->shortcode() << std::endl;
    exit(1);
  }
}
}
