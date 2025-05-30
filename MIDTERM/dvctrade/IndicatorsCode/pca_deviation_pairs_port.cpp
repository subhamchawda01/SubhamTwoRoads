/**
    \file IndicatorsCode/pca_deviation_pairs_port.cpp

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
#include "dvctrade/Indicators/simple_trend_port.hpp"
#include "dvctrade/Indicators/pca_deviation_pairs_port.hpp"
#include "dvctrade/Indicators/pca_weights_manager.hpp"
namespace HFSAT {

void PCADeviationPairsPort::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                              std::vector<std::string>& _ors_source_needed_vec_,
                                              const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[4], _shortcodes_affecting_this_indicator_);
}

PCADeviationPairsPort* PCADeviationPairsPort::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const std::vector<const char*>& r_tokens_,
                                                                PriceType_t _basepx_pxtype_) {
  if (r_tokens_.size() < 8) {
    std::cerr << "PCADeviationPairsPort syntax incorrect! given text:";
    for (auto i = 0u; i < r_tokens_.size(); i++) {
      std::cerr << " " << r_tokens_[i];
    }
    std::cerr << std::endl;
    std::cerr << "Expected syntax: INDICATOR  _this_weight_  PCADeviationPairsPort  _dep_market_view_  "
                 "_portfolio_descriptor_shortcode_  _fractional_seconds_  _eigen_vector_index_  _price_type_"
              << std::endl;

    exit(1);
  }

  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _portfolio_descriptor_shortcode_
  // _fractional_seconds_  _eigen_vector_index_  _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  // Get default values from the pcaweightsmanager
  PcaWeightsManager pca_weights_manager = PcaWeightsManager::GetUniqueInstance();
  EigenConstituentsVec principal_eigen_vec = pca_weights_manager.GetEigenConstituentVec(r_tokens_[4]);

  if (principal_eigen_vec.size() < 1) {
    std::cerr << " No eigen vectors loaded for " << (r_tokens_[4]) << " in PcaWeightsManager " << std::endl;
    return NULL;  // error condition !
  }

  unsigned int eigen_vector_index_ = 0u;
  int given_index = atoi(r_tokens_[6]);
  if (given_index >= 1) {  // since the index is given as 1,2,...
    // while in C++ array indices are 0,1,...
    eigen_vector_index_ = given_index - 1;
  }
  if (given_index < 0) {
    std::cerr << "Given eigen vector index " << given_index << " not valid!" << std::endl;
    exit(1);
  }
  if (eigen_vector_index_ >= principal_eigen_vec.size()) {
    std::cerr << "Given eigen vector index " << given_index << " not present!" << std::endl;
    exit(1);
  }

  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      (std::string)(r_tokens_[4]), atof(r_tokens_[5]), principal_eigen_vec[eigen_vector_index_].eigenvec_components_,
      StringToPriceType_t(r_tokens_[7]));
}

PCADeviationPairsPort* PCADeviationPairsPort::GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                                SecurityMarketView& _dep_market_view_,
                                                                const std::string& _portfolio_descriptor_shortcode_,
                                                                const double _fractional_seconds_,
                                                                const std::vector<double>& eigen_components_,
                                                                const PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _portfolio_descriptor_shortcode_ << ' '
              << _fractional_seconds_ << ' ' << PriceType_t_To_String(_price_type_);

  std::string concise_indicator_description_(t_temp_oss_.str());
  static std::map<std::string, PCADeviationPairsPort*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new PCADeviationPairsPort(
        _dbglogger_, _watch_, concise_indicator_description_, _dep_market_view_, _portfolio_descriptor_shortcode_,
        _fractional_seconds_, eigen_components_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

PCADeviationPairsPort::PCADeviationPairsPort(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    SecurityMarketView& r_dep_market_view_, const std::string& _portfolio_descriptor_shortcode_,
    const double _fractional_seconds_, const std::vector<double>& t_eigen_components_, const PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(r_dep_market_view_),
      indep_pca_price_(
          *(PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _price_type_))),
      dep_price_trend_(0.0),
      indep_price_trend_(0.0),
      current_projected_trend_(0),
      eigen_components_(t_eigen_components_),
      dep_index_in_portfolio_(
          (PcaWeightsManager::GetUniqueInstance())
              .GetPortfolioShortCodeIndexer(_portfolio_descriptor_shortcode_, r_dep_market_view_.shortcode())),
      stdev_each_constituent_(
          (PcaWeightsManager::GetUniqueInstance()).GetPortfolioStdevs(_portfolio_descriptor_shortcode_)) {
  if (dep_index_in_portfolio_ < 0) {
    DBGLOG_TIME_CLASS_FUNC << " since " << dep_index_in_portfolio_
                           << " not in PortFolio. Dep:" << r_dep_market_view_.shortcode()
                           << " Port: " << _portfolio_descriptor_shortcode_ << " Not initializing trends"
                           << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    // watch_.subscribe_BigTimePeriod ( this ); // not needed till now
    SimpleTrend* _dep_indicator_ =
        SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_.shortcode(), _fractional_seconds_, _price_type_);
    _dep_indicator_->add_unweighted_indicator_listener(1u, this);

    ///  This simpletrend should automatically use the PCAPortPrice
    SimpleTrendPort* _indep_indicator_ = SimpleTrendPort::GetUniqueInstance(
        t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _fractional_seconds_, _price_type_);
    _indep_indicator_->add_unweighted_indicator_listener(2u, this);

#if EQUITY_INDICATORS_ALWAYS_READY
    if (_dep_indicator_->IsIndicatorReady() && _indep_indicator_->IsIndicatorReady()) {
      is_ready_ = true;
    }
#endif
  }
}

void PCADeviationPairsPort::WhyNotReady() {
  DBGLOG_TIME_CLASS_FUNC << " why not ready " << dep_index_in_portfolio_ << " " << (is_ready_ ? "Y" : "N")
                         << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;
  if (!is_ready_) {
    DBGLOG_TIME_CLASS_FUNC << " ! is_ready_ " << dep_index_in_portfolio_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    if (!dep_market_view_.is_ready_complex(2)) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!indep_pca_price_.is_ready()) {
      DBGLOG_TIME_CLASS << indep_pca_price_.shortcode() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void PCADeviationPairsPort::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
#if EQUITY_INDICATORS_ALWAYS_READY
    if ((dep_market_view_.is_ready_complex(2) || IndicatorUtil::IsEquityShortcode(dep_market_view_.shortcode())) &&
        (indep_pca_price_.is_ready()))
#else
    if (dep_market_view_.is_ready_complex(2) && indep_pca_price_.is_ready())
#endif
    {
      is_ready_ = true;
      // InitializeValues ( ) ;
    }
  } else {
    if (_indicator_index_ == 1u) {
      dep_price_trend_ = _new_value_;
    } else if (_indicator_index_ == 2u) {
      indep_price_trend_ = _new_value_;
      /// PCA_PRICE[indep_price_trend_] * Normalizing factor(DENOm) * Sigma_of_dep_* Eigen_compo_dep
      current_projected_trend_ = indep_price_trend_ * indep_pca_price_.get_normalizing_factor() *
                                 stdev_each_constituent_[dep_index_in_portfolio_] *
                                 eigen_components_[dep_index_in_portfolio_];
    }

    indicator_value_ = current_projected_trend_ - dep_price_trend_;

    //  Divide the indicator with the sigma_dep_
    indicator_value_ /= stdev_each_constituent_[dep_index_in_portfolio_];

    if (data_interrupted_) indicator_value_ = 0;

    NotifyIndicatorListeners(indicator_value_);
  }
}
void PCADeviationPairsPort::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                    const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void PCADeviationPairsPort::OnMarketDataResumed(const unsigned int _security_id_) { data_interrupted_ = false; }
}
