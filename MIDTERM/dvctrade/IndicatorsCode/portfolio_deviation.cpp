/**
    \file IndicatorsCode/portfolio_deviation.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/portfolio_deviation.hpp"
#include "dvctrade/Indicators/pca_weights_manager.hpp"
namespace HFSAT {

void PortfolioDeviation::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                    std::vector<std::string>& _ors_source_needed_vec_,
                                                    const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() >= 6u) {
    IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[3], _shortcodes_affecting_this_indicator_);
  }
}

PortfolioDeviation* PortfolioDeviation::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                            const Watch& r_watch_,
                                                                            const std::vector<const char*>& r_tokens_,
                                                                            PriceType_t _basepx_pxtype_) {
  if (r_tokens_.size() < 6) {
    ExitVerbose(kExitErrorCodeGeneral, "PortfolioDeviation needs 6 tokens");
    return NULL;
  }

  // INDICATOR  _this_weight_  _indicator_string_  _portfolio_descriptor_shortcode_
  // _fractional_seconds_  _price_type_

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           (std::string)(r_tokens_[3]), atof(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

PortfolioDeviation* PortfolioDeviation::GetUniqueInstance(
    DebugLogger& _dbglogger_, const Watch& _watch_,
    const std::string& _portfolio_descriptor_shortcode_, const double _fractional_seconds_,
    const PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _portfolio_descriptor_shortcode_ << ' '
              << _fractional_seconds_ << ' ' << PriceType_t_To_String(_price_type_);

  std::string concise_indicator_description_(t_temp_oss_.str());
  static std::map<std::string, PortfolioDeviation*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    // Get default values from the pcaweightsmanager
    PcaWeightsManager pca_weights_manager = PcaWeightsManager::GetUniqueInstance();
    EigenConstituentsVec principal_eigen_vec =
        pca_weights_manager.GetEigenConstituentVec(_portfolio_descriptor_shortcode_);

    if (principal_eigen_vec.size() < 1) {
      std::cerr << "PortfolioDeviation: No eigen vectors loaded for " << (_portfolio_descriptor_shortcode_)
                << " in PcaWeightsManager " << std::endl;
      exit(1);
    }

    concise_indicator_description_map_[concise_indicator_description_] = new PortfolioDeviation(
        _dbglogger_, _watch_, concise_indicator_description_, _portfolio_descriptor_shortcode_,
        _fractional_seconds_, principal_eigen_vec[0u].eigenvec_components_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

PortfolioDeviation::PortfolioDeviation(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    const std::string& _portfolio_descriptor_shortcode_, const double _fractional_seconds_,
    const std::vector<double>& t_eigen_components_, const PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_vec_(t_eigen_components_.size(), NULL),
      eigen_components_(t_eigen_components_),
      stdev_each_constituent_(
          (PcaWeightsManager::GetUniqueInstance()).GetPortfolioStdevs(_portfolio_descriptor_shortcode_)),
      returns_vec_(t_eigen_components_.size(), 0.0),
      sum_of_returns_(0.0),
      sum_of_mod_returns_(0.0),
      weight_vec_(t_eigen_components_.size(), 0.0),
      is_ready_vec_(t_eigen_components_.size(), false),
      data_interrupted_vec_(t_eigen_components_.size(), false),
      recompute_indicator_value_(false),
      p_indep_indicator_vec_(t_eigen_components_.size(), NULL) {
  IndicatorUtil::GetPortfolioSMVVec(_portfolio_descriptor_shortcode_, indep_market_view_vec_);

  for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
    if (indep_market_view_vec_[i]->is_ready()) {
      is_ready_vec_[i] = true;      
    }


    SimpleReturns* _dep_indicator_ = SimpleReturns::GetUniqueInstance(
        t_dbglogger_, r_watch_, *indep_market_view_vec_[i], _fractional_seconds_, _price_type_);
    _dep_indicator_->add_unweighted_indicator_listener(i, this);
    p_indep_indicator_vec_[i] = _dep_indicator_;
    double _avg_trend_ = 
        HFSAT::SampleDataUtil::GetAvgForPeriod(indep_market_view_vec_[i]->shortcode(), watch_.YYYYMMDD(), 60, trading_start_mfm_,
                                               trading_end_mfm_, "TREND", true);
    double _avg_price_ = 
        HFSAT::SampleDataUtil::GetAvgForPeriod(indep_market_view_vec_[i]->shortcode(), watch_.YYYYMMDD(), 60, trading_start_mfm_,
                                               trading_end_mfm_, "AvgPrice", true);
    if ( _avg_trend_ > 0 ) {      
      weight_vec_[i] = _avg_price_/_avg_trend_;
    }
  }
  is_ready_ = AreAllReady();
}

void PortfolioDeviation::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if ( !is_ready_ ) {
    if (!is_ready_vec_[_indicator_index_] && indep_market_view_vec_[_indicator_index_]->is_ready()) {
      is_ready_vec_[_indicator_index_] = true;
    }
    is_ready_ = AreAllReady();
  }

  // doing this at every update because we don't want to use stale value
  sum_of_returns_ += weight_vec_[_indicator_index_] * ( _new_value_ - returns_vec_[_indicator_index_] );
  sum_of_mod_returns_ += weight_vec_[_indicator_index_] * (std::fabs(_new_value_) - std::fabs(returns_vec_[_indicator_index_]));
  if ( is_ready_ && !data_interrupted_ ) {
    if ( sum_of_mod_returns_ > 0.0 ) {
      indicator_value_ = sum_of_returns_/sum_of_mod_returns_;
    } else {
      indicator_value_ = 0.0;
    }
    NotifyIndicatorListeners(indicator_value_);
  }
}

}
