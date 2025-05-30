#pragma once

#include "tradeengine/Executioner/BaseExecutioner.hpp"
#include "tradeengine/TheoCalc/MidTermTheoCalculator.hpp"
#include "tradeengine/TheoCalc/SquareOffTheoCalculator.hpp"
#include "tradeengine/Utils/BarGenerator.hpp"

/*! \brief Header file for HighMoveTheo
*/
class HighMoveTheoCalculator : public MidTermTheoCalculator {
 protected:
  int current_directional_cascade_;
  double threshold_;
  double current_obv_;
  double max_obv_;
  double prev_bar_open_;
  double prev_bar_vol_;
  double prev_prev_bar_close_;
  int neg_pnl_count_;
  void LoadParams();
  void onBarUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  // void updateTrailingSL(double _price_);

 public:
  HighMoveTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                         HFSAT::DebugLogger& dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                         int _aggressive_get_flat_mfm, int eff_squareoff_start_utc_mfm_, double bid_multiplier_,
                         double ask_multiplier_);

  virtual ~HighMoveTheoCalculator() {}

  // void SetupPNLHooks();
  virtual int total_pnl() { return total_pnl_; }
  void ReloadConfig();
};
