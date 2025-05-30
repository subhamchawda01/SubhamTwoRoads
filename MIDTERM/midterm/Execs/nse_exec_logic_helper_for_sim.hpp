#pragma once
#include <math.h>
#include "baseinfra/SimPnls/sim_base_pnl.hpp"
#include "midterm/GeneralizedLogic/nse_exec_logic_helper.hpp"
#include "baseinfra/SimMarketMaker/sim_config.hpp"
#include "baseinfra/SimMarketMaker/price_level_sim_market_maker.hpp"
#include "baseinfra/BaseTrader/sim_trader_helper.hpp"
#include "midterm/Execs/base_modify_exec_logic_for_sim.hpp"


namespace NSE_SIMPLEEXEC {
class NseExecLogicHelperForSim : public NseExecLogicHelper {
 public:
  HFSAT::Watch& watch_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::SecurityNameIndexer& sec_name_indexer_;
  HFSAT::SimTimeSeriesInfo sim_time_series_info_;
  HFSAT::BulkFileWriter& writer_;

 public:
  NseExecLogicHelperForSim(HFSAT::Watch& watch_t, HFSAT::DebugLogger& dbglogger_t, HFSAT::BulkFileWriter& writer_t) :
	  NseExecLogicHelper(),
	  watch_(watch_t),
	  dbglogger_(dbglogger_t),
	  sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
	  sim_time_series_info_( HFSAT::SimTimeSeriesInfo(sec_name_indexer_.NumSecurityId())),
	  writer_(writer_t)
  {}

  NSE_SIMPLEEXEC::SimpleNseExecLogic* Setup( HFSAT::SmartOrderManager*& smart_order_manager_,
						std::string& shortcode_, HFSAT::SecurityMarketView* smv_,
  							NSE_SIMPLEEXEC::ParamSet* param_, HFSAT::ExternalDataListener* filesource_ ) {
  	HFSAT::BaseTrader* sim_trader_ = NULL;
  	// HFSAT::SmartOrderManager* smart_order_manager_ = NULL;
  	unsigned int t_market_model_index_ = sec_name_indexer_.GetIdFromString( shortcode_ );

  	unsigned int runtime_id_ = 1000;
    if ( sim_time_series_info_.sid_to_sim_config_.size() != sec_name_indexer_.NumSecurityId() ) {
      sim_time_series_info_.sid_to_sim_config_.resize( sec_name_indexer_.NumSecurityId() );
    }
    sim_time_series_info_.sid_to_sim_config_[ t_market_model_index_ ] = HFSAT::SimConfig::GetSimConfigsForShortcode(dbglogger_, watch_, shortcode_, "invalid");

    HFSAT::PriceLevelSimMarketMaker* sim_market_maker_ = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
        dbglogger_, watch_, *smv_, t_market_model_index_, sim_time_series_info_);

    sim_market_maker_->SubscribeL2Events(*smv_);

    sim_trader_ = HFSAT::SimTraderHelper::GetSimTrader("12345678", sim_market_maker_);
    smart_order_manager_ = new HFSAT::SmartOrderManager(dbglogger_, watch_, sec_name_indexer_, *sim_trader_,
                                                                *smv_, 1234, false, 1);

    sim_market_maker_->AddOrderNotFoundListener(smart_order_manager_);
    sim_market_maker_->AddOrderSequencedListener(smart_order_manager_);
    sim_market_maker_->AddOrderConfirmedListener(smart_order_manager_);
    sim_market_maker_->AddOrderConfCxlReplaceRejectedListener(smart_order_manager_);
    sim_market_maker_->AddOrderConfCxlReplacedListener(smart_order_manager_);
    sim_market_maker_->AddOrderCanceledListener(smart_order_manager_);
    sim_market_maker_->AddOrderExecutedListener(smart_order_manager_);
    sim_market_maker_->AddOrderRejectedListener(smart_order_manager_);

    int saci = smart_order_manager_->server_assigned_client_id_;
    sim_market_maker_->AddSecIdToSACI(saci, t_market_model_index_);

    filesource_->AddExternalDataListenerListener(sim_market_maker_);

    // create simpnl object
    HFSAT::BasePNL* p_sim_base_pnl_ = new HFSAT::SimBasePNL(
        dbglogger_, watch_, *smv_, runtime_id_, writer_);
    smart_order_manager_->SetBasePNL(p_sim_base_pnl_);

    BaseModifyExecLogic& modify_exec_logic_ = NSE_SIMPLEEXEC::BaseModifyExecLogicForSim::GetUniqueInstance();

    NSE_SIMPLEEXEC::SimpleNseExecLogic* simple_nse_exec_logic_ = new NSE_SIMPLEEXEC::NseSyntheticLegExecLogic(
          *smv_, sim_trader_, smart_order_manager_, modify_exec_logic_, dbglogger_, watch_, param_, false, shortcode_,
          HFSAT::kPriceTypeMktSizeWPrice, std::string("GENERAL"), 567828);

    smv_->subscribe_tradeprints(simple_nse_exec_logic_);
    smv_->subscribe_rawtradeprints(simple_nse_exec_logic_);
    smv_->subscribe_price_type(simple_nse_exec_logic_, HFSAT::kPriceTypeMktSizeWPrice);
    smart_order_manager_->AddExecutionListener(simple_nse_exec_logic_);
    sim_market_maker_->AddOrderRejectedListener(simple_nse_exec_logic_);

    return simple_nse_exec_logic_;
  }
};
}
