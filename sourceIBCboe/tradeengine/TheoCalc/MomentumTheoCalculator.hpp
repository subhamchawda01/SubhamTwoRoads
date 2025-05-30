#pragma once

#include "tradeengine/Executioner/BaseExecutioner.hpp"
#include "tradeengine/TheoCalc/MidTermTheoCalculator.hpp"
#include "tradeengine/TheoCalc/SquareOffTheoCalculator.hpp"
#include "tradeengine/Utils/BarGenerator.hpp"

/*! \brief Header file for MomentumTheo
*/
class MomentumTheoCalculator : public MidTermTheoCalculator {
 protected:
  int current_directional_cascade_;
  void LoadParams();
  void onBarUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  void updateTrailingSL(double _price_);

 public:
  MomentumTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                         HFSAT::DebugLogger& dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                         int _aggressive_get_flat_mfm, int eff_squareoff_start_utc_mfm_, double bid_multiplier_,
                         double ask_multiplier_);

  virtual ~MomentumTheoCalculator() {}

  // void SetupPNLHooks();
  virtual int total_pnl() { return total_pnl_; }
  void ReloadConfig();
};
