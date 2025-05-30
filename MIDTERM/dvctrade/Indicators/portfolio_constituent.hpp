/**
   \file Indicators/portfolio_constituent.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_PORTFOLIO_CONSTITUENT_H
#define BASE_INDICATORS_PORTFOLIO_CONSTITUENT_H

#include <vector>
#include <string>
#include <map>

namespace HFSAT {
typedef struct {
  double weight_;
  std::string shortcode_;
} PortfolioConstituent_t;

typedef std::vector<PortfolioConstituent_t> PortfolioConstituentVec;

typedef std::map<std::string, PortfolioConstituentVec> ShortcodePortfolioConstituentVecMap;
typedef std::map<std::string, PortfolioConstituentVec>::iterator ShortcodePortfolioConstituentVecMapIter_t;
typedef std::map<std::string, PortfolioConstituentVec>::const_iterator ShortcodePortfolioConstituentVecMapCIter_t;

namespace PortfolioConstituentUtil {
/// Does not zero out the given vector _shortcode_vec_ and adds to it, the shortcodes corresponding to the
/// _portfolio_constituent_vec_
/// Called from CollectShortCodes since there are already shortcodes inthe given vector
inline void AddShortcodes(const PortfolioConstituentVec& _portfolio_constituent_vec_,
                          std::vector<std::string>& _shortcode_vec_) {
  for (auto i = 0u; i < _portfolio_constituent_vec_.size(); i++) {
    _shortcode_vec_.push_back(_portfolio_constituent_vec_[i].shortcode_);
  }
}

/// Clears the given vector _shortcode_vec_ and sets it to the shortcodes corresponding to the
/// _portfolio_constituent_vec_
inline void GetShortcodes(const PortfolioConstituentVec& _portfolio_constituent_vec_,
                          std::vector<std::string>& _shortcode_vec_) {
  _shortcode_vec_.clear();
  AddShortcodes(_portfolio_constituent_vec_, _shortcode_vec_);
}
}
}

#endif  // BASE_INDICATORS_PORTFOLIO_CONSTITUENT_H
