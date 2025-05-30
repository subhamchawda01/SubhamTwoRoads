/*
 * shortcode_spread_market_view_map.hpp
 *
 *  Created on: 05-Jun-2014
 *      Author: archit
 */

#ifndef SHORTCODE_SPREAD_MARKET_VIEW_MAP_HPP_
#define SHORTCODE_SPREAD_MARKET_VIEW_MAP_HPP_

#include "baseinfra/MarketAdapter/spread_market_view.hpp"

namespace HFSAT {

class ShortcodeSpreadMarketViewMap {
 private:
  ShortcodeSpreadMarketViewMap(const ShortcodeSpreadMarketViewMap &);

 protected:
  std::map<std::string, SpreadMarketView *> shortcode_spread_maket_view_map_;
  ShortcodeSpreadMarketViewMap() : shortcode_spread_maket_view_map_() {}

  inline SpreadMarketView *_GetSpreadMarketView(const std::string _spread_shc_) {
    if (shortcode_spread_maket_view_map_.find(_spread_shc_) != shortcode_spread_maket_view_map_.end()) {
      return shortcode_spread_maket_view_map_[_spread_shc_];
    }
    return NULL;
  }

  inline void _AddEntry(const std::string _spread_shc_, SpreadMarketView *_spread_market_view_) {
    if (_spread_market_view_ != NULL) {
      shortcode_spread_maket_view_map_[_spread_shc_] = _spread_market_view_;
    }
  }

 public:
  static inline ShortcodeSpreadMarketViewMap &GetUniqueInstance() {
    static ShortcodeSpreadMarketViewMap uniqueinstance_;
    return uniqueinstance_;
  }

  static inline SpreadMarketView *GetSpreadMarketView(const std::string _spread_shc_) {
    return GetUniqueInstance()._GetSpreadMarketView(_spread_shc_);
  }

  static inline void AddEntry(const std::string _spread_shc_, SpreadMarketView *_spread_market_view_) {
    GetUniqueInstance()._AddEntry(_spread_shc_, _spread_market_view_);
  }

  virtual ~ShortcodeSpreadMarketViewMap() {}
};

} /* namespace HFSAT */

#endif /* SHORTCODE_SPREAD_MARKET_VIEW_MAP_HPP_ */
