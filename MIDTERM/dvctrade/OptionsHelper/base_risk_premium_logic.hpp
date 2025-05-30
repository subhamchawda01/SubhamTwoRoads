/**
 *      \file OptionsUtils/base_param_logic.hpp
 *
 *      \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 *          Address:
 *                 Suite No 353, Evoma, #14, Bhattarhalli,
 *                 Old Madras Road, Near Garden City College,
 *                 KR Puram, Bangalore 560049, India
 *                 +91 80 4190 355
 **/

#pragma once

#include "dvctrade/ExecLogic/trade_vars.hpp"
#include "dvctrade/InitCommon/options_paramset.hpp"
#include "dvctrade/ModelMath/mult_model_creator.hpp"
#include "dvctrade/ExecLogic/ExecLogicHelper/multiple_indicator_helper.hpp"
#define MAX_POS_MAP_SIZE 64

// RISK + PREMIUM STRUCTURE

// every contract needs paramset and a delta_tradevarset_map_
// 1) role_of_paramset: it should have max_loss, max_opentradeloss
// 2) role_of_delta_tradevarset_: for a given delta it should return expected premium
// 3) some params needs to use transformation function per contract, threhold maps
// 4) some params dont neeed any transformatoin, simply copy : unit_trade_size, max_loss_per_underlying ( not agg and
// nblo for now )

// first_iteration:
// basic_class : read per underlying, use same values for all contracts ( i.e no transformation functions )
//        a) read_function
//        b) exists_all_required_params
//        c) from_underlying_to_per_contract_function { build_paramset and build_delta_tradevar_set }
//        d) we need access function for paramset and delta_tradevarset_/current_tradevarset_[_current_delta_]

// Two Structs: CommonParamSet & OptionRiskPremiumSet

namespace HFSAT {

// risk and exec variables
typedef struct {
  double cooloff_interval_;
  double agg_cooloff_interval_;
  bool allowed_to_aggress_;
  int max_position_to_aggress_;
  int max_int_spread_to_place_;
  int max_int_spread_to_cross_;
  int min_int_price_to_place_;
  double max_loss_;
  double max_position_;
  double worst_position_;
  double max_gamma_;
  double max_vega_;
  OptionType_t type_;
} OptionRisk;  // this should be OptionsRiskAndExecControllers w/o threshold_multiplier_

class BaseOptionRiskPremium : public TimePeriodListener {
 public:
  std::vector<OptionsParamSet*> paramset_vec_;
  std::vector<std::vector<OptionRisk> > risk_matrix_;

  BaseOptionRiskPremium(DebugLogger& _dbglogger_, const Watch& _watch_,
                        std::vector<SecurityMarketView*> _underlying_smv_vec_, const int r_tradingdate_,
                        int _trading_start_mfm_, int _trading_end_mfm_);
  ~BaseOptionRiskPremium(){};

  DebugLogger& dbglogger_;
  const Watch& watch_;
  int trading_start_mfm_;
  int trading_end_mfm_;

  void OnRiskChange(int _product_index_, double current_delta_, double current_gamma_, double current_vega_);
  TradeVars_t GetPremium(int _product_index_, int _option_index_, int _position_);

  // constructor: same as read_function and build delta tradervarset
  // createoptions_risk_premium: build per contract : Builds OptionRiskPremiumStruct which has
  // delta_tradevarset_map_{represents premium} and RiskStruct {represents risk, this one simply has max_loss and
  // max_loss_perunderlying and cooloff_interval }
  // function to return OptionsParamSet
  // function to return CurrentTradeVarSet
 private:
  void BuildDeltaTradeVarSetMap(OptionsParamSet* param, const SecurityMarketView* smv,
                                DeltaTradeVarSetMap& position_tradevarset_map, int& map_pos_increment,
                                const int& P2TV_zero_idx);

  // Premium: using online stdev and bidask spread
  void BuildConstituentsPremiumMaps(int _product_index_, DeltaTradeVarSetMap& prod_position_tradevarset_map_);

  // Risk: using offline sample stdev and previous day price of future
  void BuildConstituentsRiskStructs(int _product_index_);

  void OnTimePeriodUpdate(const int num_pages_to_add_);

  std::vector<SecurityMarketView*> underlying_smv_vec_;

  const unsigned int P2TV_zero_idx_;

  std::map<std::string, std::vector<SecurityMarketView*> > underlying_2_options_smv_map_;

  TradeVars_t closeout_zeropos_tradevarset_;

  // underlying based risk/premium profile
  // total delta based current option tradevarset
  std::vector<int> map_pos_increment_vec_;
  std::vector<DeltaTradeVarSetMap> delta_tradevarset_vec_;

  // per underlying, for put use, 2*ZERO - INDEX
  std::vector<unsigned int> current_delta_tradevarset_map_index_;

  // derived option risk profile
  std::vector<std::vector<TradeVars_t> > current_tradevarset_matrix_;

  MultipleIndicatorHelper* indicators_helper_;
};
}
