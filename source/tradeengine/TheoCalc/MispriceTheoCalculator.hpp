#ifndef _MISPRICE_THEO_CALCULATOR_H
#define _MISPRICE_THEO_CALCULATOR_H

#include "tradeengine/TheoCalc/RatioTheoCalculator.hpp"

class MispriceTheoCalculator : public RatioTheoCalculator {
 protected:
  void LoadParams();
  void UpdateTheoPrices(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  double ratio_threshold_;
  double bid_start_ratio_;
  double ask_start_ratio_;

  double current_bid_ratio_;
  double current_ask_ratio_;
  double current_mid_ratio_;

  bool ready_to_trade_;

 public:
  MispriceTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                         HFSAT::DebugLogger& _dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                         int _aggressive_get_flat_mfm_, int eff_squareoff_start_utc_mfm_, double bid_multiplier_,
                         double ask_multiplier_);

  virtual ~MispriceTheoCalculator() {}
  void LoadInitialParams();
};

#endif  // _MISPRICE_THEO_CALCULATOR_H
