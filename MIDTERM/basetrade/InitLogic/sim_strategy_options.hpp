#pragma once

/**
    \file InitLogic/sim_strategy_options.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>

#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/slack_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "dvccode/Utils/signals.hpp"
#include "dvccode/Utils/eti_algo_tagging.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvctrade/OptionsHelper/option_vars.hpp"

#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"
#include "baseinfra/TradeUtils/market_update_manager.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"

#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"

#include "baseinfra/LoggedSources/cme_logged_message_filesource.hpp"

#include "baseinfra/LoggedSources/eurex_logged_message_filesource.hpp"

#include "baseinfra/LoggedSources/ntp_logged_message_filesource.hpp"
// #include "baseinfra/LoggedSources/bmf_logged_message_filesource.hpp"

#include "baseinfra/LoggedSources/rts_logged_message_filesource.hpp"

#include "baseinfra/LoggedSources/micex_logged_message_filesource.hpp"

#include "baseinfra/LoggedSources/liffe_logged_message_filesource.hpp"

#include "baseinfra/LoggedSources/hkex_logged_message_filesource.hpp"

#include "baseinfra/LoggedSources/ose_l1_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ose_logged_message_filesource.hpp"

#include "baseinfra/LoggedSources/eobi_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/eobi_price_feed_logged_message_filesource.hpp"

#include "baseinfra/LoggedSources/nse_logged_message_filesource.hpp"

#include "baseinfra/LoggedSources/aflash_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/aflash_logged_message_filenamer.hpp"

#include "baseinfra/LoggedSources/ice_logged_message_filesource.hpp"

#include "baseinfra/LoggedSources/puma_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/cfe_logged_message_filesource.hpp"

#include "baseinfra/LoggedSources/tmx_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ose_pricefeed_logged_message_filesource.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_shm_processor.hpp"
#include "baseinfra/LoggedSources/control_message_filesource.hpp"
#include "baseinfra/LoggedSources/hkomd_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/asx_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/asx_pf_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/sgx_pf_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/bse_pf_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/chix_l1_logged_message_filesource.hpp"

#include "dvccode/ORSMessages/control_message_livesource.hpp"
#include "dvccode/ORSMessages/ors_message_livesource.hpp"
#include "baseinfra/LoggedSources/ors_message_filesource.hpp"
#include "dvccode/ORSMessages/shortcode_ors_message_livesource_map.hpp"
#include "baseinfra/LoggedSources/shortcode_ors_message_filesource_map.hpp"
#include "baseinfra/LoggedSources/ors_message_stats_computer.hpp"

#include "baseinfra/MarketAdapter/market_adapter_list.hpp"

#include "dvctrade/ModelMath/model_creator.hpp"

#include "baseinfra/BaseTrader/base_sim_trader.hpp"
#include "baseinfra/OrderRouting/prom_order_manager.hpp"
#include "baseinfra/BaseTrader/sim_trader_helper.hpp"

#include "baseinfra/SimMarketMaker/sim_market_maker_list.hpp"
#include "baseinfra/SimMarketMaker/sim_market_maker_helper.hpp"

#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"
#include "baseinfra/SimPnls/sim_base_pnl.hpp"
#include "baseinfra/SmartOrderRouting/prom_pnl_indicator.hpp"
#include "baseinfra/SimPnls/sim_pnl_writer.hpp"

#include "dvctrade/ExecLogic/strategy_options.hpp"

#include "baseinfra/BaseUtils/curve_utils.hpp"

#include "dvccode/CommonTradeUtils/historic_price_manager.hpp"

#include "baseinfra/MarketAdapter/book_init_utils.hpp"

#include "dvctrade/ExecLogic/exec_logic_utils.hpp"

#include "dvccode/Utils/retail_update_distributor.hpp"
#include "dvccode/CDef/online_debug_logger.hpp"

#include "baseinfra/BaseUtils/curve_utils.hpp"

// For Offloaded Logging
#include "dvccode/Utils/client_logging_segment_initializer.hpp"

#include "dvccode/Utils/model_scaling.hpp"
#include "baseinfra/Tools/common_smv_source.hpp"
