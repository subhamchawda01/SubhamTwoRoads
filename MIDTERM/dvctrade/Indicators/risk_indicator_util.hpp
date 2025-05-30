/**
    \file Indicators/risk_indicator_util.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_RISK_INDICATOR_UTIL_H
#define BASE_INDICATORS_RISK_INDICATOR_UTIL_H

#include <string>
#include <vector>
#include <stdlib.h>

#include "baseinfra/OrderRouting/shortcode_prom_order_manager_map.hpp"

#include "dvctrade/Indicators/risk_portfolio_constituent_manager.hpp"

namespace HFSAT {

/// a set of functions called primarily from SelfPosition based indicators to
/// return what our risk is or how it is changing, what is the short term trend
/// adjusted position etc.
namespace RiskIndicatorUtil {

inline void AddPortfolioShortCodeVec(const std::string& _portfolio_descriptor_shortcode_,
                                     std::vector<std::string>& _shortcode_vec_) {
  const PortfolioConstituentVec& portfolio_constituent_vec_ =
      (RiskPortfolioConstituentManager::GetUniqueInstance())
          .GetPortfolioConstituentVec(_portfolio_descriptor_shortcode_);
  PortfolioConstituentUtil::AddShortcodes(portfolio_constituent_vec_, _shortcode_vec_);
}

inline void GetPortfolioShortCodeVec(const std::string& _portfolio_descriptor_shortcode_,
                                     std::vector<std::string>& _shortcode_vec_) {
  const PortfolioConstituentVec& portfolio_constituent_vec_ =
      (RiskPortfolioConstituentManager::GetUniqueInstance())
          .GetPortfolioConstituentVec(_portfolio_descriptor_shortcode_);
  PortfolioConstituentUtil::GetShortcodes(portfolio_constituent_vec_, _shortcode_vec_);
}
};
}

#endif  // BASE_INDICATORS_RISK_INDICATOR_UTIL_H
