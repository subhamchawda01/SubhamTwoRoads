/**
   \file MarketAdapter/shortcode_minute_bar_smv_map.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#pragma once

#include <string>
#include <map>
#include <vector>

#include "dvccode/CDef/error_utils.hpp"
#include "baseinfra/MinuteBar/minute_bar_security_market_view.hpp"

namespace HFSAT {

/** this class keeps a map from shortcode to SecurityMarketView *
 */
class ShortcodeMinuteBarSMVMap {
 private:
  ShortcodeMinuteBarSMVMap(const ShortcodeMinuteBarSMVMap&);

 protected:
  std::map<std::string, MinuteBarSecurityMarketView*> shortcode_smv_map_;

  ShortcodeMinuteBarSMVMap() : shortcode_smv_map_() {}

 public:
  static ShortcodeMinuteBarSMVMap& GetUniqueInstance() {
    static ShortcodeMinuteBarSMVMap uniqueinstance_;
    return uniqueinstance_;
  }

  static MinuteBarSecurityMarketView* StaticGetSecurityMarketView(const std::string& _shortcode_) {
    return GetUniqueInstance().GetSecurityMarketView(_shortcode_);
  }

  static void StaticCheckValid(const std::string& _shortcode_) { return GetUniqueInstance().CheckValid(_shortcode_); }

  ~ShortcodeMinuteBarSMVMap(){};

  inline void AddEntry(const std::string& _shortcode_, MinuteBarSecurityMarketView* _security_market_view__) {
    if (_security_market_view__ != NULL) {
      shortcode_smv_map_[_shortcode_] = _security_market_view__;
    }
  }

  MinuteBarSecurityMarketView* GetSecurityMarketView(const std::string& _shortcode_) {
    std::map<std::string, MinuteBarSecurityMarketView*>::const_iterator _citer_ = shortcode_smv_map_.find(_shortcode_);
    if (_citer_ != shortcode_smv_map_.end()) return _citer_->second;
    return NULL;
  }

  void CheckValid(const std::string& _shortcode_) {
    std::map<std::string, MinuteBarSecurityMarketView*>::const_iterator _citer_ = shortcode_smv_map_.find(_shortcode_);
    if (_citer_ != shortcode_smv_map_.end()) return;
    ExitVerbose(kShortcodeSecurityMarketViewMapNoSMVInMap);
    return;
  }

  void GetSecurityMarketViewVec(const std::vector<std::string>& _shortcode_vec_,
                                std::vector<MinuteBarSecurityMarketView*>& _smvvec_) {
    _smvvec_.clear();  // just for safety .. typically we expect empty vectors here
    for (auto i = 0u; i < _shortcode_vec_.size(); i++) {
      _smvvec_.push_back(GetSecurityMarketView(_shortcode_vec_[i]));
    }
  }
};

//-----------------------------------------------

class SecIDMinuteBarSMVMap {
 private:
  SecIDMinuteBarSMVMap(const SecIDMinuteBarSMVMap&);

 protected:
  std::map<int, MinuteBarSecurityMarketView*> secid_smv_map_;

  SecIDMinuteBarSMVMap() : secid_smv_map_() {}

 public:
  static SecIDMinuteBarSMVMap& GetUniqueInstance() {
    static SecIDMinuteBarSMVMap uniqueinstance_;
    return uniqueinstance_;
  }

  static MinuteBarSecurityMarketView* StaticGetSecurityMarketView(int sec_id) {
    return GetUniqueInstance().GetSecurityMarketView(sec_id);
  }

  static void StaticCheckValid(int sec_id) { return GetUniqueInstance().CheckValid(sec_id); }

  ~SecIDMinuteBarSMVMap(){};

  inline void AddEntry(int sec_id, MinuteBarSecurityMarketView* smv) {
    if (smv != NULL) {
      secid_smv_map_[sec_id] = smv;
    }
  }

  MinuteBarSecurityMarketView* GetSecurityMarketView(int sec_id) {
    std::map<int, MinuteBarSecurityMarketView*>::const_iterator _citer_ = secid_smv_map_.find(sec_id);
    if (_citer_ != secid_smv_map_.end()) return _citer_->second;
    return NULL;
  }

  void CheckValid(int sec_id) {
    std::map<int, MinuteBarSecurityMarketView*>::const_iterator _citer_ = secid_smv_map_.find(sec_id);
    if (_citer_ != secid_smv_map_.end()) return;
    ExitVerbose(kShortcodeSecurityMarketViewMapNoSMVInMap);
    return;
  }
};
}
