#pragma once

#include "tradeengine/Executioner/BaseExecutioner.hpp"
#include "tradeengine/TheoCalc/MasterTheoCalculator.hpp"
#include "tradeengine/TheoCalc/SquareOffTheoCalculator.hpp"
#include "tradeengine/Utils/BarGenerator.hpp"

class CorrTheoCalculator : public MasterTheoCalculator {
 protected:
  void LoadParams();
  virtual void UpdateTheoPrices(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  void ComputeAndUpdateTheoListeners();
  void UpdateTheoListeners();
  void OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_);

 public:
  MasterTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                       HFSAT::DebugLogger& dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                       int _aggressive_get_flat_mfm);

  virtual ~MasterTheoCalculator() {}

  
};
