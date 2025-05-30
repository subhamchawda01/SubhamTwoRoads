/**
   \file Indicators/risk_portfolio_constituent_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_RISK_PORTFOLIO_CONSTITUENT_MANAGER_H
#define BASE_INDICATORS_RISK_PORTFOLIO_CONSTITUENT_MANAGER_H

#include "dvccode/CDef/error_utils.hpp"
#include "dvctrade/Indicators/portfolio_constituent.hpp"

namespace HFSAT {

/// Used to store information of Portfolios for making risk based aggregation
/// Used in TrendAdjustedSelfPositionCombo, where both weight and Set of products are used
/// Made this different than PortfolioConstituentManager because unlike portfolios weighted
/// based on giving price change of each constituent roughly the same effect ,
/// i.e. perhaps divided by stdev, or looking at how overall BFut moved
/// this simply tries to see how our positions in the consituents map to a position of the dependant
/// All _portfolio_descriptor_shortcode_ arguments here are expected to be of the form "..2ZN" i.e. with a dependant,
/// since the objective is to project the positions ( or trend_adjusted_self_positions ) onto some product.
/// like FGBL, FGBM, FGBS onto FGBS. The weights should perhaps be proportional to 4:2:1 or ( the stdev of price change
/// * numbers2dollars )
/// instead of inversely proportional.
/// and the dependant might be more often than not a member of this set of shortcodes.
class RiskPortfolioConstituentManager {
 protected:
  ShortcodePortfolioConstituentVecMap shortcode_constituentvec_map_;
  PortfolioConstituentVec empty_;

  RiskPortfolioConstituentManager() { LoadPortfolioInfoFile(); }

 public:
  static RiskPortfolioConstituentManager &GetUniqueInstance() {
    static RiskPortfolioConstituentManager uniqueinstance_;
    return uniqueinstance_;
  }

  ~RiskPortfolioConstituentManager() {}

  const PortfolioConstituentVec &GetPortfolioConstituentVec(const std::string &_portfolio_descriptor_shortcode_) {
    ShortcodePortfolioConstituentVecMapCIter_t _citer_ =
        shortcode_constituentvec_map_.find(_portfolio_descriptor_shortcode_);
    if (_citer_ != shortcode_constituentvec_map_.end()) {
      return _citer_->second;
    }
    ExitVerbose(kRiskPortfolioConstituentManagerMissingPortFromMap, _portfolio_descriptor_shortcode_.c_str());
    return empty_;
  }

 protected:
  /// Reads a portfolio file of the form
  /// portfolio_descriptor_shortcode_ shortcode_1 weight_1 shortcode_2 weight_2 ... shortcode_N weight_N
  /// END_PORTFOLIO_DESCRIPTION
  /// weight_j is used only in PortfolioPrice as a multiplier on the price change of shortcode_j
  void LoadPortfolioInfoFile();

  void LoadFromFile(const std::string &, std::map<std::string, unsigned int> &, unsigned int);
};
}
#endif  // BASE_INDICATORS_RISK_PORTFOLIO_CONSTITUENT_MANAGER_H
