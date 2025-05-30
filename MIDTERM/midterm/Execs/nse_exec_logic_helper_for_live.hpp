#pragma once
#include <math.h>
#include "midterm/GeneralizedLogic/nse_exec_logic_helper.hpp"
#include "dvccode/Utils/client_logging_segment_initializer.hpp"
#include "dvccode/Utils/nse_refdata_loader.hpp"
#include "dvccode/Utils/nse_daily_token_symbol_handler.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "baseinfra/BaseTrader/base_live_trader.hpp"
#include "baseinfra/LivePnls/live_base_pnl.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_shm_processor.hpp"
#include "midterm/Execs/base_modify_exec_logic_for_live.hpp"

namespace NSE_SIMPLEEXEC {
class NseExecLogicHelperForLive : public NseExecLogicHelper {
 public:
  HFSAT::Watch& watch_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::SecurityNameIndexer& sec_name_indexer_;
  HFSAT::Utils::ClientLoggingSegmentInitializer* client_writer_;
  HFSAT::MDSMessages::CombinedMDSMessagesShmProcessor& combined_mds_messages_shm_processor_;
  std::string strategy_name_;

 public:
  NseExecLogicHelperForLive(HFSAT::Watch& watch_t, HFSAT::DebugLogger& dbglogger_t,
                            HFSAT::Utils::ClientLoggingSegmentInitializer* writer_t,
                            HFSAT::MDSMessages::CombinedMDSMessagesShmProcessor& shm_processor_t,
                            std::string strategy_name_t)
      : NseExecLogicHelper(),
        watch_(watch_t),
        dbglogger_(dbglogger_t),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
        client_writer_(writer_t),
        combined_mds_messages_shm_processor_(shm_processor_t),
        strategy_name_(strategy_name_t) {}

  NSE_SIMPLEEXEC::SimpleNseExecLogic* Setup(HFSAT::SmartOrderManager*& p_smart_order_manager_, std::string& shortcode_, HFSAT::SecurityMarketView* smv_,
                                            NSE_SIMPLEEXEC::ParamSet* param_, HFSAT::ExternalDataListener* filesource_) {
    // Get RefData Info From RefLoader
    // This exec only trades equities futures and options
    char segment = NSE_FO_SEGMENT_MARKING;
    int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
    HFSAT::Utils::NSERefDataLoader& nse_ref_data_loader =
        HFSAT::Utils::NSERefDataLoader::GetUniqueInstance(tradingdate_);
    std::map<int32_t, NSE_UDP_MDS::NSERefData>& nse_ref_data = nse_ref_data_loader.GetNSERefData(segment);
    if (0 == nse_ref_data.size()) {
      dbglogger_ << "FOR SEGMENT : " << segment << " REF DATA DOES NOT EXIST" << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }

    // get token handler and sec_def
    std::string exch_sym_ = HFSAT::NSESecurityDefinitions::GetExchSymbolNSE(shortcode_);
    std::string datasoure_sym_ = HFSAT::NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exch_sym_);
    HFSAT::Utils::NSEDailyTokenSymbolHandler& nse_daily_token_symbol_handler =
        HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(tradingdate_);
    int32_t const this_symbol_token =
        nse_daily_token_symbol_handler.GetTokenFromInternalSymbol(datasoure_sym_.c_str(), segment);

    // This is the ORS segment which we need to connect to for this live trader/strat
    HFSAT::ExchSource_t nse_segment_source = nse_ref_data[this_symbol_token].exch_source;

    // Create the live trader
    HFSAT::BaseTrader* p_base_trader_ = NULL;
    HFSAT::NetworkAccountInfoManager network_account_info_manager_;
    p_base_trader_ = new HFSAT::BaseLiveTrader(
        nse_segment_source, network_account_info_manager_.GetDepTradeAccount(HFSAT::kExchSourceNSE, shortcode_),
        network_account_info_manager_.GetDepTradeHostIp(HFSAT::kExchSourceNSE, shortcode_),
        network_account_info_manager_.GetDepTradeHostPort(HFSAT::kExchSourceNSE, shortcode_), watch_, dbglogger_);

    // create order manager
    unsigned int runtime_id_ = 1000;
    // HFSAT::SmartOrderManager* p_smart_order_manager_ = NULL;
    p_smart_order_manager_ = new HFSAT::SmartOrderManager(dbglogger_, watch_, sec_name_indexer_, *(p_base_trader_),
                                                          *smv_, runtime_id_, true, 1);

    int security_id_ = sec_name_indexer_.GetIdFromString(shortcode_);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderNotFoundListener(security_id_,
                                                                                          p_smart_order_manager_);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderSequencedListener(security_id_,
                                                                                           p_smart_order_manager_);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderConfirmedListener(security_id_,
                                                                                           p_smart_order_manager_);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderConfCxlReplacedListener(
        security_id_, p_smart_order_manager_);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderConfCxlReplaceRejectListener(
        security_id_, p_smart_order_manager_);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderCanceledListener(security_id_,
                                                                                          p_smart_order_manager_);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderExecutedListener(security_id_,
                                                                                          p_smart_order_manager_);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderRejectedListener(security_id_,
                                                                                          p_smart_order_manager_);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderRejectedDueToFundsListener(
        security_id_, p_smart_order_manager_);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderInternallyMatchedListener(
        security_id_, p_smart_order_manager_);

    // create live base pnl
    HFSAT::BasePNL* p_base_pnl_ = NULL;
    p_base_pnl_ =
        new HFSAT::LiveBasePNL(dbglogger_, watch_, *smv_, runtime_id_, client_writer_);
    p_smart_order_manager_->SetBasePNL(p_base_pnl_);

    BaseModifyExecLogic& modify_exec_logic_ = NSE_SIMPLEEXEC::BaseModifyExecLogicForLive::GetUniqueInstance();

    NSE_SIMPLEEXEC::SimpleNseExecLogic* simple_nse_exec_logic_ = new NSE_SIMPLEEXEC::NseSyntheticLegExecLogic(
        *smv_, p_base_trader_, p_smart_order_manager_, modify_exec_logic_, dbglogger_, watch_, param_, true, shortcode_,
        HFSAT::kPriceTypeMktSizeWPrice, strategy_name_, 567828);

    smv_->subscribe_tradeprints(simple_nse_exec_logic_);
    smv_->subscribe_rawtradeprints(simple_nse_exec_logic_);
    smv_->subscribe_price_type(simple_nse_exec_logic_, HFSAT::kPriceTypeMktSizeWPrice);
    p_smart_order_manager_->AddExecutionListener(simple_nse_exec_logic_);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderRejectedListener(security_id_,
                                                                                          p_smart_order_manager_);

    return simple_nse_exec_logic_;
  }
};
}
