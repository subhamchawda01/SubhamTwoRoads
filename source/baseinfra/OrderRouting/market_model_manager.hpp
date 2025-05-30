/**
    \file OrderRouting/market_model_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_ORDERROUTING_MARKET_MODEL_MANAGER_H
#define BASE_ORDERROUTING_MARKET_MODEL_MANAGER_H

#include <map>
#include <string>
#include "baseinfra/OrderRouting/market_model.hpp"

namespace HFSAT {

/// Class to read the MarketModel info from a file per product, store it, and
/// based on the given _market_model_index_ return the MarketModel for this simtrader to use
class MarketModelManager {
  typedef std::map<std::string, std::vector<MarketModel> > ShortcodeMarketModelMap;
  typedef std::map<std::string, std::vector<MarketModel> >::const_iterator ShortcodeMarketModelMapCiter_t;
  typedef std::map<std::string, std::map<int,int> > ExchDrDatesDelay;


  MarketModelManager(const MarketModelManager&);

 private:
  int tradingdate_;

 protected:
  ShortcodeMarketModelMap shortcode_market_model_map_;
  ExchDrDatesDelay ExchDrDatesDelay_;

  MarketModelManager(int t_tradingdate_) {
    tradingdate_ = t_tradingdate_;
    LoadMarketModelInfoFile(tradingdate_);
  }

 public:
  static MarketModelManager& GetUniqueInstance(int tradingdate_) {
    static MarketModelManager uniqueinstance_(tradingdate_);
    return uniqueinstance_;
  }

  virtual ~MarketModelManager() {}

  static void GetMarketModel(const std::string& _shortcode_, int _market_model_index_, MarketModel& market_model_,
                             int tradingdate_) {
    MarketModelManager::GetUniqueInstance(tradingdate_)
        ._GetMarketModel(_shortcode_, _market_model_index_, market_model_, tradingdate_);
  }

 protected:
  void _GetMarketModel(const std::string& _shortcode_, int _market_model_index_, MarketModel& market_model_,
                       int tradingdate_) const;
  std::string FindClosestDatedMarketModelInfoFile(int tradingdate_);
  /// \brief to load the market_model info from file
  /// Reads a file of the form
  /// shortcode_ com_msecs_ conf_msecs_ cxl_msecs_ exec_msecs_
  void LoadMarketModelInfoFile(int tradingdate_);
  void LoadDrDates();
  int CheckDateDr(std::string shortcode_, int tradeing_date_);
};
}
#endif  // BASE_ORDERROUTING_MARKET_MODEL_MANAGER_H
