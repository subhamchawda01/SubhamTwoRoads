/**
    \file OrderRouting/shortcode_prom_order_manager_map.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_SHORTCODE_PROM_ORDER_MANAGER_MAP_H
#define BASE_INDICATORS_SHORTCODE_PROM_ORDER_MANAGER_MAP_H

#include <string>
#include <map>
#include <vector>

#include "baseinfra/OrderRouting/prom_order_manager.hpp"

namespace HFSAT {

/// this class keeps a map from shortcode to PromOrderManager *, to be primarily used by variables to bind themselves as
/// listeners
class ShortcodePromOrderManagerMap {
 private:
  ShortcodePromOrderManagerMap(const ShortcodePromOrderManagerMap&);

 protected:
  std::map<std::string, PromOrderManager*> shortcode_pom_map_;

  ShortcodePromOrderManagerMap() : shortcode_pom_map_() {}

 public:
  static ShortcodePromOrderManagerMap& GetUniqueInstance() {
    static ShortcodePromOrderManagerMap uniqueinstance_;
    return uniqueinstance_;
  }
  static PromOrderManager* StaticGetPromOrderManager(const std::string& _shortcode_) {
    return GetUniqueInstance().GetPromOrderManager(_shortcode_);
  }

  ~ShortcodePromOrderManagerMap(){};

  void AddEntry(const std::string& _shortcode_, PromOrderManager* _p_prom_order_manager__) {
    if (_p_prom_order_manager__ != NULL) {
      shortcode_pom_map_[_shortcode_] = _p_prom_order_manager__;
    }
  }

  PromOrderManager* GetPromOrderManager(const std::string& _shortcode_) {
    return ((shortcode_pom_map_.find(_shortcode_) == shortcode_pom_map_.end()) ? NULL
                                                                               : (shortcode_pom_map_[_shortcode_]));
  }

  void GetPromOrderManagerVec(const std::vector<std::string>& _shortcode_vec_,
                              std::vector<PromOrderManager*>& _p_pomvec_) {
    _p_pomvec_.clear();  // just for safety .. typically we expect empty vectors here
    for (auto i = 0u; i < _shortcode_vec_.size(); i++) {
      _p_pomvec_.push_back(GetPromOrderManager(_shortcode_vec_[i]));
    }
  }
};
}

#endif  // BASE_INDICATORS_SHORTCODE_PROM_ORDER_MANAGER_MAP_H
