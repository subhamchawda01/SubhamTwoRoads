/**
   \file Indicators/portfolio_constituent_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_PORTFOLIO_CONSTITUENT_MANAGER_H
#define BASE_INDICATORS_PORTFOLIO_CONSTITUENT_MANAGER_H

#include "dvccode/CDef/error_utils.hpp"
#include "dvctrade/Indicators/portfolio_constituent.hpp"

namespace HFSAT {

/// Used to store information of Portfolios for making indicators
/// Weights are only used in _Port variables, through PortfolioPrice
/// In all other indicators this is used primarily as a way to describe
/// a "Set" of shortcodes by a single word/shortcode
class PortfolioConstituentManager {
 protected:
  ShortcodePortfolioConstituentVecMap shortcode_constituentvec_map_;
  PortfolioConstituentVec empty_;

  PortfolioConstituentManager() { LoadPortfolioInfoFile(); }

 public:
  static PortfolioConstituentManager &GetUniqueInstance() {
    static PortfolioConstituentManager uniqueinstance_;
    return uniqueinstance_;
  }

  ~PortfolioConstituentManager() {}

  const PortfolioConstituentVec &GetPortfolioConstituentVec(const std::string &_portfolio_descriptor_shortcode_) {
    ShortcodePortfolioConstituentVecMapCIter_t _citer_ =
        shortcode_constituentvec_map_.find(_portfolio_descriptor_shortcode_);
    if (_citer_ != shortcode_constituentvec_map_.end()) {
      return _citer_->second;
    }
    ExitVerbose(kPortfolioConstituentManagerMissingPortFromMap, _portfolio_descriptor_shortcode_.c_str());
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
#endif  // BASE_INDICATORS_PORTFOLIO_CONSTITUENT_MANAGER_H
