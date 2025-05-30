/**
    \file IndicatorsCode/volume_ratio_calculator_code.cpp

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

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

#include "dvctrade/Indicators/volume_ratio_calculator_port.hpp"

namespace HFSAT {

VolumeRatioCalculatorPort* VolumeRatioCalculatorPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& t_portfolio_descriptor_shortcode_,
    double _fractional_seconds_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_portfolio_descriptor_shortcode_ << ' ' << _fractional_seconds_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, VolumeRatioCalculatorPort*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new VolumeRatioCalculatorPort(t_dbglogger_, r_watch_, concise_indicator_description_,
                                      t_portfolio_descriptor_shortcode_, _fractional_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

VolumeRatioCalculatorPort::VolumeRatioCalculatorPort(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                     const std::string& t_concise_indicator_description_,
                                                     const std::string& t_portfolio_descriptor_shortcode_,
                                                     double _fractional_seconds_)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      indep_market_view_vec_(),
      volume_ratio_(1.0),
      volume_ratio_vec_(),
      volume_ratio_calculator_vec_(),
      security_id_weight_map_(),
      volume_ratio_listener_vec_(),
      concise_indicator_description_(t_concise_indicator_description_) {
  std::vector<std::string> shortcode_vec_;
  IndicatorUtil::GetPortfolioShortCodeVec(t_portfolio_descriptor_shortcode_, shortcode_vec_);

  (ShortcodeSecurityMarketViewMap::GetUniqueInstance())
      .GetSecurityMarketViewVec(shortcode_vec_, indep_market_view_vec_);
  const EigenConstituentsVec& eigen_constituent_vec =
      (PcaWeightsManager::GetUniqueInstance()).GetEigenConstituentVec(t_portfolio_descriptor_shortcode_);
  // const std::vector < double > & stdev_constituent_vec = (PcaWeightsManager::GetUniqueInstance (
  // )).GetPortfolioStdevs ( t_portfolio_descriptor_shortcode_ ) ;
  if (eigen_constituent_vec.empty()) {
    std::cerr << "PCAPortPrice::EigenConstituentsVec " << t_portfolio_descriptor_shortcode_
              << "does not have even single eigenvectors/values computed " << std::endl;
    ExitVerbose(kPCAWeightManagerMissingPortFromMap, t_portfolio_descriptor_shortcode_.c_str());
  }
  // if ( stdev_constituent_vec.size ( ) != shortcode_vec_.size ( ) )
  //   {
  //     std::cerr << "PCAPortPrice::Stdevs computed for "<< stdev_constituent_vec.size() << " but portfolio size is "
  //     << shortcode_vec_.size ( ) << std::endl;
  //     ExitVerbose (kPCAWeightManagerMissingPortFromMap, t_portfolio_descriptor_shortcode_.c_str () );
  //   }

  if (!eigen_constituent_vec.empty()) {
    if (eigen_constituent_vec[0].eigenvec_components_.size() != shortcode_vec_.size()) {
      std::cerr << "PCAPortPrice:: PCA EIgenvector components size"
                << eigen_constituent_vec[0].eigenvec_components_.size() << " but portfolio size is "
                << shortcode_vec_.size() << std::endl;
      ExitVerbose(kPCAWeightManagerMissingPortFromMap, t_portfolio_descriptor_shortcode_.c_str());
    }
  }
  double sum_of_eigen_compo_ = 0;

  for (unsigned int t_shortcode_vec_index_ = 0; t_shortcode_vec_index_ < shortcode_vec_.size();
       t_shortcode_vec_index_++) {
    sum_of_eigen_compo_ += eigen_constituent_vec[0].eigenvec_components_[t_shortcode_vec_index_];
  }

  for (unsigned int ii = 0u; ii < shortcode_vec_.size(); ii++) {
    SecurityMarketView* p_this_indep_market_view_ = indep_market_view_vec_[ii];
    if (p_this_indep_market_view_ == NULL) {
      std::cerr << "p_this_indep_market_view_ NULL in recent_volume_ratio_calculator_port.cpp constructor for "
                << shortcode_vec_[ii] << std::endl;
      ExitVerbose(kExitErrorCodeGeneral,
                  "p_this_indep_market_view_ NULL in recent_volume_ratio_calculator_port.cpp constructor");
    }

    double t_thisweight_ = (eigen_constituent_vec[0].eigenvec_components_[ii] / sum_of_eigen_compo_);

    auto this_array_index_ = volume_ratio_vec_.size();
    volume_ratio_vec_.push_back(1.0);  // default value
    security_id_weight_map_.push_back(t_thisweight_);
    volume_ratio_calculator_vec_.push_back(VolumeRatioCalculator::GetUniqueInstance(
        t_dbglogger_, r_watch_, *p_this_indep_market_view_, _fractional_seconds_));
    if (volume_ratio_calculator_vec_.size() != this_array_index_ + 1) {
      std::cerr << "Some error in check volume_ratio_calculator_vec_.size() != this_array_index_+ 1 in "
                   "recent_volume_ratio_calculator_port.cpp constructor"
                << std::endl;
    }
    volume_ratio_calculator_vec_[this_array_index_]->AddVolumeRatioListener(this_array_index_, this);
  }

  NotifyListeners();
}

void VolumeRatioCalculatorPort::OnVolumeRatioUpdate(const unsigned int array_index_,
                                                    const double& t_new_volume_ratio_) {
  if (array_index_ < volume_ratio_vec_.size())  // should be always true
  {
    volume_ratio_vec_[array_index_] = t_new_volume_ratio_;
    volume_ratio_ = 0;
    for (auto ii = 0u; ii < volume_ratio_vec_.size(); ii++) {
      volume_ratio_ += volume_ratio_vec_[ii] * security_id_weight_map_[ii];
    }
    NotifyListeners();
  }
}
}
