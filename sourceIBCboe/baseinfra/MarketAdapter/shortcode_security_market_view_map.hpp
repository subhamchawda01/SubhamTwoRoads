/**
   \file MarketAdapter/shortcode_security_market_view_map.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_INDICATORS_SHORTCODE_SECURITY_MARKET_VIEW_MAP_H
#define BASE_INDICATORS_SHORTCODE_SECURITY_MARKET_VIEW_MAP_H

#include <string>
#include <map>
#include <vector>

#include "dvccode/CDef/error_utils.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#define PAIRSTRADEFILE "/spare/local/tradeinfo/PairTradeInfo/portfolio_inputs.txt"

namespace HFSAT {

/** this class keeps a map from shortcode to SecurityMarketView *
 */
class ShortcodeSecurityMarketViewMap {
 private:
  ShortcodeSecurityMarketViewMap(const ShortcodeSecurityMarketViewMap&);

 protected:
  std::map<std::string, SecurityMarketView*> shortcode_smv_map_;

  ShortcodeSecurityMarketViewMap() : shortcode_smv_map_() {}

 public:
  static ShortcodeSecurityMarketViewMap& GetUniqueInstance() {
    static ShortcodeSecurityMarketViewMap uniqueinstance_;
    return uniqueinstance_;
  }

  static SecurityMarketView* StaticGetSecurityMarketView(const std::string& _shortcode_) {
    return GetUniqueInstance().GetSecurityMarketView(_shortcode_);
  }

  static void StaticCheckValid(const std::string& _shortcode_) { return GetUniqueInstance().CheckValid(_shortcode_); }
  static bool StaticCheckValidWithoutExit(const std::string& _shortcode_) {
    return GetUniqueInstance().CheckValidWithoutExit(_shortcode_);
  }
  static bool StaticCheckValidPortWithoutExit(const std::string& _shortcode_) {
    return GetUniqueInstance().CheckValidPortWithoutExit(_shortcode_);
  }
  static bool StaticCheckValidShortCodeOrPortWithExit(const std::string& _shortcode_) {
    return GetUniqueInstance().CheckValidPortWithoutExit(_shortcode_);
  }
  ~ShortcodeSecurityMarketViewMap(){};

  inline void AddEntry(const std::string& _shortcode_, SecurityMarketView* _security_market_view__) {
    if (_security_market_view__ != NULL) {
      shortcode_smv_map_[_shortcode_] = _security_market_view__;
    }
  }

  SecurityMarketView* GetSecurityMarketView(const std::string& _shortcode_) {
    std::map<std::string, SecurityMarketView*>::const_iterator _citer_ = shortcode_smv_map_.find(_shortcode_);
    if (_citer_ != shortcode_smv_map_.end()) return _citer_->second;
    return NULL;
  }

  void CheckValid(const std::string& _shortcode_) {
    std::map<std::string, SecurityMarketView*>::const_iterator _citer_ = shortcode_smv_map_.find(_shortcode_);
    if (_citer_ != shortcode_smv_map_.end()) return;
    ExitVerbose(kShortcodeSecurityMarketViewMapNoSMVInMap);
    return;
  }
  bool CheckValidWithoutExit(const std::string& _shortcode_) {
    std::map<std::string, SecurityMarketView*>::const_iterator _citer_ = shortcode_smv_map_.find(_shortcode_);
    if (_citer_ != shortcode_smv_map_.end()) return true;
    return false;
  }
  bool CheckValidPortWithoutExit(const std::string& _shortcode_) {
    std::string t_portfolio_descriptor_shortcode_ = _shortcode_;
    std::string line;
    std::ifstream Portfoliofile(PAIRSTRADEFILE);
    // bool flag_port_found_ = false;
    if (Portfoliofile.is_open()) {
      while (getline(Portfoliofile, line)) {
        std::istringstream buf(line);
        std::istream_iterator<std::string> beg(buf), end;

        std::vector<std::string> tokens(beg, end);
        if (tokens.size() == 0) return false;
        if (tokens[0] == t_portfolio_descriptor_shortcode_) {
          return true;
        }
      }
    }
    return false;
  }
  void CheckValidShortCodeOrPortWithExit(const std::string& _shortcode_) {
    if (CheckValidWithoutExit(_shortcode_) || CheckValidPortWithoutExit(_shortcode_)) {
      return;
    } else {
      ExitVerbose(kShortcodeSecurityMarketViewMapNoSMVInMap);
      return;
    }
  }
  void GetSecurityMarketViewVec(const std::vector<std::string>& _shortcode_vec_,
                                std::vector<SecurityMarketView*>& _smvvec_) {
    _smvvec_.clear();  // just for safety .. typically we expect empty vectors here
    for (auto i = 0u; i < _shortcode_vec_.size(); i++) {
      _smvvec_.push_back(GetSecurityMarketView(_shortcode_vec_[i]));
    }
  }
};
}

#endif  // BASE_INDICATORS_SHORTCODE_SECURITY_MARKET_VIEW_MAP_H
